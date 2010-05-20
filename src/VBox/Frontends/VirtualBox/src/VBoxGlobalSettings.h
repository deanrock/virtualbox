/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * VBoxGlobalSettingsData, VBoxGlobalSettings class declarations
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __VBoxGlobalSettings_h__
#define __VBoxGlobalSettings_h__

#include "CIShared.h"

/* Qt includes */
#include <QObject>

class CVirtualBox;

class VBoxGlobalSettingsData
{
public:

    VBoxGlobalSettingsData();
    VBoxGlobalSettingsData( const VBoxGlobalSettingsData &that );
    virtual ~VBoxGlobalSettingsData();
    bool operator==( const VBoxGlobalSettingsData &that ) const;

private:

    int hostkey;
    bool autoCapture;
    QString guiFeatures;
    QString languageId;
    QString maxGuestRes;
    QString remapScancodes;
    bool trayIconEnabled;
    bool dockPreviewEnabled;
    bool presentationModeEnabled;

    friend class VBoxGlobalSettings;
};

/////////////////////////////////////////////////////////////////////////////

class VBoxGlobalSettings : public QObject, public CIShared <VBoxGlobalSettingsData>
{
    Q_OBJECT
    Q_PROPERTY (int hostKey READ hostKey WRITE setHostKey)
    Q_PROPERTY (bool autoCapture READ autoCapture WRITE setAutoCapture)
    Q_PROPERTY (QString guiFeatures READ guiFeatures WRITE setGuiFeatures)
    Q_PROPERTY (QString languageId READ languageId WRITE setLanguageId)
    Q_PROPERTY (QString maxGuestRes READ maxGuestRes WRITE setMaxGuestRes)
    Q_PROPERTY (QString remapScancodes READ remapScancodes WRITE setRemapScancodes)
    Q_PROPERTY (bool trayIconEnabled READ trayIconEnabled WRITE setTrayIconEnabled)
    Q_PROPERTY (bool dockPreviewEnabled READ dockPreviewEnabled WRITE setDockPreviewEnabled)
    Q_PROPERTY (bool presentationModeEnabled READ presentationModeEnabled WRITE setPresentationModeEnabled)

public:

    VBoxGlobalSettings (bool null = true)
        : CIShared <VBoxGlobalSettingsData> (null) {}
    VBoxGlobalSettings (const VBoxGlobalSettings &that)
        : QObject(), CIShared <VBoxGlobalSettingsData> (that) {}
    VBoxGlobalSettings &operator= (const VBoxGlobalSettings &that) {
        CIShared <VBoxGlobalSettingsData>::operator= (that);
        return *this;
    }

    // Properties

    int hostKey() const { return data()->hostkey; }
    void setHostKey (int key);

    bool autoCapture() const { return data()->autoCapture; }
    void setAutoCapture (bool aAutoCapture)
    {
        mData()->autoCapture = aAutoCapture;
        resetError();
    }

    QString guiFeatures() const { return data()->guiFeatures; }
    void setGuiFeatures (const QString &aFeatures)
    {
        mData()->guiFeatures = aFeatures;
    }
    bool isFeatureActive (const char*) const;

    QString languageId() const { return data()->languageId; }
    void setLanguageId (const QString &aLanguageId)
    {
        mData()->languageId = aLanguageId;
    }

    QString maxGuestRes() const { return data()->maxGuestRes; }
    void setMaxGuestRes (const QString &aMaxGuestRes)
    {
        mData()->maxGuestRes = aMaxGuestRes;
    }

    QString remapScancodes() const { return data()->remapScancodes; }
    void setRemapScancodes (const QString &aRemapScancodes)
    {
        mData()->remapScancodes = aRemapScancodes;
    }


    bool trayIconEnabled() const { return data()->trayIconEnabled; }
    void setTrayIconEnabled (bool enabled)
    {
        mData()->trayIconEnabled = enabled;
    }

    bool dockPreviewEnabled() const { return data()->dockPreviewEnabled; }
    void setDockPreviewEnabled (bool enabled)
    {
        mData()->dockPreviewEnabled = enabled;
    }

    bool presentationModeEnabled() const { return data()->presentationModeEnabled; }
    void setPresentationModeEnabled (bool enabled)
    {
        mData()->presentationModeEnabled = enabled;
    }


    //

    void load (CVirtualBox &vbox);
    void save (CVirtualBox &vbox) const;

    /**
     *  Returns true if the last setter or #load() operation has been failed
     *  and false otherwise.
     */
    bool operator !() const { return !last_err.isEmpty(); }

    /**
     *  Returns the description of the error set by the last setter or #load()
     *  operation, or an empty (or null) string if the last operation was
     *  successful.
     */
    QString lastError() const { return last_err; }

    QString publicProperty (const QString &publicName) const;
    bool setPublicProperty (const QString &publicName, const QString &value);

signals:

    /**
     *  This sighal is emitted only when #setPublicProperty() or #load() is called.
     *  Direct modification of properties through individual setters or through
     *  QObject::setProperty() currently cannot be tracked.
     */
    void propertyChanged (const char *publicName, const char *name);

private:

    void setPropertyPrivate (size_t index, const QString &value);
    void resetError() { last_err = QString::null; }

    QString last_err;
};

#endif // __VBoxGlobalSettings_h__
