/* $Id: service.cpp 36529 2011-04-04 13:54:13Z vboxsync $ */
/** @file
 * Guest Property Service: Host service entry points.
 */

/*
 * Copyright (C) 2008 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/** @page pg_svc_guest_properties   Guest Property HGCM Service
 *
 * This HGCM service allows the guest to set and query values in a property
 * store on the host.  The service proxies the guest requests to the service
 * owner on the host using a request callback provided by the owner, and is
 * notified of changes to properties made by the host.  It forwards these
 * notifications to clients in the guest which have expressed interest and
 * are waiting for notification.
 *
 * The service currently consists of two threads.  One of these is the main
 * HGCM service thread which deals with requests from the guest and from the
 * host.  The second thread sends the host asynchronous notifications of
 * changes made by the guest and deals with notification timeouts.
 *
 * Guest requests to wait for notification are added to a list of open
 * notification requests and completed when a corresponding guest property
 * is changed or when the request times out.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_HGCM
#include <VBox/HostServices/GuestPropertySvc.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/cpp/autores.h>
#include <iprt/cpp/utils.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/req.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/time.h>

#include <memory>  /* for auto_ptr */
#include <string>
#include <list>

namespace guestProp {

/**
 * Structure for holding a property
 */
struct Property
{
    /** The name of the property */
    std::string mName;
    /** The property value */
    std::string mValue;
    /** The timestamp of the property */
    uint64_t mTimestamp;
    /** The property flags */
    uint32_t mFlags;

    /** Default constructor */
    Property() : mTimestamp(0), mFlags(NILFLAG) {}
    /** Constructor with const char * */
    Property(const char *pcszName, const char *pcszValue,
             uint64_t u64Timestamp, uint32_t u32Flags)
        : mName(pcszName), mValue(pcszValue), mTimestamp(u64Timestamp),
          mFlags(u32Flags) {}
    /** Constructor with std::string */
    Property(std::string name, std::string value, uint64_t u64Timestamp,
             uint32_t u32Flags)
        : mName(name), mValue(value), mTimestamp(u64Timestamp),
          mFlags(u32Flags) {}

    /** Does the property name match one of a set of patterns? */
    bool Matches(const char *pszPatterns) const
    {
        return (   pszPatterns[0] == '\0'  /* match all */
                || RTStrSimplePatternMultiMatch(pszPatterns, RTSTR_MAX,
                                                mName.c_str(), RTSTR_MAX,
                                                NULL)
               );
    }

    /** Are two properties equal? */
    bool operator==(const Property &prop)
    {
        if (mTimestamp != prop.mTimestamp)
            return false;
        if (mFlags != prop.mFlags)
            return false;
        if (mName != prop.mName)
            return false;
        if (mValue != prop.mValue)
            return false;
        return true;
    }

    /* Is the property nil? */
    bool isNull()
    {
        return mName.empty();
    }
};
/** The properties list type */
typedef std::list <Property> PropertyList;

/**
 * Structure for holding an uncompleted guest call
 */
struct GuestCall
{
    /** The call handle */
    VBOXHGCMCALLHANDLE mHandle;
    /** The function that was requested */
    uint32_t mFunction;
    /** The call parameters */
    VBOXHGCMSVCPARM *mParms;
    /** The default return value, used for passing warnings */
    int mRc;

    /** The standard constructor */
    GuestCall() : mFunction(0) {}
    /** The normal constructor */
    GuestCall(VBOXHGCMCALLHANDLE aHandle, uint32_t aFunction,
              VBOXHGCMSVCPARM aParms[], int aRc)
              : mHandle(aHandle), mFunction(aFunction), mParms(aParms),
                mRc(aRc) {}
};
/** The guest call list type */
typedef std::list <GuestCall> CallList;

/**
 * Class containing the shared information service functionality.
 */
class Service : public RTCNonCopyable
{
private:
    /** Type definition for use in callback functions */
    typedef Service SELF;
    /** HGCM helper functions. */
    PVBOXHGCMSVCHELPERS mpHelpers;
    /** Global flags for the service */
    ePropFlags meGlobalFlags;
    /** The property list */
    PropertyList mProperties;
    /** The list of property changes for guest notifications */
    PropertyList mGuestNotifications;
    /** The list of outstanding guest notification calls */
    CallList mGuestWaiters;
    /** @todo we should have classes for thread and request handler thread */
    /** Callback function supplied by the host for notification of updates
     * to properties */
    PFNHGCMSVCEXT mpfnHostCallback;
    /** User data pointer to be supplied to the host callback function */
    void *mpvHostData;
    /** The previous timestamp.
     * This is used by getCurrentTimestamp() to decrease the chance of
     * generating duplicate timestamps.  */
    uint64_t mPrevTimestamp;
    /** The number of consecutive timestamp adjustments that we've made.
     * Together with mPrevTimestamp, this defines a set of obsolete timestamp
     * values: {(mPrevTimestamp - mcTimestampAdjustments), ..., mPrevTimestamp} */
    uint64_t mcTimestampAdjustments;

    /**
     * Get the next property change notification from the queue of saved
     * notification based on the timestamp of the last notification seen.
     * Notifications will only be reported if the property name matches the
     * pattern given.
     *
     * @returns iprt status value
     * @returns VWRN_NOT_FOUND if the last notification was not found in the queue
     * @param   pszPatterns   the patterns to match the property name against
     * @param   u64Timestamp  the timestamp of the last notification
     * @param   pProp         where to return the property found.  If none is
     *                        found this will be set to nil.
     * @thread  HGCM
     */
    int getOldNotification(const char *pszPatterns, uint64_t u64Timestamp,
                           Property *pProp)
    {
        AssertPtrReturn(pszPatterns, VERR_INVALID_POINTER);
        /* Zero means wait for a new notification. */
        AssertReturn(u64Timestamp != 0, VERR_INVALID_PARAMETER);
        AssertPtrReturn(pProp, VERR_INVALID_POINTER);
        int rc = getOldNotificationInternal(pszPatterns, u64Timestamp, pProp);
#ifdef VBOX_STRICT
        /*
         * ENSURE that pProp is the first event in the notification queue that:
         *  - Appears later than u64Timestamp
         *  - Matches the pszPatterns
         */
        /** @todo r=bird: This incorrectly ASSUMES that mTimestamp is unique.
         *  The timestamp resolution can be very coarse on windows for instance. */
        PropertyList::const_iterator it = mGuestNotifications.begin();
        for (;    it != mGuestNotifications.end()
               && it->mTimestamp != u64Timestamp; ++it)
            {}
        if (it == mGuestNotifications.end())  /* Not found */
            it = mGuestNotifications.begin();
        else
            ++it;  /* Next event */
        for (;    it != mGuestNotifications.end()
               && it->mTimestamp != pProp->mTimestamp; ++it)
            Assert(!it->Matches(pszPatterns));
        if (pProp->mTimestamp != 0)
        {
            Assert(*pProp == *it);
            Assert(pProp->Matches(pszPatterns));
        }
#endif /* VBOX_STRICT */
        return rc;
    }

    /**
     * Check whether we have permission to change a property.
     *
     * @returns Strict VBox status code.
     * @retval  VINF_SUCCESS if we do.
     * @retval  VERR_PERMISSION_DENIED if the value is read-only for the requesting
     *          side.
     * @retval  VINF_PERMISSION_DENIED if the side is globally marked read-only.
     *
     * @param   eFlags   the flags on the property in question
     * @param   isGuest  is the guest or the host trying to make the change?
     */
    int checkPermission(ePropFlags eFlags, bool isGuest)
    {
        if (eFlags & (isGuest ? RDONLYGUEST : RDONLYHOST))
            return VERR_PERMISSION_DENIED;
        if (isGuest && (meGlobalFlags & RDONLYGUEST))
            return VINF_PERMISSION_DENIED;
        return VINF_SUCCESS;
    }

public:
    explicit Service(PVBOXHGCMSVCHELPERS pHelpers)
        : mpHelpers(pHelpers)
        , meGlobalFlags(NILFLAG)
        , mpfnHostCallback(NULL)
        , mpvHostData(NULL)
        , mPrevTimestamp(0)
        , mcTimestampAdjustments(0)
    { }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnUnload
     * Simply deletes the service object
     */
    static DECLCALLBACK(int) svcUnload (void *pvService)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->uninit();
        AssertRC(rc);
        if (RT_SUCCESS(rc))
            delete pSelf;
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnConnect
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcConnectDisconnect (void * /* pvService */,
                                                   uint32_t /* u32ClientID */,
                                                   void * /* pvClient */)
    {
        return VINF_SUCCESS;
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnCall
     * Wraps to the call member function
     */
    static DECLCALLBACK(void) svcCall (void * pvService,
                                       VBOXHGCMCALLHANDLE callHandle,
                                       uint32_t u32ClientID,
                                       void *pvClient,
                                       uint32_t u32Function,
                                       uint32_t cParms,
                                       VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturnVoid(VALID_PTR(pvService));
        LogFlowFunc (("pvService=%p, callHandle=%p, u32ClientID=%u, pvClient=%p, u32Function=%u, cParms=%u, paParms=%p\n", pvService, callHandle, u32ClientID, pvClient, u32Function, cParms, paParms));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->call(callHandle, u32ClientID, pvClient, u32Function, cParms, paParms);
        LogFlowFunc (("returning\n"));
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnHostCall
     * Wraps to the hostCall member function
     */
    static DECLCALLBACK(int) svcHostCall (void *pvService,
                                          uint32_t u32Function,
                                          uint32_t cParms,
                                          VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        LogFlowFunc (("pvService=%p, u32Function=%u, cParms=%u, paParms=%p\n", pvService, u32Function, cParms, paParms));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        int rc = pSelf->hostCall(u32Function, cParms, paParms);
        LogFlowFunc (("rc=%Rrc\n", rc));
        return rc;
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnRegisterExtension
     * Installs a host callback for notifications of property changes.
     */
    static DECLCALLBACK(int) svcRegisterExtension (void *pvService,
                                                   PFNHGCMSVCEXT pfnExtension,
                                                   void *pvExtension)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->mpfnHostCallback = pfnExtension;
        pSelf->mpvHostData = pvExtension;
        return VINF_SUCCESS;
    }
private:
    static DECLCALLBACK(int) reqThreadFn(RTTHREAD ThreadSelf, void *pvUser);
    uint64_t getCurrentTimestamp(void);
    int validateName(const char *pszName, uint32_t cbName);
    int validateValue(const char *pszValue, uint32_t cbValue);
    int setPropertyBlock(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int getProperty(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int setProperty(uint32_t cParms, VBOXHGCMSVCPARM paParms[], bool isGuest);
    int delProperty(uint32_t cParms, VBOXHGCMSVCPARM paParms[], bool isGuest);
    int enumProps(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int getNotification(VBOXHGCMCALLHANDLE callHandle, uint32_t cParms,
                        VBOXHGCMSVCPARM paParms[]);
    int getOldNotificationInternal(const char *pszPattern,
                                   uint64_t u64Timestamp, Property *pProp);
    int getNotificationWriteOut(VBOXHGCMSVCPARM paParms[], Property prop);
    void doNotifications(const char *pszProperty, uint64_t u64Timestamp);
    int notifyHost(const char *pszName, const char *pszValue,
                   uint64_t u64Timestamp, const char *pszFlags);

    void call (VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID,
               void *pvClient, uint32_t eFunction, uint32_t cParms,
               VBOXHGCMSVCPARM paParms[]);
    int hostCall (uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int uninit ();
};


/**
 * Gets the current timestamp.
 *
 * Since the RTTimeNow resolution can be very coarse, this method takes some
 * simple steps to try avoid returning the same timestamp for two consecutive
 * calls.  Code like getOldNotification() more or less assumes unique
 * timestamps.
 *
 * @returns Nanosecond timestamp.
 */
uint64_t Service::getCurrentTimestamp(void)
{
    RTTIMESPEC time;
    uint64_t u64NanoTS = RTTimeSpecGetNano(RTTimeNow(&time));
    if (mPrevTimestamp - u64NanoTS > mcTimestampAdjustments)
        mcTimestampAdjustments = 0;
    else
    {
        mcTimestampAdjustments++;
        u64NanoTS = mPrevTimestamp + 1;
    }
    this->mPrevTimestamp = u64NanoTS;
    return u64NanoTS;
}

/**
 * Check that a string fits our criteria for a property name.
 *
 * @returns IPRT status code
 * @param   pszName   the string to check, must be valid Utf8
 * @param   cbName    the number of bytes @a pszName points to, including the
 *                    terminating '\0'
 * @thread  HGCM
 */
int Service::validateName(const char *pszName, uint32_t cbName)
{
    LogFlowFunc(("cbName=%d\n", cbName));
    int rc = VINF_SUCCESS;
    if (RT_SUCCESS(rc) && (cbName < 2))
        rc = VERR_INVALID_PARAMETER;
    for (unsigned i = 0; RT_SUCCESS(rc) && i < cbName; ++i)
        if (pszName[i] == '*' || pszName[i] == '?' || pszName[i] == '|')
            rc = VERR_INVALID_PARAMETER;
    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}


/**
 * Check a string fits our criteria for the value of a guest property.
 *
 * @returns IPRT status code
 * @param   pszValue  the string to check, must be valid Utf8
 * @param   cbValue   the length in bytes of @a pszValue, including the
 *                    terminator
 * @thread  HGCM
 */
int Service::validateValue(const char *pszValue, uint32_t cbValue)
{
    LogFlowFunc(("cbValue=%d\n", cbValue));

    int rc = VINF_SUCCESS;
    if (RT_SUCCESS(rc) && cbValue == 0)
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        LogFlow(("    pszValue=%s\n", cbValue > 0 ? pszValue : NULL));
    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}

/**
 * Set a block of properties in the property registry, checking the validity
 * of the arguments passed.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 */
int Service::setPropertyBlock(uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    char **ppNames, **ppValues, **ppFlags;
    uint64_t *pTimestamps;
    uint32_t cbDummy;
    int rc = VINF_SUCCESS;

    /*
     * Get and validate the parameters
     */
    if (   (cParms != 4)
        || RT_FAILURE(paParms[0].getPointer ((void **) &ppNames, &cbDummy))
        || RT_FAILURE(paParms[1].getPointer ((void **) &ppValues, &cbDummy))
        || RT_FAILURE(paParms[2].getPointer ((void **) &pTimestamps, &cbDummy))
        || RT_FAILURE(paParms[3].getPointer ((void **) &ppFlags, &cbDummy))
        )
        rc = VERR_INVALID_PARAMETER;

    /*
     * Add the properties to the end of the list.  If we succeed then we
     * will remove duplicates afterwards.
     */
    /* Remember the last property before we started adding, for rollback or
     * cleanup. */
    PropertyList::iterator itEnd = mProperties.end();
    if (!mProperties.empty())
        --itEnd;
    try
    {
        for (unsigned i = 0; RT_SUCCESS(rc) && ppNames[i] != NULL; ++i)
        {
            uint32_t fFlags;
            if (   !VALID_PTR(ppNames[i])
                || !VALID_PTR(ppValues[i])
                || !VALID_PTR(ppFlags[i])
              )
                rc = VERR_INVALID_POINTER;
            if (RT_SUCCESS(rc))
                rc = validateFlags(ppFlags[i], &fFlags);
            if (RT_SUCCESS(rc))
                mProperties.push_back(Property(ppNames[i], ppValues[i],
                                               pTimestamps[i], fFlags));
        }
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }

    /*
     * If all went well then remove the duplicate elements.
     */
    if (RT_SUCCESS(rc) && itEnd != mProperties.end())
    {
        ++itEnd;
        for (unsigned i = 0; ppNames[i] != NULL; ++i)
            for (PropertyList::iterator it = mProperties.begin(); it != itEnd; ++it)
                if (it->mName.compare(ppNames[i]) == 0)
                {
                    mProperties.erase(it);
                    break;
                }
    }

    /*
     * If something went wrong then rollback.  This is possible because we
     * haven't deleted anything yet.
     */
    if (RT_FAILURE(rc))
    {
        if (itEnd != mProperties.end())
            ++itEnd;
        mProperties.erase(itEnd, mProperties.end());
    }
    return rc;
}

/**
 * Retrieve a value from the property registry by name, checking the validity
 * of the arguments passed.  If the guest has not allocated enough buffer
 * space for the value then we return VERR_OVERFLOW and set the size of the
 * buffer needed in the "size" HGCM parameter.  If the name was not found at
 * all, we return VERR_NOT_FOUND.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 */
int Service::getProperty(uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    const char *pcszName = NULL;        /* shut up gcc */
    char *pchBuf;
    uint32_t cchName, cchBuf;
    char szFlags[MAX_FLAGS_LEN];

    /*
     * Get and validate the parameters
     */
    LogFlowThisFunc(("\n"));
    if (   cParms != 4  /* Hardcoded value as the next lines depend on it. */
        || RT_FAILURE (paParms[0].getString(&pcszName, &cchName))  /* name */
        || RT_FAILURE (paParms[1].getBuffer((void **) &pchBuf, &cchBuf))  /* buffer */
       )
        rc = VERR_INVALID_PARAMETER;
    else
        rc = validateName(pcszName, cchName);

    /*
     * Read and set the values we will return
     */

    /* Get the value size */
    PropertyList::const_iterator it;
    if (RT_SUCCESS(rc))
    {
        rc = VERR_NOT_FOUND;
        for (it = mProperties.begin(); it != mProperties.end(); ++it)
            if (it->mName.compare(pcszName) == 0)
            {
                rc = VINF_SUCCESS;
                break;
            }
    }
    if (RT_SUCCESS(rc))
        rc = writeFlags(it->mFlags, szFlags);
    if (RT_SUCCESS(rc))
    {
        /* Check that the buffer is big enough */
        size_t cchBufActual = it->mValue.size() + 1 + strlen(szFlags);
        paParms[3].setUInt32 ((uint32_t)cchBufActual);
        if (cchBufActual <= cchBuf)
        {
            /* Write the value, flags and timestamp */
            it->mValue.copy(pchBuf, cchBuf, 0);
            pchBuf[it->mValue.size()] = '\0'; /* Terminate the value */
            strcpy(pchBuf + it->mValue.size() + 1, szFlags);
            paParms[2].setUInt64 (it->mTimestamp);

            /*
             * Done!  Do exit logging and return.
             */
            Log2(("Queried string %s, value=%s, timestamp=%lld, flags=%s\n",
                  pcszName, it->mValue.c_str(), it->mTimestamp, szFlags));
        }
        else
            rc = VERR_BUFFER_OVERFLOW;
    }

    LogFlowThisFunc(("rc = %Rrc\n", rc));
    return rc;
}

/**
 * Set a value in the property registry by name, checking the validity
 * of the arguments passed.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @param   isGuest is this call coming from the guest (or the host)?
 * @throws  std::bad_alloc  if an out of memory condition occurs
 * @thread  HGCM
 */
int Service::setProperty(uint32_t cParms, VBOXHGCMSVCPARM paParms[], bool isGuest)
{
    int rc = VINF_SUCCESS;
    const char *pcszName = NULL;        /* shut up gcc */
    const char *pcszValue = NULL;       /* ditto */
    const char *pcszFlags = NULL;
    uint32_t cchName = 0;               /* ditto */
    uint32_t cchValue = 0;              /* ditto */
    uint32_t cchFlags = 0;
    uint32_t fFlags = NILFLAG;
    uint64_t u64TimeNano = getCurrentTimestamp();

    LogFlowThisFunc(("\n"));
    /*
     * First of all, make sure that we won't exceed the maximum number of properties.
     */
    if (mProperties.size() >= MAX_PROPS)
        rc = VERR_TOO_MUCH_DATA;

    /*
     * General parameter correctness checking.
     */
    if (   RT_SUCCESS(rc)
        && (   (cParms < 2) || (cParms > 3)  /* Hardcoded value as the next lines depend on it. */
            || RT_FAILURE(paParms[0].getString(&pcszName, &cchName))  /* name */
            || RT_FAILURE(paParms[1].getString(&pcszValue, &cchValue))  /* value */
            || (   (3 == cParms)
                && RT_FAILURE(paParms[2].getString(&pcszFlags, &cchFlags)) /* flags */
               )
           )
       )
        rc = VERR_INVALID_PARAMETER;

    /*
     * Check the values passed in the parameters for correctness.
     */
    if (RT_SUCCESS(rc))
        rc = validateName(pcszName, cchName);
    if (RT_SUCCESS(rc))
        rc = validateValue(pcszValue, cchValue);
    if ((3 == cParms) && RT_SUCCESS(rc))
        rc = RTStrValidateEncodingEx(pcszFlags, cchFlags,
                                     RTSTR_VALIDATE_ENCODING_ZERO_TERMINATED);
    if ((3 == cParms) && RT_SUCCESS(rc))
        rc = validateFlags(pcszFlags, &fFlags);
    if (RT_SUCCESS(rc))
    {
        /*
         * If the property already exists, check its flags to see if we are allowed
         * to change it.
         */
        PropertyList::iterator it;
        bool found = false;
        for (it = mProperties.begin(); it != mProperties.end(); ++it)
            if (it->mName.compare(pcszName) == 0)
            {
                found = true;
                break;
            }

        rc = checkPermission(found ? (ePropFlags)it->mFlags : NILFLAG,
                             isGuest);
        if (rc == VINF_SUCCESS)
        {
            /*
             * Set the actual value
             */
            if (found)
            {
                it->mValue = pcszValue;
                it->mTimestamp = u64TimeNano;
                it->mFlags = fFlags;
            }
            else  /* This can throw.  No problem as we have nothing to roll back. */
                mProperties.push_back(Property(pcszName, pcszValue, u64TimeNano, fFlags));

            /*
             * Send a notification to the host and return.
             */
            // if (isGuest)  /* Notify the host even for properties that the host
            //                * changed.  Less efficient, but ensures consistency. */
                doNotifications(pcszName, u64TimeNano);
            Log2(("Set string %s, rc=%Rrc, value=%s\n", pcszName, rc, pcszValue));
        }
    }

    LogFlowThisFunc(("rc = %Rrc\n", rc));
    return rc;
}


/**
 * Remove a value in the property registry by name, checking the validity
 * of the arguments passed.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @param   isGuest is this call coming from the guest (or the host)?
 * @thread  HGCM
 */
int Service::delProperty(uint32_t cParms, VBOXHGCMSVCPARM paParms[], bool isGuest)
{
    int rc = VINF_SUCCESS;
    const char *pcszName = NULL;        /* shut up gcc */
    uint32_t cbName;

    LogFlowThisFunc(("\n"));

    /*
     * Check the user-supplied parameters.
     */
    if (   (cParms == 1)  /* Hardcoded value as the next lines depend on it. */
        && RT_SUCCESS(paParms[0].getString(&pcszName, &cbName))  /* name */
       )
        rc = validateName(pcszName, cbName);
    else
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
    {
        /*
         * If the property exists, check its flags to see if we are allowed
         * to change it.
         */
        PropertyList::iterator it;
        bool found = false;
        for (it = mProperties.begin(); it != mProperties.end(); ++it)
            if (it->mName.compare(pcszName) == 0)
            {
                found = true;
                rc = checkPermission((ePropFlags)it->mFlags, isGuest);
                break;
            }

        /*
         * And delete the property if all is well.
         */
        if (rc == VINF_SUCCESS && found)
        {
            uint64_t u64Timestamp = getCurrentTimestamp();
            mProperties.erase(it);
            // if (isGuest)  /* Notify the host even for properties that the host
            //                * changed.  Less efficient, but ensures consistency. */
                doNotifications(pcszName, u64Timestamp);
        }
    }

    LogFlowThisFunc(("rc = %Rrc\n", rc));
    return rc;
}

/**
 * Enumerate guest properties by mask, checking the validity
 * of the arguments passed.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 */
int Service::enumProps(uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;

    /*
     * Get the HGCM function arguments.
     */
    char *pcchPatterns = NULL, *pchBuf = NULL;
    uint32_t cchPatterns = 0, cchBuf = 0;
    LogFlowThisFunc(("\n"));
    if (   (cParms != 3)  /* Hardcoded value as the next lines depend on it. */
        || RT_FAILURE(paParms[0].getString(&pcchPatterns, &cchPatterns))  /* patterns */
        || RT_FAILURE(paParms[1].getBuffer((void **) &pchBuf, &cchBuf))  /* return buffer */
       )
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc) && cchPatterns > MAX_PATTERN_LEN)
        rc = VERR_TOO_MUCH_DATA;

    /*
     * First repack the patterns into the format expected by RTStrSimplePatternMatch()
     */
    char pszPatterns[MAX_PATTERN_LEN];
    if (RT_SUCCESS(rc))
    {
        for (unsigned i = 0; i < cchPatterns - 1; ++i)
            if (pcchPatterns[i] != '\0')
                pszPatterns[i] = pcchPatterns[i];
            else
                pszPatterns[i] = '|';
        pszPatterns[cchPatterns - 1] = '\0';
    }

    /*
     * Next enumerate into a temporary buffer.  This can throw, but this is
     * not a problem as we have nothing to roll back.
     */
    std::string buffer;
    for (PropertyList::const_iterator it = mProperties.begin();
         RT_SUCCESS(rc) && (it != mProperties.end()); ++it)
    {
        if (it->Matches(pszPatterns))
        {
            char szFlags[MAX_FLAGS_LEN];
            char szTimestamp[256];
            uint32_t cchTimestamp;
            buffer += it->mName;
            buffer += '\0';
            buffer += it->mValue;
            buffer += '\0';
            cchTimestamp = RTStrFormatNumber(szTimestamp, it->mTimestamp,
                                             10, 0, 0, 0);
            buffer.append(szTimestamp, cchTimestamp);
            buffer += '\0';
            rc = writeFlags(it->mFlags, szFlags);
            if (RT_SUCCESS(rc))
                buffer += szFlags;
            buffer += '\0';
        }
    }
    if (RT_SUCCESS(rc))
        buffer.append(4, '\0');  /* The final terminators */

    /*
     * Finally write out the temporary buffer to the real one if it is not too
     * small.
     */
    if (RT_SUCCESS(rc))
    {
        paParms[2].setUInt32 ((uint32_t)buffer.size());
        /* Copy the memory if it fits into the guest buffer */
        if (buffer.size() <= cchBuf)
            buffer.copy(pchBuf, cchBuf);
        else
            rc = VERR_BUFFER_OVERFLOW;
    }
    return rc;
}


/** Helper query used by getOldNotification */
int Service::getOldNotificationInternal(const char *pszPatterns,
                                        uint64_t u64Timestamp,
                                        Property *pProp)
{
    /* We count backwards, as the guest should normally be querying the
     * most recent events. */
    int rc = VWRN_NOT_FOUND;
    PropertyList::reverse_iterator it = mGuestNotifications.rbegin();
    for (; it != mGuestNotifications.rend(); ++it)
        if (it->mTimestamp == u64Timestamp)
        {
            rc = VINF_SUCCESS;
            break;
        }

    /* Now look for an event matching the patterns supplied.  The base()
     * member conveniently points to the following element. */
    PropertyList::iterator base = it.base();
    for (; base != mGuestNotifications.end(); ++base)
        if (base->Matches(pszPatterns))
        {
            *pProp = *base;
            return rc;
        }
    *pProp = Property();
    return rc;
}


/** Helper query used by getNotification */
int Service::getNotificationWriteOut(VBOXHGCMSVCPARM paParms[], Property prop)
{
    int rc = VINF_SUCCESS;
    /* Format the data to write to the buffer. */
    std::string buffer;
    uint64_t u64Timestamp;
    char *pchBuf;
    uint32_t cchBuf;
    rc = paParms[2].getBuffer((void **) &pchBuf, &cchBuf);
    if (RT_SUCCESS(rc))
    {
        char szFlags[MAX_FLAGS_LEN];
        rc = writeFlags(prop.mFlags, szFlags);
        if (RT_SUCCESS(rc))
        {
            buffer += prop.mName;
            buffer += '\0';
            buffer += prop.mValue;
            buffer += '\0';
            buffer += szFlags;
            buffer += '\0';
            u64Timestamp = prop.mTimestamp;
        }
    }
    /* Write out the data. */
    if (RT_SUCCESS(rc))
    {
        paParms[1].setUInt64(u64Timestamp);
        paParms[3].setUInt32((uint32_t)buffer.size());
        if (buffer.size() <= cchBuf)
            buffer.copy(pchBuf, cchBuf);
        else
            rc = VERR_BUFFER_OVERFLOW;
    }
    return rc;
}


/**
 * Get the next guest notification.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 * @throws  can throw std::bad_alloc
 */
int Service::getNotification(VBOXHGCMCALLHANDLE callHandle, uint32_t cParms,
                             VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    char *pszPatterns = NULL;           /* shut up gcc */
    char *pchBuf;
    uint32_t cchPatterns = 0;
    uint32_t cchBuf = 0;
    uint64_t u64Timestamp;

    /*
     * Get the HGCM function arguments and perform basic verification.
     */
    LogFlowThisFunc(("\n"));
    if (   (cParms != 4)  /* Hardcoded value as the next lines depend on it. */
        || RT_FAILURE(paParms[0].getString(&pszPatterns, &cchPatterns))  /* patterns */
        || RT_FAILURE(paParms[1].getUInt64(&u64Timestamp))  /* timestamp */
        || RT_FAILURE(paParms[2].getBuffer((void **) &pchBuf, &cchBuf))  /* return buffer */
       )
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        LogFlow(("    pszPatterns=%s, u64Timestamp=%llu\n", pszPatterns,
                 u64Timestamp));

    /*
     * If no timestamp was supplied or no notification was found in the queue
     * of old notifications, enqueue the request in the waiting queue.
     */
    Property prop;
    if (RT_SUCCESS(rc) && u64Timestamp != 0)
        rc = getOldNotification(pszPatterns, u64Timestamp, &prop);
    if (RT_SUCCESS(rc) && prop.isNull())
    {
        mGuestWaiters.push_back(GuestCall(callHandle, GET_NOTIFICATION,
                                          paParms, rc));
        rc = VINF_HGCM_ASYNC_EXECUTE;
    }
    /*
     * Otherwise reply at once with the enqueued notification we found.
     */
    else
    {
        int rc2 = getNotificationWriteOut(paParms, prop);
        if (RT_FAILURE(rc2))
            rc = rc2;
    }
    return rc;
}


/**
 * Notify the service owner and the guest that a property has been
 * added/deleted/changed
 * @param pszProperty  the name of the property which has changed
 * @param u64Timestamp the time at which the change took place
 *
 * @thread  HGCM service
 */
void Service::doNotifications(const char *pszProperty, uint64_t u64Timestamp)
{
    int rc = VINF_SUCCESS;

    AssertPtrReturnVoid(pszProperty);
    LogFlowThisFunc (("pszProperty=%s, u64Timestamp=%llu\n", pszProperty, u64Timestamp));
    /* Ensure that our timestamp is different to the last one. */
    if (   !mGuestNotifications.empty()
        && u64Timestamp == mGuestNotifications.back().mTimestamp)
        ++u64Timestamp;

    /*
     * Try to find the property.  Create a change event if we find it and a
     * delete event if we do not.
     */
    Property prop;
    prop.mName = pszProperty;
    prop.mTimestamp = u64Timestamp;
    /* prop is currently a delete event for pszProperty */
    bool found = false;
    if (RT_SUCCESS(rc))
        for (PropertyList::const_iterator it = mProperties.begin();
             !found && it != mProperties.end(); ++it)
            if (it->mName.compare(pszProperty) == 0)
            {
                found = true;
                /* Make prop into a change event. */
                prop.mValue = it->mValue;
                prop.mFlags = it->mFlags;
            }


    /* Release waiters if applicable and add the event to the queue for
     * guest notifications */
    if (RT_SUCCESS(rc))
    {
        try
        {
            CallList::iterator it = mGuestWaiters.begin();
            while (it != mGuestWaiters.end())
            {
                const char *pszPatterns;
                uint32_t cchPatterns;
                it->mParms[0].getString(&pszPatterns, &cchPatterns);
                if (prop.Matches(pszPatterns))
                {
                    GuestCall curCall = *it;
                    int rc2 = getNotificationWriteOut(curCall.mParms, prop);
                    if (RT_SUCCESS(rc2))
                        rc2 = curCall.mRc;
                    mpHelpers->pfnCallComplete(curCall.mHandle, rc2);
                    it = mGuestWaiters.erase(it);
                }
                else
                    ++it;
            }
            mGuestNotifications.push_back(prop);
        }
        catch (std::bad_alloc)
        {
            rc = VERR_NO_MEMORY;
        }
    }
    if (mGuestNotifications.size() > MAX_GUEST_NOTIFICATIONS)
        mGuestNotifications.pop_front();

    /*
     * Host notifications - first case: if the property exists then send its
     * current value
     */
    if (found && mpfnHostCallback != NULL)
    {
        char szFlags[MAX_FLAGS_LEN];
        /* Send out a host notification */
        const char *pszValue = prop.mValue.c_str();
        if (RT_SUCCESS(rc))
            rc = writeFlags(prop.mFlags, szFlags);
        if (RT_SUCCESS(rc))
            rc = notifyHost(pszProperty, pszValue, u64Timestamp, szFlags);
    }

    /*
     * Host notifications - second case: if the property does not exist then
     * send the host an empty value
     */
    if (!found && mpfnHostCallback != NULL)
    {
        /* Send out a host notification */
        if (RT_SUCCESS(rc))
            rc = notifyHost(pszProperty, NULL, u64Timestamp, NULL);
    }
    LogFlowThisFunc (("returning\n"));
}

/**
 * Notify the service owner that a property has been added/deleted/changed.
 * @returns  IPRT status value
 * @param    pszName       the property name
 * @param    pszValue      the new value, or NULL if the property was deleted
 * @param    u64Timestamp  the time of the change
 * @param    pszFlags      the new flags string
 */
int Service::notifyHost(const char *pszName, const char *pszValue,
                        uint64_t u64Timestamp, const char *pszFlags)
{
    LogFlowFunc (("pszName=%s, pszValue=%s, u64Timestamp=%llu, pszFlags=%s\n",
                  pszName, pszValue, u64Timestamp, pszFlags));
    HOSTCALLBACKDATA HostCallbackData;
    HostCallbackData.u32Magic     = HOSTCALLBACKMAGIC;
    HostCallbackData.pcszName     = pszName;
    HostCallbackData.pcszValue    = pszValue;
    HostCallbackData.u64Timestamp = u64Timestamp;
    HostCallbackData.pcszFlags    = pszFlags;
    int rc = mpfnHostCallback (mpvHostData, 0 /*u32Function*/,
                           (void *)(&HostCallbackData),
                           sizeof(HostCallbackData));
    LogFlowFunc (("returning %Rrc\n", rc));
    return rc;
}


/**
 * Handle an HGCM service call.
 * @copydoc VBOXHGCMSVCFNTABLE::pfnCall
 * @note    All functions which do not involve an unreasonable delay will be
 *          handled synchronously.  If needed, we will add a request handler
 *          thread in future for those which do.
 *
 * @thread  HGCM
 */
void Service::call (VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID,
                    void * /* pvClient */, uint32_t eFunction, uint32_t cParms,
                    VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    LogFlowFunc(("u32ClientID = %d, fn = %d, cParms = %d, pparms = %d\n",
                 u32ClientID, eFunction, cParms, paParms));

    try
    {
        switch (eFunction)
        {
            /* The guest wishes to read a property */
            case GET_PROP:
                LogFlowFunc(("GET_PROP\n"));
                rc = getProperty(cParms, paParms);
                break;

            /* The guest wishes to set a property */
            case SET_PROP:
                LogFlowFunc(("SET_PROP\n"));
                rc = setProperty(cParms, paParms, true);
                break;

            /* The guest wishes to set a property value */
            case SET_PROP_VALUE:
                LogFlowFunc(("SET_PROP_VALUE\n"));
                rc = setProperty(cParms, paParms, true);
                break;

            /* The guest wishes to remove a configuration value */
            case DEL_PROP:
                LogFlowFunc(("DEL_PROP\n"));
                rc = delProperty(cParms, paParms, true);
                break;

            /* The guest wishes to enumerate all properties */
            case ENUM_PROPS:
                LogFlowFunc(("ENUM_PROPS\n"));
                rc = enumProps(cParms, paParms);
                break;

            /* The guest wishes to get the next property notification */
            case GET_NOTIFICATION:
                LogFlowFunc(("GET_NOTIFICATION\n"));
                rc = getNotification(callHandle, cParms, paParms);
                break;

            default:
                rc = VERR_NOT_IMPLEMENTED;
        }
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }
    LogFlowFunc(("rc = %Rrc\n", rc));
    if (rc != VINF_HGCM_ASYNC_EXECUTE)
    {
        mpHelpers->pfnCallComplete (callHandle, rc);
    }
}


/**
 * Service call handler for the host.
 * @copydoc VBOXHGCMSVCFNTABLE::pfnHostCall
 * @thread  hgcm
 */
int Service::hostCall (uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("fn = %d, cParms = %d, pparms = %d\n",
                 eFunction, cParms, paParms));

    try
    {
        switch (eFunction)
        {
            /* The host wishes to set a block of properties */
            case SET_PROPS_HOST:
                LogFlowFunc(("SET_PROPS_HOST\n"));
                rc = setPropertyBlock(cParms, paParms);
                break;

            /* The host wishes to read a configuration value */
            case GET_PROP_HOST:
                LogFlowFunc(("GET_PROP_HOST\n"));
                rc = getProperty(cParms, paParms);
                break;

            /* The host wishes to set a configuration value */
            case SET_PROP_HOST:
                LogFlowFunc(("SET_PROP_HOST\n"));
                rc = setProperty(cParms, paParms, false);
                break;

            /* The host wishes to set a configuration value */
            case SET_PROP_VALUE_HOST:
                LogFlowFunc(("SET_PROP_VALUE_HOST\n"));
                rc = setProperty(cParms, paParms, false);
                break;

            /* The host wishes to remove a configuration value */
            case DEL_PROP_HOST:
                LogFlowFunc(("DEL_PROP_HOST\n"));
                rc = delProperty(cParms, paParms, false);
                break;

            /* The host wishes to enumerate all properties */
            case ENUM_PROPS_HOST:
                LogFlowFunc(("ENUM_PROPS\n"));
                rc = enumProps(cParms, paParms);
                break;

            /* The host wishes to set global flags for the service */
            case SET_GLOBAL_FLAGS_HOST:
                LogFlowFunc(("SET_GLOBAL_FLAGS_HOST\n"));
                if (cParms == 1)
                {
                    uint32_t eFlags;
                    rc = paParms[0].getUInt32(&eFlags);
                    if (RT_SUCCESS(rc))
                        meGlobalFlags = (ePropFlags)eFlags;
                }
                else
                    rc = VERR_INVALID_PARAMETER;
                break;

            default:
                rc = VERR_NOT_SUPPORTED;
                break;
        }
    }
    catch (std::bad_alloc)
    {
        rc = VERR_NO_MEMORY;
    }

    LogFlowFunc(("rc = %Rrc\n", rc));
    return rc;
}

int Service::uninit()
{
    return VINF_SUCCESS;
}

} /* namespace guestProp */

using guestProp::Service;

/**
 * @copydoc VBOXHGCMSVCLOAD
 */
extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("ptable = %p\n", ptable));

    if (!VALID_PTR(ptable))
    {
        rc = VERR_INVALID_PARAMETER;
    }
    else
    {
        LogFlowFunc(("ptable->cbSize = %d, ptable->u32Version = 0x%08X\n", ptable->cbSize, ptable->u32Version));

        if (   ptable->cbSize != sizeof (VBOXHGCMSVCFNTABLE)
            || ptable->u32Version != VBOX_HGCM_SVC_VERSION)
        {
            rc = VERR_VERSION_MISMATCH;
        }
        else
        {
            std::auto_ptr<Service> apService;
            /* No exceptions may propagate outside. */
            try {
                apService = std::auto_ptr<Service>(new Service(ptable->pHelpers));
            } catch (int rcThrown) {
                rc = rcThrown;
            } catch (...) {
                rc = VERR_UNRESOLVED_ERROR;
            }

            if (RT_SUCCESS(rc))
            {
                /* We do not maintain connections, so no client data is needed. */
                ptable->cbClient = 0;

                ptable->pfnUnload             = Service::svcUnload;
                ptable->pfnConnect            = Service::svcConnectDisconnect;
                ptable->pfnDisconnect         = Service::svcConnectDisconnect;
                ptable->pfnCall               = Service::svcCall;
                ptable->pfnHostCall           = Service::svcHostCall;
                ptable->pfnSaveState          = NULL;  /* The service is stateless, so the normal */
                ptable->pfnLoadState          = NULL;  /* construction done before restoring suffices */
                ptable->pfnRegisterExtension  = Service::svcRegisterExtension;

                /* Service specific initialization. */
                ptable->pvService = apService.release();
            }
        }
    }

    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}

