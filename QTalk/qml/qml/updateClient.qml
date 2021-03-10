import QtQuick 2.10
import "." as NetCom
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import PkgDownload 1.0

Rectangle {
    id: root

    property bool has_update: false
    property string newVersion: ""
    property string downloadLink: ""
    property string packageMd5: ""

    Rectangle {
        id: title
        width: root.width
        height: 40
        y: 0
        z:2
        color: "#dbdbdb"

        Text {
            color: "#000000"
            text: qsTr("软件更新")
            font.pixelSize: 16
            font.bold: true
            anchors.centerIn: parent
        }
    }

    Rectangle {
        id: errorFrm
        width: root.width
        height: 30
        x: 0; y: 40
        z: 1
        visible: false
        color: "#88FF0000"

        property alias text : errorCont.text

        Text {
            id: errorCont
            anchors.centerIn: parent
            font.pixelSize: 14
            color: "#FFF"
        }

        Timer {
            id: errFrmTimer
            interval: 3000
            triggeredOnStart: false;
            repeat: true;
            onTriggered: {
                if(hideAnim.paused)
                    hideAnim.restart();
                else
                    hideAnim.start()
            }
        }

        PropertyAnimation {
            id: hideAnim
            target: errorFrm
            property: "opacity"
            duration: 1000
            from: 1.0; to: 0.0
            easing.type: Easing.InOutQuad

            onStopped: {
                errorFrm.visible = false
                errorFrm.opacity = 1
            }

            onPausedChanged: {
                errorFrm.opacity = 1
            }
        }
    }


    Rectangle {
        id: leftFrm
        width: 180
        height: root.height - title.height
        x:0; y: title.height
        color: "#ececec"

        Image {
            id: icon
            anchors.horizontalCenter: parent.horizontalCenter
            y:leftFrm.y + 20
            source: "http://chaocc.wang/resource/" + $QmlEngine.getExecuteName() + "/icon"
            width: 80
            height: 80
        }

        Text {
            id: version
            anchors.horizontalCenter: parent.horizontalCenter
            y: icon.y + icon.height + 50
            text: "Ver: (2.0 build - 2.1.010)"
        }

        Component.onCompleted: {
            version.text = "Ver: " + $QmlEngine.getClientVersion()
        }
    }


    Rectangle {
        id: rightFrm
        color: "#e3e3e3"
        width: root.width - leftFrm.width
        height: root.height - title.height
        x: leftFrm.width
        y: title.height


        Rectangle {
            id: checkUpdateFrm
            width: parent.width - 40
            height: 80
            color: "#dadada"
            x: 20
            y: 20
            z: 100
            radius: 10
            visible: !has_update

            AnimatedImage {
                id: loading
                source: "http://chaocc.wang/resource/loading.gif"
                width: 30; height: 30
                x: 10
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: tipContent
                text: qsTr("正在检查更新.")
                anchors.verticalCenter: parent.verticalCenter
                x: 50
            }
        }

        Rectangle {
            id: showNewFrm
            width: parent.width - 40
            height: 200
            color: "#dadada"
            x: 20
            y: 20
            radius: 10
            visible: has_update

            Text {
                id: updateNewVersionLabel
                x: 10
                y: 10
                font.bold: true
                font.pixelSize: 14
            }

            Flickable {
                id: flick
                width: parent.width
                height: parent.height - 50
                y: 40
                x: 10
                flickableDirection: Flickable.VerticalFlick
                contentWidth: updateContent.paintedWidth
                contentHeight: updateContent.paintedHeight
                clip: true

                TextEdit {
                    id: updateContent
                    readOnly: true
                    text: ""
                }
            }

            ProgressBar{
                id: downloadProcessBar
                minimumValue: 0
                maximumValue: 100
                visible: false
                width: parent.width - 20;
                height: 5;
                x: 10
                y: flick.height + 60

                style: ProgressBarStyle {
                    background: Rectangle {
                        radius: 2
                        color: "#fff"
                        border.width: 0
                        implicitWidth: 200
                        implicitHeight: 24
                    }
                    progress: Rectangle {
                        radius: 2
                        border.width: 0
                        color: "lightgreen"
                        border.color: "steelblue"
                    }
                }

            }

        }

        Rectangle {
            width: parent.width
            height: 20
            color: "transparent"
            x: 0
            y: parent.height - 70
            visible: has_update

            Text {
                id: countDownText
                x: 30
                color: "#555"
                anchors.verticalCenter: parent.verticalCenter
            }

            Timer {
                id: countDownTimer
                interval: 1000
                triggeredOnStart: true;
                repeat: true;

                property int countDownCount: 10
                onTriggered: {
                    if(countDownCount <= 0) {
                        downloadHelper.openPkg();
                        stop();
                        return;
                    }
                    countDownCount--;
                    countDownText.text = countDownCount + qsTr("s 后打开更新.");
                }
            }
        }

        Rectangle {
            id: btnFrm
            color: "#e3e3e3"
            width: parent.width
            height: 40
            x: 0
            y: parent.height - height
            visible: has_update

            NetCom.Button {
                id: cancelUpdateBtn
                width: 80
                height: 30
                x: parent.width - 100
                normal_color: "#00C3BC"
                hover_color: "#AA00CABE"
                text: {
                    checked ? qsTr("继续") : qsTr("暂停")
                }
                border.color: "#00AAAAAA"
                radius: 6
                text_color: "#FFF"
                visible: false
                property bool checked: false

                onClicked: {
                    if(checked) {
                        countDownTimer.start()
                    }
                    else if(countDownTimer.running){
                        countDownTimer.stop();
                    }
                    checked = !checked
                }
            }

            NetCom.Button {
                id: updateBtn
                width: 80
                height: 30
                x: parent.width - 100
                normal_color: "#00C3BC"
                hover_color: "#AA00CABE"
                text: qsTr("立即更新")
                border.color: "#00AAAAAA"
                radius: 6
                text_color: "#FFF"
                onClicked: downloadFile()
            }
        }

        PkgDownload {
            id: downloadHelper

            onDownloadProgress: {
                var process = bytesReceived / bytesTotal * 100;
                downloadProcessBar.value = process;
            }

            onFinished: {
                if(success) {
                    console.log("download success");
                    if(downloadHelper.getPkgMd5() === packageMd5) {
                        countDownTimer.countDownCount = 10
                        countDownTimer.start();
                        updateBtn.visible = false
                        cancelUpdateBtn.visible = true
                        downloadProcessBar.visible = false
                        $QmlEngine.setVisible(true)
                        return
                    }
                    else {
                        console.log("check package md5 error");
                        onShowError(qsTr("安装包校验失败"))
                    }
                }
                else {
                    console.log("download failed");
                }

                downloadProcessBar.visible = false
                updateBtn.enabled = true
            }

            onError: {
                console.log(error)
                onShowError(error)
            }
        }
    }

    Connections {
        target: $QmlEngine
        onCheckUpdate : doCheckUpdate()
    }

    Component.onCompleted: {
//        loadConf()
        doCheckUpdate()
    }

    function onShowError (text) {
        errorFrm.visible = true
        errorFrm.text = text
        if (errFrmTimer.running) {
            errFrmTimer.stop()
        }
        if(hideAnim.running) {
            hideAnim.pause();
        }
        errFrmTimer.start()
    }

    function downloadFile() {
        downloadHelper.url = downloadLink
        updateBtn.enabled = false
        if(downloadHelper.getPkgMd5() === packageMd5) {
            countDownTimer.countDownCount = 10
            countDownTimer.start();
            updateBtn.visible = false
            cancelUpdateBtn.visible = true
            $QmlEngine.setVisible(true)
        }
        else {
            downloadProcessBar.visible = true
            downloadHelper.start()
        }
    }

    function getVersionChanged(ver) {
        let url = serverRoot + "versionchanged?platform=" +
            $QmlEngine.getPlatformStr() +
            "&exec=" + $QmlEngine.getExecuteName() +
            "&version=" + ver +
            "&user=" + $QmlEngine.getSelfXmppId() +
            "&action=1";

        var xmlhttp = new XMLHttpRequest();
        if(xmlhttp === null) {
            alert('暂不支持http');
            return;
        }
        xmlhttp.onreadystatechange = function() {
            if(xmlhttp.readyState === 4 && xmlhttp.status === 200) {
                var res = JSON.parse(xmlhttp.responseText);
                if(res['ret']) {
                    updateContent.text = res['data']
                }
            }else{

            }
        }
        xmlhttp.open("GET", url, false);
        xmlhttp.send(null);
    }

    function doCheckUpdate() {
        if(downloadHelper.downloading || countDownTimer.running ) {
            return;
        }

        let url = serverRoot + "checkupdater?platform=" +
            $QmlEngine.getPlatformStr() +
            "&exec=" + $QmlEngine.getExecuteName() +
            "&clientVersion=" + $QmlEngine.getClientVersion() +
            "&beta=" + $QmlEngine.getTestChannel() +
            "&os=" + $QmlEngine.getOSVersion() +
            "&versionId=" + $QmlEngine.getVersionId() +
            "&user=" + $QmlEngine.getSelfXmppId() ;

        var xmlhttp = new XMLHttpRequest();
        if(xmlhttp === null) {
            alert('暂不支持http');
            return;
        }
        xmlhttp.onreadystatechange = function() {
            if(xmlhttp.readyState === 4 && xmlhttp.status === 200) {
                var res = JSON.parse(xmlhttp.responseText);

                if(res['ret']) {
                    downloadLink = res['link'];
                    var force = false;
                    if (res.hasOwnProperty('newVersion')) {
                        newVersion = res['newVersion']
                        has_update = (downloadLink !== "" && downloadLink !== null)
                        packageMd5 = res['md5']
                        force = res['force']
                    }
                    else {
                        has_update = false
                    }

                    if(has_update) {
                        updateNewVersionLabel.text = qsTr("新版本 ") + newVersion + qsTr(" 可用")
                        getVersionChanged(newVersion)

                        if(force) {
                            downloadFile();
                        }
                    }
                    else {
                        loading.visible = false
                        tipContent.text = qsTr("暂无可用更新.")
                        downloadHelper.clearPkg()
                    }

                    $QmlEngine.setUpdateTipVisible(has_update)
                }
            }else{

            }
        }
        xmlhttp.open("GET", url, false);
        xmlhttp.send(null);
    }

//    function loadConf() {
//        let url = "http://chaocc.wang/resource/" + $QmlEngine.getExecuteName() + "/conf.json";
//        var xmlhttp = new XMLHttpRequest();
//        if(xmlhttp === null) {
//            return;
//        }
//        xmlhttp.onreadystatechange = function() {
//            if(xmlhttp.readyState === 4 && xmlhttp.status === 200) {
//                console.log(xmlhttp.responseText)
//                var res = JSON.parse(xmlhttp.responseText);
//                if(res['ret']) {
//                    var data = res['data']

//                }
//            }else{

//            }
//        }
//        xmlhttp.open("GET", url, false);
//        xmlhttp.send(null);
//    }
}
