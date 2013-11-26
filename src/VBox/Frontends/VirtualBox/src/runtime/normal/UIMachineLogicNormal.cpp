/* $Id: UIMachineLogicNormal.cpp $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIMachineLogicNormal class implementation
 */

/*
 * Copyright (C) 2010-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QMenu>

/* GUI includes: */
#include "VBoxGlobal.h"
#include "UIMessageCenter.h"
#include "UISession.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogicNormal.h"
#include "UIMachineWindow.h"
#include "UIDownloaderAdditions.h"
#include "UIDownloaderUserManual.h"
#include "UIDownloaderExtensionPack.h"
#ifdef Q_WS_MAC
#include "VBoxUtils.h"
#endif /* Q_WS_MAC */

UIMachineLogicNormal::UIMachineLogicNormal(QObject *pParent, UISession *pSession)
    : UIMachineLogic(pParent, pSession, UIVisualStateType_Normal)
{
}

bool UIMachineLogicNormal::checkAvailability()
{
    /* Normal mode is always available: */
    return true;
}

void UIMachineLogicNormal::sltPrepareNetworkAdaptersMenu()
{
    QMenu *menu = qobject_cast<QMenu*>(sender());
    AssertMsg(menu, ("This slot should be called only on Network Adapters menu show!\n"));
    menu->clear();
    menu->addAction(gActionPool->action(UIActionIndexRuntime_Simple_NetworkSettings));
}

void UIMachineLogicNormal::sltPrepareSharedFoldersMenu()
{
    QMenu *menu = qobject_cast<QMenu*>(sender());
    AssertMsg(menu, ("This slot should be called only on Shared Folders menu show!\n"));
    menu->clear();
    menu->addAction(gActionPool->action(UIActionIndexRuntime_Simple_SharedFoldersSettings));
}

void UIMachineLogicNormal::sltPrepareVideoCaptureMenu()
{
    QMenu *pMenu = qobject_cast<QMenu*>(sender());
    AssertMsg(pMenu, ("This slot should be called only on Video Capture menu show!\n"));
    pMenu->clear();
    pMenu->addAction(gActionPool->action(UIActionIndexRuntime_Toggle_VideoCapture));
    pMenu->addAction(gActionPool->action(UIActionIndexRuntime_Simple_VideoCaptureSettings));
}

void UIMachineLogicNormal::sltPrepareMouseIntegrationMenu()
{
    QMenu *menu = qobject_cast<QMenu*>(sender());
    AssertMsg(menu, ("This slot should be called only on Mouse Integration Menu show!\n"));
    menu->clear();
    menu->addAction(gActionPool->action(UIActionIndexRuntime_Toggle_MouseIntegration));
}

void UIMachineLogicNormal::prepareActionConnections()
{
    /* Base class connections: */
    UIMachineLogic::prepareActionConnections();

    /* This class connections: */
    connect(gActionPool->action(UIActionIndexRuntime_Menu_Network)->menu(), SIGNAL(aboutToShow()),
            this, SLOT(sltPrepareNetworkAdaptersMenu()));
    connect(gActionPool->action(UIActionIndexRuntime_Menu_SharedFolders)->menu(), SIGNAL(aboutToShow()),
            this, SLOT(sltPrepareSharedFoldersMenu()));
    connect(gActionPool->action(UIActionIndexRuntime_Menu_VideoCapture)->menu(), SIGNAL(aboutToShow()),
            this, SLOT(sltPrepareVideoCaptureMenu()));
    connect(gActionPool->action(UIActionIndexRuntime_Menu_MouseIntegration)->menu(), SIGNAL(aboutToShow()),
            this, SLOT(sltPrepareMouseIntegrationMenu()));
}

void UIMachineLogicNormal::prepareMachineWindows()
{
    /* Do not create machine-window(s) if they created already: */
    if (isMachineWindowsCreated())
        return;

#ifdef Q_WS_MAC // TODO: Is that really need here?
    /* We have to make sure that we are getting the front most process.
     * This is necessary for Qt versions > 4.3.3: */
    ::darwinSetFrontMostProcess();
#endif /* Q_WS_MAC */

    /* Get monitors count: */
    ulong uMonitorCount = session().GetMachine().GetMonitorCount();
    /* Create machine window(s): */
    for (ulong uScreenId = 0; uScreenId < uMonitorCount; ++ uScreenId)
        addMachineWindow(UIMachineWindow::create(this, uScreenId));
    /* Order machine window(s): */
    for (ulong uScreenId = uMonitorCount; uScreenId > 0; -- uScreenId)
        machineWindows()[uScreenId - 1]->raise();

    /* Mark machine-window(s) created: */
    setMachineWindowsCreated(true);
}

void UIMachineLogicNormal::cleanupMachineWindows()
{
    /* Do not destroy machine-window(s) if they destroyed already: */
    if (!isMachineWindowsCreated())
        return;

    /* Mark machine-window(s) destroyed: */
    setMachineWindowsCreated(false);

    /* Cleanup machine-window(s): */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        UIMachineWindow::destroy(pMachineWindow);
}

