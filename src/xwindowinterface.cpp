/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     rekols <revenmartin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xwindowinterface.h"
#include "utils.h"

#include <QTimer>
#include <QDebug>
#include <QWindow>
#include <QScreen>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

// KF6
#include <KWindowEffects>
#include <KWindowSystem>
#include <KWindowInfo>
#include <KX11Extras>

static XWindowInterface *INSTANCE = nullptr;

XWindowInterface *XWindowInterface::instance()
{
    if (!INSTANCE)
        INSTANCE = new XWindowInterface;
    return INSTANCE;
}

XWindowInterface::XWindowInterface(QObject *parent)
    : QObject(parent)
{
    // KDE6 中使用字符串信号名连接
    connect(KWindowSystem::self(), SIGNAL(windowAdded(WId)), this, SLOT(handleWindowAdded(WId)));
    connect(KWindowSystem::self(), SIGNAL(windowRemoved(WId)), this, SLOT(handleWindowRemoved(WId)));
    connect(KWindowSystem::self(), SIGNAL(activeWindowChanged(WId)), this, SLOT(handleActiveWindowChanged(WId)));
}

void XWindowInterface::enableBlurBehind(QWindow *view, bool enable, const QRegion &region)
{
    // KDE6 中直接传递 QWindow*
    KWindowEffects::enableBlurBehind(view, enable, region);
}

WId XWindowInterface::activeWindow()
{
    return KX11Extras::activeWindow();
}

void XWindowInterface::minimizeWindow(WId win)
{
    KX11Extras::minimizeWindow(win);
}

void XWindowInterface::closeWindow(WId id)
{
    // FIXME: Why there is no such thing in KWindowSystem??
    QWindow* window = QWindow::fromWinId(id); // 根据 WId 获取 QWindow
    if (window) {
        window->close(); // 请求关闭窗口
        // 或者，如果需要更强制性的方式，可以销毁窗口
        // window->destroy(); 
    }
}

void XWindowInterface::forceActiveWindow(WId win)
{
    KX11Extras::forceActiveWindow(win);
}

QMap<QString, QVariant> XWindowInterface::requestInfo(quint64 wid)
{
    const KWindowInfo winfo(wid, 
        NET::WMFrameExtents |
        NET::WMWindowType |
        NET::WMGeometry |
        NET::WMDesktop |
        NET::WMState |
        NET::WMName |
        NET::WMVisibleName,
        NET::WM2WindowClass |
        NET::WM2Activities |
        NET::WM2AllowedActions |
        NET::WM2TransientFor);
        
    QMap<QString, QVariant> result;
    const QString winClass = QString(winfo.windowClassClass());

    result.insert("iconName", winClass.toLower());
    result.insert("active", wid == KX11Extras::activeWindow());
    result.insert("visibleName", winfo.visibleName());
    result.insert("id", winClass);

    return result;
}

QString XWindowInterface::requestWindowClass(quint64 wid)
{
    KWindowInfo info(wid, NET::Supported, NET::WM2WindowClass);
    return QString(info.windowClassClass());
}

bool XWindowInterface::isAcceptableWindow(quint64 wid)
{
    QFlags<NET::WindowTypeMask> ignoreList;
    ignoreList |= NET::DesktopMask;
    ignoreList |= NET::DockMask;
    ignoreList |= NET::SplashMask;
    ignoreList |= NET::ToolbarMask;
    ignoreList |= NET::MenuMask;
    ignoreList |= NET::PopupMenuMask;
    ignoreList |= NET::NotificationMask;

    KWindowInfo info(wid, NET::WMWindowType | NET::WMState, NET::WM2TransientFor | NET::WM2WindowClass);

    if (!info.valid())
        return false;

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList))
        return false;

    if (info.hasState(NET::SkipTaskbar) || info.hasState(NET::SkipPager))
        return false;

    WId transFor = info.transientFor();
    
    // Qt6 中获取根窗口的新方式
    auto nativeInterface = QGuiApplication::platformNativeInterface();
    if (nativeInterface) {
        void* resource = nativeInterface->nativeResourceForIntegration("rootwindow");
        if (resource) {
            quint32 rootWindow = static_cast<quint32>(reinterpret_cast<quintptr>(resource));
            if (transFor == 0 || transFor == wid || transFor == rootWindow)
                return true;
        }
    }
    
    if (transFor == 0 || transFor == wid)
        return true;

    KWindowInfo transInfo(transFor, NET::WMWindowType);

    QFlags<NET::WindowTypeMask> normalFlag;
    normalFlag |= NET::NormalMask;
    normalFlag |= NET::DialogMask;
    normalFlag |= NET::UtilityMask;

    return !NET::typeMatchesMask(transInfo.windowType(NET::AllTypesMask), normalFlag);
}

void XWindowInterface::setViewStruts(QWindow *view, DockSettings::Direction direction, const QRect &rect, bool compositing)
{
    NETExtendedStrut strut;

    const auto screen = view->screen();
    bool isRound = DockSettings::self()->style() == DockSettings::Round;
    const int edgeMargins = compositing && isRound ? DockSettings::self()->edgeMargins() : 0;

    switch (direction) {
    case DockSettings::Left: {
        const int leftOffset = screen->geometry().left();
        strut.left_width = rect.width() + leftOffset + edgeMargins;
        strut.left_start = rect.y();
        strut.left_end = rect.y() + rect.height() - 1;
        break;
    }
    case DockSettings::Bottom: {
        strut.bottom_width = rect.height() + edgeMargins;
        strut.bottom_start = rect.x();
        strut.bottom_end = rect.x() + rect.width();
        break;
    }
    case DockSettings::Right: {
        strut.right_width = rect.width() + edgeMargins;
        strut.right_start = rect.y();
        strut.right_end = rect.y() + rect.height() - 1;
        break;
    }
    default:
        break;
    }

    // kde6移除了这个函数所以目前先删掉
    // KWindowSystem::setExtendedStrut(view->winId(),
    //                                 strut.left_width,   strut.left_start,   strut.left_end,
    //                                 strut.right_width,  strut.right_start,  strut.right_end,
    //                                 strut.top_width,    strut.top_start,    strut.top_end,
    //                                 strut.bottom_width, strut.bottom_start, strut.bottom_end);
}

void XWindowInterface::clearViewStruts(QWindow *view)
{
    // KWindowSystem::setExtendedStrut(view->winId(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

void XWindowInterface::startInitWindows()
{
    const auto windows = KX11Extras::windows();
    for (auto wid : windows) {
        processWindowAdded(wid);
    }
}

QString XWindowInterface::desktopFilePath(quint64 wid)
{
    const KWindowInfo info(wid, NET::Properties(), NET::WM2WindowClass | NET::WM2DesktopFileName);
    
    KWindowInfo pidInfo(wid, NET::WMPid);
    const qint64 pid = pidInfo.pid();

    return Utils::instance()->desktopPathFromMetadata(info.windowClassClass(), pid, info.windowClassName());
}

void XWindowInterface::setIconGeometry(quint64 wid, const QRect &rect)
{
    // kde6中没有这个函数了，先注释掉
    // KX11Extras::setIconGeometry(wid, rect);
}

void XWindowInterface::handleWindowAdded(WId wid)
{
    processWindowAdded(wid);
}

void XWindowInterface::handleWindowRemoved(WId wid)
{
    emit windowRemoved(wid);
}

void XWindowInterface::handleActiveWindowChanged(WId wid)
{
    emit activeChanged(wid);
}

void XWindowInterface::processWindowAdded(quint64 wid)
{
    if (isAcceptableWindow(wid)) {
        emit windowAdded(wid);
    }
}