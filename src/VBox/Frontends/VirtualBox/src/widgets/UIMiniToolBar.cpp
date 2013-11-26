/* $Id: UIMiniToolBar.cpp $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIMiniToolBar class implementation.
 * This is the toolbar shown in fullscreen/seamless modes.
 */

/*
 * Copyright (C) 2009-2013 Oracle Corporation
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
#include <QApplication>
#include <QTimer>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QDesktopWidget>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QStateMachine>
#include <QPainter>

/* GUI includes: */
#include "UIMiniToolBar.h"
#include "UIAnimationFramework.h"
#include "UIIconPool.h"
#include "VBoxGlobal.h"

#ifndef Q_WS_X11
# define VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR
#endif /* !Q_WS_X11 */

UIRuntimeMiniToolBar::UIRuntimeMiniToolBar(QWidget *pParent,
                                           Qt::Alignment alignment,
                                           IntegrationMode integrationMode,
                                           bool fAutoHide /* = true*/)
    : QWidget(pParent)
    /* Variables: General stuff: */
    , m_alignment(alignment)
    , m_integrationMode(integrationMode)
    , m_fAutoHide(fAutoHide)
    /* Variables: Contents stuff: */
    , m_pMdiArea(0)
    , m_pToolbar(0)
    , m_pEmbeddedToolbar(0)
    /* Variables: Hover stuff: */
    , m_fHovered(false)
    , m_pHoverEnterTimer(0)
    , m_pHoverLeaveTimer(0)
    , m_pAnimation(0)
{
    /* Prepare: */
    prepare();
}

UIRuntimeMiniToolBar::~UIRuntimeMiniToolBar()
{
    /* Cleanup: */
    cleanup();
}

void UIRuntimeMiniToolBar::setAlignment(Qt::Alignment alignment)
{
    /* Make sure alignment really changed: */
    if (m_alignment == alignment)
        return;

    /* Update alignment: */
    m_alignment = alignment;

    /* Re-initialize: */
    adjustGeometry();

    /* Propagate to child to update shape: */
    m_pToolbar->setAlignment(m_alignment);
}

void UIRuntimeMiniToolBar::setIntegrationMode(IntegrationMode integrationMode)
{
    /* Make sure integration-mode really changed: */
    if (m_integrationMode == integrationMode)
        return;

    /* Update integration-mode: */
    m_integrationMode = integrationMode;

    /* Re-integrate: */
    integrate();

    /* Re-initialize: */
    adjustGeometry();

    /* Propagate to child to update shape: */
    m_pToolbar->setIntegrationMode(m_integrationMode);
}

void UIRuntimeMiniToolBar::setAutoHide(bool fAutoHide, bool fPropagateToChild /* = true*/)
{
    /* Make sure auto-hide really changed: */
    if (m_fAutoHide == fAutoHide)
        return;

    /* Update auto-hide: */
    m_fAutoHide = fAutoHide;

    /* Re-initialize: */
    adjustGeometry();

    /* Propagate to child to update action if necessary: */
    if (fPropagateToChild)
        m_pToolbar->setAutoHide(m_fAutoHide);
}

void UIRuntimeMiniToolBar::setText(const QString &strText)
{
    /* Propagate to child: */
    m_pToolbar->setText(strText);
}

void UIRuntimeMiniToolBar::addMenus(const QList<QMenu*> &menus)
{
    /* Propagate to child: */
    m_pToolbar->addMenus(menus);
}

void UIRuntimeMiniToolBar::adjustGeometry()
{
    /* This method could be called before parent-widget
     * become visible, we should skip everything in that case: */
    if (QApplication::desktop()->screenNumber(parentWidget()) == -1)
        return;

    /* Reset toolbar geometry: */
    m_pEmbeddedToolbar->move(0, 0);
    m_pEmbeddedToolbar->resize(m_pEmbeddedToolbar->sizeHint());

    /* Adjust window geometry: */
    resize(m_pEmbeddedToolbar->size());
    QRect screenRect;
    int iX = 0, iY = 0;
    switch (m_integrationMode)
    {
        case IntegrationMode_Embedded:
        {
            /* Screen geometry: */
            screenRect = QApplication::desktop()->screenGeometry(parentWidget());
            /* Local coordinates, tool-bar is a child of the parent-widget: */
            iX = screenRect.width() / 2 - width() / 2;
            switch (m_alignment)
            {
                case Qt::AlignTop:
                    iY = 0;
                    break;
                case Qt::AlignBottom:
                    iY = screenRect.height() - height();
                    break;
            }
            break;
        }
        case IntegrationMode_External:
        {
            /* Available geometry: */
            screenRect = vboxGlobal().availableGeometry(QApplication::desktop()->screenNumber(parentWidget()));
            /* Global coordinates, tool-bar is tool-window aligned according the parent-widget: */
            iX = screenRect.x() + screenRect.width() / 2 - width() / 2;
            switch (m_alignment)
            {
                case Qt::AlignTop:
                    iY = screenRect.y();
                    break;
                case Qt::AlignBottom:
                    iY = screenRect.y() + screenRect.height() - height();
                    break;
            }
            break;
        }
    }
    move(iX, iY);

    /* Recalculate auto-hide animation: */
    updateAutoHideAnimationBounds();

    /* Simulate toolbar auto-hiding: */
    simulateToolbarAutoHiding();

    /* Due to [probably] Qt bug QMdiSubWindow still
     * can receive focus even if focus policy is Qt::NoFocus,
     * We should return the focus to our parent: */
    parentWidget()->setFocus();
}

void UIRuntimeMiniToolBar::sltHandleToolbarResize()
{
    /* Re-initialize: */
    adjustGeometry();
}

void UIRuntimeMiniToolBar::sltAutoHideToggled()
{
    /* Propagate from child: */
    setAutoHide(m_pToolbar->autoHide(), false);
}

void UIRuntimeMiniToolBar::sltHoverEnter()
{
    /* Mark as 'hovered' if necessary: */
    if (!m_fHovered)
    {
        m_fHovered = true;
        emit sigHoverEnter();
    }
}

void UIRuntimeMiniToolBar::sltHoverLeave()
{
    /* Mark as 'unhovered' if necessary: */
    if (m_fHovered)
    {
        m_fHovered = false;
        emit sigHoverLeave();
    }
}

void UIRuntimeMiniToolBar::prepare()
{
    /* Allow any size: */
    setMinimumSize(1, 1);
    /* Make sure we have no focus: */
    setFocusPolicy(Qt::NoFocus);

    /* Prepare mdi-area: */
    m_pMdiArea = new QMdiArea;
    {
        /* Configure own background: */
        QPalette pal = m_pMdiArea->palette();
        pal.setColor(QPalette::Window, QColor(Qt::transparent));
        m_pMdiArea->setPalette(pal);
        /* Configure viewport background: */
        m_pMdiArea->setBackground(QColor(Qt::transparent));
        /* Layout mdi-area according parent-widget: */
        QVBoxLayout *pMainLayout = new QVBoxLayout(this);
        pMainLayout->setContentsMargins(0, 0, 0, 0);
        pMainLayout->addWidget(m_pMdiArea);
        /* Make sure we have no focus: */
        m_pMdiArea->setFocusPolicy(Qt::NoFocus);
        m_pMdiArea->viewport()->setFocusPolicy(Qt::NoFocus);
    }

    /* Prepare mini-toolbar: */
    m_pToolbar = new UIMiniToolBar;
    {
        /* Propagate known options to child: */
        m_pToolbar->setAutoHide(m_fAutoHide);
        m_pToolbar->setAlignment(m_alignment);
        m_pToolbar->setIntegrationMode(m_integrationMode);
        /* Configure own background: */
        QPalette pal = m_pToolbar->palette();
        pal.setColor(QPalette::Window, palette().color(QPalette::Window));
        m_pToolbar->setPalette(pal);
        /* Configure child connections: */
        connect(m_pToolbar, SIGNAL(sigResized()), this, SLOT(sltHandleToolbarResize()));
        connect(m_pToolbar, SIGNAL(sigAutoHideToggled()), this, SLOT(sltAutoHideToggled()));
        connect(m_pToolbar, SIGNAL(sigMinimizeAction()), this, SIGNAL(sigMinimizeAction()));
        connect(m_pToolbar, SIGNAL(sigExitAction()), this, SIGNAL(sigExitAction()));
        connect(m_pToolbar, SIGNAL(sigCloseAction()), this, SIGNAL(sigCloseAction()));
        /* Add child to mdi-area: */
        m_pEmbeddedToolbar = m_pMdiArea->addSubWindow(m_pToolbar, Qt::Window | Qt::FramelessWindowHint);
        /* Make sure we have no focus: */
        m_pToolbar->setFocusPolicy(Qt::NoFocus);
        m_pEmbeddedToolbar->setFocusPolicy(Qt::NoFocus);
    }

    /* Prepare hover-enter/leave timers: */
    m_pHoverEnterTimer = new QTimer(this);
    {
        m_pHoverEnterTimer->setSingleShot(true);
        m_pHoverEnterTimer->setInterval(50);
        connect(m_pHoverEnterTimer, SIGNAL(timeout()), this, SLOT(sltHoverEnter()));
    }
    m_pHoverLeaveTimer = new QTimer(this);
    {
        m_pHoverLeaveTimer->setSingleShot(true);
        m_pHoverLeaveTimer->setInterval(500);
        connect(m_pHoverLeaveTimer, SIGNAL(timeout()), this, SLOT(sltHoverLeave()));
    }

    /* Install 'auto-hide' animation to 'toolbarPosition' property: */
    m_pAnimation = UIAnimation::installPropertyAnimation(this,
                                                         "toolbarPosition",
                                                         "hiddenToolbarPosition", "shownToolbarPosition",
                                                         SIGNAL(sigHoverEnter()), SIGNAL(sigHoverLeave()),
                                                         true);

    /* Integrate if necessary: */
    integrate();

    /* Adjust geometry finally: */
    adjustGeometry();
}

void UIRuntimeMiniToolBar::cleanup()
{
    /* Stop hover-enter/leave timers: */
    if (m_pHoverEnterTimer->isActive())
        m_pHoverEnterTimer->stop();
    if (m_pHoverLeaveTimer->isActive())
        m_pHoverLeaveTimer->stop();
}

void UIRuntimeMiniToolBar::enterEvent(QEvent*)
{
    /* Stop the hover-leave timer if necessary: */
    if (m_pHoverLeaveTimer->isActive())
        m_pHoverLeaveTimer->stop();

    /* Start the hover-enter timer: */
    if (m_fAutoHide)
        m_pHoverEnterTimer->start();
}

void UIRuntimeMiniToolBar::leaveEvent(QEvent*)
{
    /* Stop the hover-enter timer if necessary: */
    if (m_pHoverEnterTimer->isActive())
        m_pHoverEnterTimer->stop();

    /* Start the hover-leave timer: */
    if (m_fAutoHide)
        m_pHoverLeaveTimer->start();
}

void UIRuntimeMiniToolBar::updateAutoHideAnimationBounds()
{
    /* Update animation: */
    m_shownToolbarPosition = m_pEmbeddedToolbar->pos();
    switch (m_alignment)
    {
        case Qt::AlignTop:
            m_hiddenToolbarPosition = m_shownToolbarPosition - QPoint(0, m_pEmbeddedToolbar->height() - 3);
            break;
        case Qt::AlignBottom:
            m_hiddenToolbarPosition = m_shownToolbarPosition + QPoint(0, m_pEmbeddedToolbar->height() - 3);
            break;
    }
    m_pAnimation->update();
}

void UIRuntimeMiniToolBar::simulateToolbarAutoHiding()
{
    /* This simulation helps user to notice
     * toolbar location, so it will be used only
     * 1. if toolbar unhovered and
     * 2. auto-hide feature enabled: */
    if (m_fHovered || !m_fAutoHide)
        return;

    /* Simulate hover-leave event: */
    m_fHovered = true;
    m_pHoverLeaveTimer->start();
}

void UIRuntimeMiniToolBar::setToolbarPosition(QPoint point)
{
    /* Make sure toolbar exists: */
    if (!m_pEmbeddedToolbar)
        return;

    /* Update position: */
    m_pEmbeddedToolbar->move(point);

#ifdef Q_WS_X11
    /* The setMask functionality is excessive under Win/Mac hosts
     * because there is a Qt composition works properly,
     * Mac host has native translucency support,
     * Win host allows to enable it through Qt::WA_TranslucentBackground: */
    setMask(m_pEmbeddedToolbar->geometry());
#endif /* Q_WS_X11 */
}

QPoint UIRuntimeMiniToolBar::toolbarPosition() const
{
    /* Make sure toolbar exists: */
    if (!m_pEmbeddedToolbar)
        return QPoint();

    /* Return position: */
    return m_pEmbeddedToolbar->pos();
}

void UIRuntimeMiniToolBar::integrate()
{
    /* Reintegrate if necessary: */
    if (m_integrationMode == IntegrationMode_Embedded && isWindow())
    {
        setWindowFlags(Qt::Widget);
#ifdef VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR
        setAttribute(Qt::WA_TranslucentBackground, false);
#endif /* VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR */
        show();
    }
    else if (m_integrationMode == IntegrationMode_External && !isWindow())
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
#ifdef VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR
        setAttribute(Qt::WA_TranslucentBackground, true);
#endif /* VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR */
        show();
    }
}


UIMiniToolBar::UIMiniToolBar()
    /* Variables: General stuff: */
    : m_fPolished(false)
    , m_alignment(Qt::AlignBottom)
    /* Variables: Contents stuff: */
    , m_pAutoHideAction(0)
    , m_pLabel(0)
    , m_pMinimizeAction(0)
    , m_pRestoreAction(0)
    , m_pCloseAction(0)
    /* Variables: Menu stuff: */
    , m_pMenuInsertPosition(0)
{
    /* Prepare: */
    prepare();
}

void UIMiniToolBar::setAlignment(Qt::Alignment alignment)
{
    /* Make sure alignment really changed: */
    if (m_alignment == alignment)
        return;

    /* Update alignment: */
    m_alignment = alignment;

    /* Rebuild shape: */
    rebuildShape();
}

void UIMiniToolBar::setIntegrationMode(IntegrationMode integrationMode)
{
    /* Make sure integration-mode really changed: */
    if (m_integrationMode == integrationMode)
        return;

    /* Update integration-mode: */
    m_integrationMode = integrationMode;

    /* Rebuild shape: */
    rebuildShape();
}

bool UIMiniToolBar::autoHide() const
{
    /* Return auto-hide: */
    return !m_pAutoHideAction->isChecked();
}

void UIMiniToolBar::setAutoHide(bool fAutoHide)
{
    /* Make sure auto-hide really changed: */
    if (m_pAutoHideAction->isChecked() == !fAutoHide)
        return;

    /* Update auto-hide: */
    m_pAutoHideAction->setChecked(!fAutoHide);
}

void UIMiniToolBar::setText(const QString &strText)
{
    /* Make sure text really changed: */
    if (m_pLabel->text() == strText)
        return;

    /* Update text: */
    m_pLabel->setText(strText);

    /* Resize to sizehint: */
    resize(sizeHint());
}

void UIMiniToolBar::addMenus(const QList<QMenu*> &menus)
{
    /* For each of the passed menu items: */
    for (int i = 0; i < menus.size(); ++i)
    {
        /* Get corresponding menu-action: */
        QAction *pAction = menus[i]->menuAction();
        /* Insert it into corresponding place: */
        insertAction(m_pMenuInsertPosition, pAction);
        /* Configure corresponding tool-button: */
        if (QToolButton *pButton = qobject_cast<QToolButton*>(widgetForAction(pAction)))
        {
            pButton->setPopupMode(QToolButton::InstantPopup);
            pButton->setAutoRaise(true);
        }
        /* Add some spacing: */
        if (i != menus.size() - 1)
            m_spacings << widgetForAction(insertWidget(m_pMenuInsertPosition, new QWidget(this)));
    }

    /* Resize to sizehint: */
    resize(sizeHint());
}

void UIMiniToolBar::showEvent(QShowEvent *pEvent)
{
    /* Make sure we should polish dialog: */
    if (m_fPolished)
        return;

    /* Call to polish-event: */
    polishEvent(pEvent);

    /* Mark dialog as polished: */
    m_fPolished = true;
}

void UIMiniToolBar::polishEvent(QShowEvent*)
{
    /* Toolbar spacings: */
    foreach(QWidget *pSpacing, m_spacings)
        pSpacing->setMinimumWidth(5);

    /* Title spacings: */
    foreach(QWidget *pLableMargin, m_margins)
        pLableMargin->setMinimumWidth(15);

    /* Resize to sizehint: */
    resize(sizeHint());
}

void UIMiniToolBar::resizeEvent(QResizeEvent*)
{
    /* Rebuild shape: */
    rebuildShape();

    /* Notify listeners: */
    emit sigResized();
}

void UIMiniToolBar::paintEvent(QPaintEvent*)
{
    /* Prepare painter: */
    QPainter painter(this);

    /* Fill background: */
    if (!m_shape.isEmpty())
    {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setClipPath(m_shape);
    }
    QRect backgroundRect = rect();
    QColor backgroundColor = palette().color(QPalette::Window);
    QLinearGradient headerGradient(backgroundRect.bottomLeft(), backgroundRect.topLeft());
    headerGradient.setColorAt(0, backgroundColor.darker(120));
    headerGradient.setColorAt(1, backgroundColor.darker(90));
    painter.fillRect(backgroundRect, headerGradient);
}

void UIMiniToolBar::prepare()
{
    /* Configure toolbar: */
    setIconSize(QSize(16, 16));

#ifdef VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR
    /* Left margin: */
    m_spacings << widgetForAction(addWidget(new QWidget));
#endif /* VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR */

    /* Prepare push-pin: */
    m_pAutoHideAction = new QAction(this);
    m_pAutoHideAction->setIcon(UIIconPool::iconSet(":/pin_16px.png"));
    m_pAutoHideAction->setToolTip(tr("Always show the toolbar"));
    m_pAutoHideAction->setCheckable(true);
    connect(m_pAutoHideAction, SIGNAL(toggled(bool)), this, SIGNAL(sigAutoHideToggled()));
    addAction(m_pAutoHideAction);

    /* Left menu margin: */
    m_spacings << widgetForAction(addWidget(new QWidget));

    /* Right menu margin: */
    m_pMenuInsertPosition = addWidget(new QWidget);
    m_spacings << widgetForAction(m_pMenuInsertPosition);

    /* Left label margin: */
    m_margins << widgetForAction(addWidget(new QWidget));

    /* Insert a label for VM Name: */
    m_pLabel = new QLabel;
    m_pLabel->setAlignment(Qt::AlignCenter);
    addWidget(m_pLabel);

    /* Right label margin: */
    m_margins << widgetForAction(addWidget(new QWidget));

    /* Minimize action: */
    m_pMinimizeAction = new QAction(this);
    m_pMinimizeAction->setIcon(UIIconPool::iconSet(":/minimize_16px.png"));
    m_pMinimizeAction->setToolTip(tr("Minimize Window"));
    connect(m_pMinimizeAction, SIGNAL(triggered()), this, SIGNAL(sigMinimizeAction()));
    addAction(m_pMinimizeAction);

    /* Exit action: */
    m_pRestoreAction = new QAction(this);
    m_pRestoreAction->setIcon(UIIconPool::iconSet(":/restore_16px.png"));
    m_pRestoreAction->setToolTip(tr("Exit Full Screen or Seamless Mode"));
    connect(m_pRestoreAction, SIGNAL(triggered()), this, SIGNAL(sigExitAction()));
    addAction(m_pRestoreAction);

    /* Close action: */
    m_pCloseAction = new QAction(this);
    m_pCloseAction->setIcon(UIIconPool::iconSet(":/close_16px.png"));
    m_pCloseAction->setToolTip(tr("Close VM"));
    connect(m_pCloseAction, SIGNAL(triggered()), this, SIGNAL(sigCloseAction()));
    addAction(m_pCloseAction);

#ifdef VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR
    /* Right margin: */
    m_spacings << widgetForAction(addWidget(new QWidget));
#endif /* VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR */

    /* Resize to sizehint: */
    resize(sizeHint());
}

void UIMiniToolBar::rebuildShape()
{
#ifdef VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR
    /* Rebuild shape: */
    QPainterPath shape;
    switch (m_alignment)
    {
        case Qt::AlignTop:
        {
            shape.moveTo(0, 0);
            shape.lineTo(shape.currentPosition().x(), height() - 10);
            shape.arcTo(QRectF(shape.currentPosition(), QSizeF(20, 20)).translated(0, -10), 180, 90);
            shape.lineTo(width() - 10, shape.currentPosition().y());
            shape.arcTo(QRectF(shape.currentPosition(), QSizeF(20, 20)).translated(-10, -20), 270, 90);
            shape.lineTo(shape.currentPosition().x(), 0);
            shape.closeSubpath();
            break;
        }
        case Qt::AlignBottom:
        {
            shape.moveTo(0, height());
            shape.lineTo(shape.currentPosition().x(), 10);
            shape.arcTo(QRectF(shape.currentPosition(), QSizeF(20, 20)).translated(0, -10), 180, -90);
            shape.lineTo(width() - 10, shape.currentPosition().y());
            shape.arcTo(QRectF(shape.currentPosition(), QSizeF(20, 20)).translated(-10, 0), 90, -90);
            shape.lineTo(shape.currentPosition().x(), height());
            shape.closeSubpath();
            break;
        }
        default:
            break;
    }
    m_shape = shape;

    /* Update: */
    update();
#endif /* VBOX_RUNTIME_UI_WITH_SHAPED_MINI_TOOLBAR */
}

