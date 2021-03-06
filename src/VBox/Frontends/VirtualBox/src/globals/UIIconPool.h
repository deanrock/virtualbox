/* $Id: UIIconPool.h $ */
/** @file
 * VBox Qt GUI - UIIconPool class declaration.
 */

/*
 * Copyright (C) 2010-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIIconPool_h___
#define ___UIIconPool_h___

/* Qt includes: */
#include <QIcon>
#include <QPixmap>
#include <QHash>

/** Interface which provides GUI with static API
  * allowing to dynamically compose icons at runtime. */
class UIIconPool
{
public:

    /** Default icon types. */
    enum UIDefaultIconType
    {
        /* Message-box related stuff: */
        UIDefaultIconType_MessageBoxInformation,
        UIDefaultIconType_MessageBoxQuestion,
        UIDefaultIconType_MessageBoxWarning,
        UIDefaultIconType_MessageBoxCritical,
        /* Dialog related stuff: */
        UIDefaultIconType_DialogCancel,
        UIDefaultIconType_DialogHelp,
        UIDefaultIconType_ArrowBack,
        UIDefaultIconType_ArrowForward
    };

    /** Creates pixmap from passed pixmap @a strName. */
    static QPixmap pixmap(const QString &strName);

    /** Creates icon from passed pixmap names for
      * @a strNormal, @a strDisabled and @a strActive icon states. */
    static QIcon iconSet(const QString &strNormal,
                         const QString &strDisabled = QString(),
                         const QString &strActive = QString());

    /** Creates icon from passed pixmap names for
      * @a strNormal, @a strDisabled, @a strActive icon states and
      * their analogs for toggled-off case. Used for toggle actions. */
    static QIcon iconSetOnOff(const QString &strNormal, const QString strNormalOff,
                              const QString &strDisabled = QString(), const QString &strDisabledOff = QString(),
                              const QString &strActive = QString(), const QString &strActiveOff = QString());

    /** Creates icon from passed pixmap names for
      * @a strNormal, @a strDisabled, @a strActive icon states and
      * their analogs for small-icon case. Used for setting pages. */
    static QIcon iconSetFull(const QString &strNormal, const QString &strSmall,
                             const QString &strNormalDisabled = QString(), const QString &strSmallDisabled = QString(),
                             const QString &strNormalActive = QString(), const QString &strSmallActive = QString());

    /** Creates icon from passed pixmaps for
      * @a normal, @a disabled and @a active icon states. */
    static QIcon iconSet(const QPixmap &normal,
                         const QPixmap &disabled = QPixmap(),
                         const QPixmap &active = QPixmap());

    /** Creates icon of passed @a defaultIconType
      * based on passed @a pWidget style (if any) or application style (otherwise). */
    static QIcon defaultIcon(UIDefaultIconType defaultIconType, const QWidget *pWidget = 0);

protected:

    /** Icon-pool constructor. */
    UIIconPool() {};

    /** Icon-pool destructor. */
    virtual ~UIIconPool() {};

private:

    /** Adds resource named @a strName to passed @a icon
      * for @a mode (QIcon::Normal by default) and @a state (QIcon::Off by default). */
    static void addName(QIcon &icon, const QString &strName,
                        QIcon::Mode mode = QIcon::Normal, QIcon::State state = QIcon::Off);
};

/** UIIconPool interface extension used as general GUI icon-pool.
  * Provides GUI with guest OS types pixmap cache. */
class UIIconPoolGeneral : public UIIconPool
{
public:

    /** General icon-pool constructor. */
    UIIconPoolGeneral();

    /** Returns pixmap corresponding to passed @a strOSTypeID.
      * In case if non-null @a pLogicalSize pointer provided, it will be updated properly. */
    QPixmap guestOSTypeIcon(const QString &strOSTypeID, QSize *pLogicalSize = 0) const;

private:

    /** Guest OS type icon-names cache. */
    QHash<QString, QString> m_guestOSTypeIconNames;
    /** Guest OS type icons cache. */
    mutable QHash<QString, QIcon> m_guestOSTypeIcons;
};

#endif /* !___UIIconPool_h___ */
