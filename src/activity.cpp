/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     Reion Wong <reionwong@gmail.com>
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

#include "activity.h"
#include "docksettings.h"

#include <KWindowSystem>
#include <KX11Extras>
#include <KWindowInfo>

static Activity *SELF = nullptr;

Activity *Activity::self()
{
    if (!SELF)
        SELF = new Activity;

    return SELF;
}

Activity::Activity(QObject *parent)
    : QObject(parent)
    , m_existsWindowMaximized(false)
{
    onActiveWindowChanged();

    connect(KX11Extras::self(), &KX11Extras::activeWindowChanged,
        this, &Activity::onActiveWindowChanged);
    connect(KX11Extras::self(),
        static_cast<void (KX11Extras::*)(WId, NET::Properties, NET::Properties2)>
        (&KX11Extras::windowChanged),
        this, &Activity::onActiveWindowChanged);
}

bool Activity::existsWindowMaximized() const
{
    return m_existsWindowMaximized;
}

bool Activity::launchPad() const
{
    return m_launchPad;
}
#include <QDebug>
void Activity::onActiveWindowChanged()
{
    // 获取活动窗口
    WId activeWindow = KX11Extras::activeWindow();
    
    if (!activeWindow) {
        // 没有活动窗口时的处理
        if (m_launchPad) {
            m_launchPad = false;
            emit launchPadChanged();
        }
        return;
    }

    // 获取窗口信息 - 使用 KWindowInfo (如果可用)
    KWindowInfo info(activeWindow, NET::WMState | NET::WMVisibleName | NET::WMWindowType,
                    NET::WM2WindowClass);
    
    // 检查是否是启动器
    QString windowClass = info.windowClassClass();
    bool launchPad = windowClass == "cutefish-launcher" || 
                    info.windowType(NET::AllTypesMask) == NET::Dock;

    // 智能隐藏模式处理
    if (DockSettings::self()->visibility() == DockSettings::IntellHide) {
        bool existsWindowMaximized = false;

        // 获取所有窗口
        const QList<WId> windows = KX11Extras::windows();
        
        for (WId wid : windows) {
            KWindowInfo windowInfo(wid, NET::WMState, NET::WM2WindowClass);

            // 跳过最小化或跳过任务栏的窗口
            if (windowInfo.isMinimized() || windowInfo.hasState(NET::SkipTaskbar))
                continue;

            // 检查是否最大化
            if (windowInfo.hasState(NET::MaxVert) && windowInfo.hasState(NET::MaxHoriz)) {
                existsWindowMaximized = true;
                break;
            }
        }

        if (m_existsWindowMaximized != existsWindowMaximized) {
            m_existsWindowMaximized = existsWindowMaximized;
            emit existsWindowMaximizedChanged();
        }
    }

    // 更新启动器状态
    if (m_launchPad != launchPad) {
        m_launchPad = launchPad;
        emit launchPadChanged();
    }

    // 更新窗口信息
    m_pid = info.pid();
    m_windowClass = windowClass.toLower();
    
    // // 可选：发射其他信号
    // emit activeWindowChanged(activeWindow);
}