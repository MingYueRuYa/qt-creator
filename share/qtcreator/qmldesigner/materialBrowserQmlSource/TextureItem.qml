// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

import QtQuick
import QtQuick.Layouts
import QtQuickDesignerTheme
import HelperWidgets
import StudioTheme as StudioTheme

Rectangle {
    id: root

    visible: textureVisible

    color: "transparent"
    border.width: materialBrowserTexturesModel.selectedIndex === index ? 1 : 0
    border.color: materialBrowserTexturesModel.selectedIndex === index
                        ? StudioTheme.Values.themeControlOutlineInteraction
                        : "transparent"

    signal showContextMenu()

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            materialBrowserTexturesModel.selectTexture(index)

            if (mouse.button === Qt.LeftButton)
                rootView.startDragTexture(index, mapToGlobal(mouse.x, mouse.y))
            else if (mouse.button === Qt.RightButton)
                root.showContextMenu()
        }
    }

    Image {
        source: textureSource
        sourceSize.width: root.width - 10
        sourceSize.height: root.height - 10
        anchors.centerIn: parent
        cache: false
    }
}
