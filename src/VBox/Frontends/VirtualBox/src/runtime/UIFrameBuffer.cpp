/* $Id: UIFrameBuffer.cpp $ */
/** @file
 * VBox Qt GUI - UIFrameBuffer class implementation.
 */

/*
 * Copyright (C) 2010-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include "precomp.h"
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* GUI includes: */
# include "UIMachineView.h"
# include "UIFrameBuffer.h"
# include "UIMessageCenter.h"
# include "VBoxGlobal.h"
# ifndef VBOX_WITH_TRANSLUCENT_SEAMLESS
#  include "UIMachineWindow.h"
# endif /* !VBOX_WITH_TRANSLUCENT_SEAMLESS */

/* Other VBox includes: */
# include <VBox/VBoxVideo3D.h>

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

#if defined (Q_OS_WIN32)
static CComModule _Module;
#else
NS_DECL_CLASSINFO (UIFrameBuffer)
NS_IMPL_THREADSAFE_ISUPPORTS1_CI (UIFrameBuffer, IFramebuffer)
#endif

UIFrameBuffer::UIFrameBuffer(UIMachineView *pMachineView)
    : m_pMachineView(pMachineView)
    , m_width(0), m_height(0)
    , m_fIsMarkedAsUnused(false)
    , m_fIsAutoEnabled(false)
#ifdef Q_OS_WIN
    , m_iRefCnt(0)
#endif /* Q_OS_WIN */
{
    /* Assign mahine-view: */
    AssertMsg(m_pMachineView, ("UIMachineView must not be NULL\n"));
    m_WinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

    /* Initialize critical-section: */
    int rc = RTCritSectInit(&m_critSect);
    AssertRC(rc);

    /* Connect handlers: */
    if (m_pMachineView)
        prepareConnections();
}

UIFrameBuffer::~UIFrameBuffer()
{
    /* Disconnect handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Deinitialize critical-section: */
    RTCritSectDelete(&m_critSect);
}

/**
 * Sets the framebuffer <b>unused</b> status.
 * @param fIsMarkAsUnused determines whether framebuffer should ignore EMT events or not.
 * @note  Call to this (and any EMT callback) method is synchronized between calling threads (from GUI side).
 */
void UIFrameBuffer::setMarkAsUnused(bool fIsMarkAsUnused)
{
    lock();
    m_fIsMarkedAsUnused = fIsMarkAsUnused;
    unlock();
}

/**
 * Returns the framebuffer <b>auto-enabled</b> status.
 * @returns @c true if guest-screen corresponding to this framebuffer was automatically enabled by
            the auto-mount guest-screen auto-pilot, @c false otherwise.
 * @note    <i>Auto-enabled</i> status means the framebuffer was automatically enabled by the multi-screen layout
 *          and so have potentially incorrect guest size hint posted into guest event queue. Machine-view will try to
 *          automatically adjust guest-screen size as soon as possible.
 */
bool UIFrameBuffer::isAutoEnabled() const
{
    return m_fIsAutoEnabled;
}

/**
 * Sets the framebuffer <b>auto-enabled</b> status.
 * @param fIsAutoEnabled determines whether guest-screen corresponding to this framebuffer
 *        was automatically enabled by the auto-mount guest-screen auto-pilot.
 * @note  <i>Auto-enabled</i> status means the framebuffer was automatically enabled by the multi-screen layout
 *        and so have potentially incorrect guest size hint posted into guest event queue. Machine-view will try to
 *        automatically adjust guest-screen size as soon as possible.
 */
void UIFrameBuffer::setAutoEnabled(bool fIsAutoEnabled)
{
    m_fIsAutoEnabled = fIsAutoEnabled;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Address) (BYTE **ppAddress)
{
    if (!ppAddress)
        return E_POINTER;
    *ppAddress = address();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Width) (ULONG *puWidth)
{
    if (!puWidth)
        return E_POINTER;
    *puWidth = (ULONG)width();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Height) (ULONG *puHeight)
{
    if (!puHeight)
        return E_POINTER;
    *puHeight = (ULONG)height();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(BitsPerPixel) (ULONG *puBitsPerPixel)
{
    if (!puBitsPerPixel)
        return E_POINTER;
    *puBitsPerPixel = bitsPerPixel();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(BytesPerLine) (ULONG *puBytesPerLine)
{
    if (!puBytesPerLine)
        return E_POINTER;
    *puBytesPerLine = bytesPerLine();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(PixelFormat) (ULONG *puPixelFormat)
{
    if (!puPixelFormat)
        return E_POINTER;
    *puPixelFormat = pixelFormat();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(UsesGuestVRAM) (BOOL *pbUsesGuestVRAM)
{
    if (!pbUsesGuestVRAM)
        return E_POINTER;
    *pbUsesGuestVRAM = usesGuestVRAM();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(HeightReduction) (ULONG *puHeightReduction)
{
    if (!puHeightReduction)
        return E_POINTER;
    *puHeightReduction = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(Overlay) (IFramebufferOverlay **ppOverlay)
{
    if (!ppOverlay)
        return E_POINTER;
    /* not yet implemented */
    *ppOverlay = 0;
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::COMGETTER(WinId) (LONG64 *pWinId)
{
    if (!pWinId)
        return E_POINTER;
    *pWinId = m_WinId;
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::Lock()
{
    this->lock();
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::Unlock()
{
    this->unlock();
    return S_OK;
}

/**
 * EMT callback: Requests a size and pixel format change.
 * @param uScreenId     Guest screen number. Must be used in the corresponding call to CDisplay::ResizeCompleted if this call is made.
 * @param uPixelFormat  Pixel format of the memory buffer pointed to by @a pVRAM.
 * @param pVRAM         Pointer to the virtual video card's VRAM (may be <i>null</i>).
 * @param uBitsPerPixel Color depth, bits per pixel.
 * @param uBytesPerLine Size of one scan line, in bytes.
 * @param uWidth        Width of the guest display, in pixels.
 * @param uHeight       Height of the guest display, in pixels.
 * @param pfFinished    Can the VM start using the new frame buffer immediately after this method returns or it should wait for CDisplay::ResizeCompleted.
 * @note  Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
 * @note  Calls to this and #setMarkAsUnused method are synchronized between calling threads (from GUI side).
 */
STDMETHODIMP UIFrameBuffer::RequestResize(ULONG uScreenId, ULONG uPixelFormat,
                                          BYTE *pVRAM, ULONG uBitsPerPixel, ULONG uBytesPerLine,
                                          ULONG uWidth, ULONG uHeight,
                                          BOOL *pfFinished)
{
    LogRelFlow(("UIFrameBuffer::RequestResize: "
                "Screen=%lu, Format=%lu, "
                "BitsPerPixel=%lu, BytesPerLine=%lu, "
                "Size=%lux%lu\n",
                (unsigned long)uScreenId, (unsigned long)uPixelFormat,
                (unsigned long)uBitsPerPixel, (unsigned long)uBytesPerLine,
                (unsigned long)uWidth, (unsigned long)uHeight));

    /* Make sure result pointer is valid: */
    if (!pfFinished)
    {
        LogRelFlow(("UIFrameBuffer::RequestResize: Invalid pfFinished pointer!\n"));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fIsMarkedAsUnused)
    {
        LogRelFlow(("UIFrameBuffer::RequestResize: Ignored!\n"));

        /* Mark request as finished.
         * It is required to report to the VM thread that we finished resizing and rely on the
         * later synchronisation when the new view is attached. */
        *pfFinished = TRUE;

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore RequestResize: */
        return E_FAIL;
    }

    /* Mark request as not-yet-finished: */
    *pfFinished = FALSE;

    /* Widget resize is NOT thread-safe and *probably* never will be,
     * We have to notify machine-view with the async-signal to perform resize operation. */
    LogRelFlow(("UIFrameBuffer::RequestResize: Sending to async-handler...\n"));
    emit sigRequestResize(uPixelFormat, pVRAM, uBitsPerPixel, uBytesPerLine, uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm RequestResize: */
    return S_OK;
}

/**
 * EMT callback: Informs about an update.
 * @param uX      Horizontal origin of the update rectangle, in pixels.
 * @param uY      Vertical origin of the update rectangle, in pixels.
 * @param uWidth  Width of the update rectangle, in pixels.
 * @param uHeight Height of the update rectangle, in pixels.
 * @note  Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
 * @note  Calls to this and #setMarkAsUnused method are synchronized between calling threads (from GUI side).
 */
STDMETHODIMP UIFrameBuffer::NotifyUpdate(ULONG uX, ULONG uY, ULONG uWidth, ULONG uHeight)
{
    LogRel2(("UIFrameBuffer::NotifyUpdate: Origin=%lux%lu, Size=%lux%lu\n",
             (unsigned long)uX, (unsigned long)uY,
             (unsigned long)uWidth, (unsigned long)uHeight));

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fIsMarkedAsUnused)
    {
        LogRel2(("UIFrameBuffer::NotifyUpdate: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore NotifyUpdate: */
        return E_FAIL;
    }

    /* Widget update is NOT thread-safe and *seems* never will be,
     * We have to notify machine-view with the async-signal to perform update operation. */
    LogRel2(("UIFrameBuffer::NotifyUpdate: Sending to async-handler...\n"));
    emit sigNotifyUpdate(uX, uY, uWidth, uHeight);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm NotifyUpdate: */
    return S_OK;
}

/**
 * EMT callback: Returns whether the framebuffer implementation is willing to support a given video mode.
 * @param uWidth      Width of the guest display, in pixels.
 * @param uHeight     Height of the guest display, in pixels.
 * @param uBPP        Color depth, bits per pixel.
 * @param pfSupported Is framebuffer able/willing to render the video mode or not.
 * @note  Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
 * @note  Calls to this and #setMarkAsUnused method are synchronized between calling threads (from GUI side).
 */
STDMETHODIMP UIFrameBuffer::VideoModeSupported(ULONG uWidth, ULONG uHeight, ULONG uBPP, BOOL *pfSupported)
{
    LogRel2(("UIFrameBuffer::IsVideoModeSupported: Mode: BPP=%lu, Size=%lux%lu\n",
             (unsigned long)uBPP, (unsigned long)uWidth, (unsigned long)uHeight));

    /* Make sure result pointer is valid: */
    if (!pfSupported)
    {
        LogRel2(("UIFrameBuffer::IsVideoModeSupported: Invalid pfSupported pointer!\n"));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fIsMarkedAsUnused)
    {
        LogRel2(("UIFrameBuffer::IsVideoModeSupported: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore VideoModeSupported: */
        return E_FAIL;
    }

    /* Determine if supported: */
    *pfSupported = TRUE;
    QSize screenSize = m_pMachineView->maxGuestSize();
    if (   (screenSize.width() != 0)
        && (uWidth > (ULONG)screenSize.width())
        && (uWidth > (ULONG)width()))
        *pfSupported = FALSE;
    if (   (screenSize.height() != 0)
        && (uHeight > (ULONG)screenSize.height())
        && (uHeight > (ULONG)height()))
        *pfSupported = FALSE;

    LogRel2(("UIFrameBuffer::IsVideoModeSupported: %s\n", *pfSupported ? "TRUE" : "FALSE"));

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm VideoModeSupported: */
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::GetVisibleRegion(BYTE *pRectangles, ULONG uCount, ULONG *puCountCopied)
{
    PRTRECT rects = (PRTRECT)pRectangles;

    if (!rects)
        return E_POINTER;

    Q_UNUSED(uCount);
    Q_UNUSED(puCountCopied);

    return S_OK;
}

/**
 * EMT callback: Suggests a new visible region to this framebuffer.
 * @param pRectangles Pointer to the RTRECT array.
 * @param uCount      Number of RTRECT elements in the rectangles array.
 * @note  Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
 * @note  Calls to this and #setMarkAsUnused method are synchronized between calling threads (from GUI side).
 */
STDMETHODIMP UIFrameBuffer::SetVisibleRegion(BYTE *pRectangles, ULONG uCount)
{
    LogRel2(("UIFrameBuffer::SetVisibleRegion: Rectangle count=%lu\n",
             (unsigned long)uCount));

    /* Make sure rectangles were passed: */
    if (!pRectangles)
    {
        LogRel2(("UIFrameBuffer::SetVisibleRegion: Invalid pRectangles pointer!\n"));

        return E_POINTER;
    }

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fIsMarkedAsUnused)
    {
        LogRel2(("UIFrameBuffer::SetVisibleRegion: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore SetVisibleRegion: */
        return E_FAIL;
    }

    /* Compose region: */
    QRegion region;
    PRTRECT rects = (PRTRECT)pRectangles;
    for (ULONG ind = 0; ind < uCount; ++ind)
    {
        /* Get current rectangle: */
        QRect rect;
        rect.setLeft(rects->xLeft);
        rect.setTop(rects->yTop);
        /* Which is inclusive: */
        rect.setRight(rects->xRight - 1);
        rect.setBottom(rects->yBottom - 1);
        /* Append region: */
        region += rect;
        ++rects;
    }

    /* We are directly updating synchronous visible-region: */
    m_syncVisibleRegion = region;
    /* And send async-signal to update asynchronous one: */
    LogRel2(("UIFrameBuffer::SetVisibleRegion: Sending to async-handler...\n"));
    emit sigSetVisibleRegion(region);

    /* Unlock access to frame-buffer: */
    unlock();

    /* Confirm SetVisibleRegion: */
    return S_OK;
}

STDMETHODIMP UIFrameBuffer::ProcessVHWACommand(BYTE *pCommand)
{
    Q_UNUSED(pCommand);
    return E_NOTIMPL;
}

/**
 * EMT callback: Notifies framebuffer about 3D backend event.
 * @param uType Event type. Currently only VBOX3D_NOTIFY_EVENT_TYPE_VISIBLE_3DDATA is supported.
 * @param pData Event-specific data, depends on the supplied event type.
 * @note  Any EMT callback is subsequent. No any other EMT callback can be called until this one processed.
 * @note  Calls to this and #setMarkAsUnused method are synchronized between calling threads (from GUI side).
 */
STDMETHODIMP UIFrameBuffer::Notify3DEvent(ULONG uType, BYTE *pData)
{
    LogRel2(("UIFrameBuffer::Notify3DEvent\n"));

    /* Lock access to frame-buffer: */
    lock();

    /* Make sure frame-buffer is used: */
    if (m_fIsMarkedAsUnused)
    {
        LogRel2(("UIFrameBuffer::Notify3DEvent: Ignored!\n"));

        /* Unlock access to frame-buffer: */
        unlock();

        /* Ignore Notify3DEvent: */
        return E_FAIL;
    }

    switch (uType)
    {
        case VBOX3D_NOTIFY_EVENT_TYPE_VISIBLE_3DDATA:
        {
            /* Notify machine-view with the async-signal
             * about 3D overlay visibility change: */
            BOOL fVisible = !!pData;
            LogRel2(("UIFrameBuffer::Notify3DEvent: Sending to async-handler: "
                     "(VBOX3D_NOTIFY_EVENT_TYPE_VISIBLE_3DDATA = %s)\n",
                     fVisible ? "TRUE" : "FALSE"));
            emit sigNotifyAbout3DOverlayVisibilityChange(fVisible);

            /* Unlock access to frame-buffer: */
            unlock();

            /* Confirm Notify3DEvent: */
            return S_OK;
        }
        default:
            break;
    }

    /* Unlock access to frame-buffer: */
    unlock();

    /* Ignore Notify3DEvent: */
    return E_INVALIDARG;
}

void UIFrameBuffer::applyVisibleRegion(const QRegion &region)
{
    /* Make sure async visible-region has changed: */
    if (m_asyncVisibleRegion == region)
        return;

    /* We are accounting async visible-regions one-by-one
     * to keep corresponding viewport area always updated! */
    if (!m_asyncVisibleRegion.isEmpty())
        m_pMachineView->viewport()->update(m_asyncVisibleRegion - region);

    /* Remember last visible region: */
    m_asyncVisibleRegion = region;

#ifndef VBOX_WITH_TRANSLUCENT_SEAMLESS
    /* We have to use async visible-region to apply to [Q]Widget [set]Mask: */
    m_pMachineView->machineWindow()->setMask(m_asyncVisibleRegion);
#endif /* !VBOX_WITH_TRANSLUCENT_SEAMLESS */
}

#ifdef VBOX_WITH_VIDEOHWACCEL
void UIFrameBuffer::doProcessVHWACommand(QEvent *pEvent)
{
    Q_UNUSED(pEvent);
    /* should never be here */
    AssertBreakpoint();
}
#endif /* VBOX_WITH_VIDEOHWACCEL */

void UIFrameBuffer::setView(UIMachineView * pView)
{
    /* Disconnect handlers: */
    if (m_pMachineView)
        cleanupConnections();

    /* Reassign machine-view: */
    m_pMachineView = pView;
    m_WinId = (m_pMachineView && m_pMachineView->viewport()) ? (LONG64)m_pMachineView->viewport()->winId() : 0;

    /* Connect handlers: */
    if (m_pMachineView)
        prepareConnections();
}

void UIFrameBuffer::prepareConnections()
{
    connect(this, SIGNAL(sigRequestResize(int, uchar*, int, int, int, int)),
            m_pMachineView, SLOT(sltHandleRequestResize(int, uchar*, int, int, int, int)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigNotifyUpdate(int, int, int, int)),
            m_pMachineView, SLOT(sltHandleNotifyUpdate(int, int, int, int)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigSetVisibleRegion(QRegion)),
            m_pMachineView, SLOT(sltHandleSetVisibleRegion(QRegion)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(sigNotifyAbout3DOverlayVisibilityChange(bool)),
            m_pMachineView, SLOT(sltHandle3DOverlayVisibilityChange(bool)),
            Qt::QueuedConnection);
}

void UIFrameBuffer::cleanupConnections()
{
    disconnect(this, SIGNAL(sigRequestResize(int, uchar*, int, int, int, int)),
               m_pMachineView, SLOT(sltHandleRequestResize(int, uchar*, int, int, int, int)));
    disconnect(this, SIGNAL(sigNotifyUpdate(int, int, int, int)),
               m_pMachineView, SLOT(sltHandleNotifyUpdate(int, int, int, int)));
    disconnect(this, SIGNAL(sigSetVisibleRegion(QRegion)),
               m_pMachineView, SLOT(sltHandleSetVisibleRegion(QRegion)));
    disconnect(this, SIGNAL(sigNotifyAbout3DOverlayVisibilityChange(bool)),
               m_pMachineView, SLOT(sltHandle3DOverlayVisibilityChange(bool)));
}

