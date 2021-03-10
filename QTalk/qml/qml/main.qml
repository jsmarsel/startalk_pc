import QtQuick 2.0
import QtQuick.Window 2.2
import "." as Remote

Rectangle {
    id: root
    width: 484; height:336

    property string serverRoot: ""

    Loader {
        id: _loader
        anchors.fill: parent
    }

    Component.onCompleted: {
        loadConf()
    }

    function loadConf() {
        let url = "http://chaocc.wang/resource/" + $QmlEngine.getExecuteName() + "/conf.json";
        var xmlhttp = new XMLHttpRequest();
        if(xmlhttp === null) {
            return;
        }
        xmlhttp.onreadystatechange = function() {
            if(xmlhttp.readyState === 4 && xmlhttp.status === 200) {
                var res = JSON.parse(xmlhttp.responseText);
                if(res['ret']) {
                    var data = res['data']
                    serverRoot = data['checkUrl']
                    var mode = data['qmlMode']
                    if (mode === 1) {
                        _loader.source = data['updateClient']
                    } else if (mode === 2) {
                        _loader.source = "qrc:/qml/updateClient.qml"
                    }
                }
            }else{

            }
        }
        xmlhttp.open("GET", url, false);
        xmlhttp.send(null);
    }
}
