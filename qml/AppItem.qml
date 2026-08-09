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

import QtQuick 6.0
import QtQuick.Controls 6.0
import Cutefish.Dock 1.0
import FishUI 1.0 as FishUI

DockItem {
    id: appItem

    property var windowCount: model.windowCount
    property var dragSource: null

    iconName: model.iconName ? model.iconName : "application-x-desktop"
    isActive: model.isActive
    popupText: model.visibleName
    enableActivateDot: windowCount !== 0
    draggable: !model.fixed
    dragItemIndex: index

    onXChanged: {
        if (windowCount > 0)
            updateGeometry()
    }

    onYChanged: {
        if (windowCount > 0)
            updateGeometry()
    }

    onWindowCountChanged: {
        if (windowCount > 0)
            updateGeometry()
    }

    onPositionChanged: updateGeometry()
    onPressed: updateGeometry()
    onRightClicked: {
        if (model.appId === "cutefish-launcher")
            return

        var sz = contextMenu.contentSize()
        var pos = contextMenuPosition(sz.width, sz.height)
        contextMenu.popupAt(pos.x, pos.y)
    }

    onClicked: (mouse) => {
        if (mouse.button === Qt.LeftButton)
            appModel.clicked(model.appId)
        else if (mouse.button === Qt.MiddleButton)
            appModel.openNewInstance(model.appId)
    }

    dropArea.onEntered: {
        appItem.dragSource = drag.source
        dropTimer.restart()
    }

    dropArea.onExited: {
        appItem.dragSource = null
        dropTimer.stop()
    }

    dropArea.onDropped: {
        appModel.save()
        updateGeometry()
    }

    Timer {
        id: dropTimer
        interval: 300
        onTriggered: {
            if (appItem.dragSource)
                appModel.move(appItem.dragSource.dragItemIndex,
                              appItem.dragItemIndex)
            else
                appModel.raiseWindow(model.appId)
        }
    }

    FishUI.DesktopMenu {
        id: contextMenu

        FishUI.MenuItem {
            text: qsTr("Open")
            icon.source: "qrc:/images/open.svg"
            icon.color: FishUI.Theme.textColor
            visible: windowCount === 0
            onTriggered: appModel.openNewInstance(model.appId)
        }

        FishUI.MenuItem {
            text: model.visibleName
            icon.source: model.iconName ? "image://icontheme/" + model.iconName : ""
            visible: windowCount > 0 && model.visibleName
            onTriggered: appModel.openNewInstance(model.appId)
        }

        FishUI.MenuItem {
            text: model.isPinned ? qsTr("Unpin") : qsTr("Pin")
            icon.source: model.isPinned ? "qrc:/images/unpin.svg" : "qrc:/images/pin.svg"
            icon.color: FishUI.Theme.textColor
            visible: model.desktopFile !== ""
            onTriggered: {
                model.isPinned ? appModel.unPin(model.appId) : appModel.pin(model.appId)
            }
        }

        FishUI.MenuItem {
            visible: windowCount !== 0
            text: windowCount === 1 ? qsTr("Close window")
                                    : qsTr("Close %1 windows").arg(windowCount)
            icon.source: "qrc:/images/close.svg"
            icon.color: FishUI.Theme.textColor
            onTriggered: appModel.closeAllByAppId(model.appId)
        }
    }


    function updateGeometry() {
        if (model.fixed)
            return

        appModel.updateGeometries(model.appId, Qt.rect(appItem.mapToGlobal(0, 0).x,
                                                       appItem.mapToGlobal(0, 0).y,
                                                       appItem.width, appItem.height))
    }
}
