import QtQuick 2.0

Rectangle {
    id: root
    property alias text: label.text

    width: 116; height: 26
    color: "#00000000"
    property alias text_color: label.color
    property alias font_size: label.font.pixelSize
    property string normal_color: "#EEE"
    property string hover_color: "#EEE"
    border.color: "slategray"

    signal clicked

    Text {
        id: label
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.clicked()
        }

        hoverEnabled: true
        onEntered: {
            root.color = hover_color
        }
        onExited: {
             root.color = normal_color
        }
    }

    Component.onCompleted: {
        root.color = normal_color
    }
}
