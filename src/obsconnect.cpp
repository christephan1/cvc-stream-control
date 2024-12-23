// vim:ts=4:sw=4:et:cin

#include "obsconnect.h"
#include <stdio.h>
#include <iostream>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "cvcsetting.h"

OBSConnect::OBSConnect()
    : QWebSocket()
{
    connect(this, &QWebSocket::connected, this, [this]() { emit updateStatus("OBS connecting."); });
    connect(this, &QWebSocket::disconnected, this, [this]() { emit updateStatus("OBS disconnected."); connectOBS(); });
    connect(this, &QWebSocket::textMessageReceived, this, &OBSConnect::processOBSMsg);

    connectOBS();
}

void OBSConnect::connectOBS()
{
    isConnected = false;
    QTimer::singleShot( 5000, this, [this] () {
        QUrl url;
        url.setScheme("ws");
        url.setHost(OBS_SERVER);
        url.setPort(OBS_PORT);
        open(url);
    });
}

void OBSConnect::sendRequest(const char* requestType, QJsonObject&& requestData)
{
    static int requestId = 0;
    sendRequest(6, //op = Request
        QJsonObject {
            {"requestType", requestType},
            {"requestId", ++requestId},
            {"requestData", std::move(requestData)}
        }
    );
}

void OBSConnect::sendRequest(const int op, QJsonObject&& d)
{
    QJsonObject request {
        {"op", op},
        {"d", std::move(d)}
    };
    //std::cout << "request: " << QString::fromUtf8 (QJsonDocument(request).toJson(QJsonDocument::Compact)).toStdString() << std::endl;
    sendTextMessage (QString::fromUtf8 (QJsonDocument(request).toJson(QJsonDocument::Compact)));
}

void OBSConnect::processOBSMsg(const QString& msg)
{
    //std::cout << "msg: " << msg.toStdString() << std::endl;
    QJsonDocument json = QJsonDocument::fromJson(msg.toUtf8());
    if (json.isNull()) return;

    switch (json["op"].toInt(/*default=*/-1)) {
        case 0: //Hello
            {
                sendRequest (1, //op = Identify
                    QJsonObject {
                        {"rpcVersion", 1},
                        {"eventSubscriptions", 4 | 1024} //4 - Scenes Events, 1024 - UI Events
                    });
                isConnected = false;
                emit updateStatus("OBS connecting..");
                break;
            }

        case 2: //Identified
            {
                sendRequest("GetStudioModeEnabled");
                sendRequest("GetSceneList");
                isConnected = true;
                emit updateStatus("OBS connected.");
                break;
            }

        case 5: //Event
            {
                QString eventType = json["d"]["eventType"].toString();
                if (eventType == "CurrentProgramSceneChanged" || eventType == "CurrentPreviewSceneChanged") {
                    QString sceneName = json["d"]["eventData"]["sceneName"].toString();
                    auto sId = getSceneIdFromName(sceneName);
                    emit updateStatus("Current Scene: " + sceneName);
                    emit currentSceneChanged(sId.first, sId.second);

                } else if (eventType == "StudioModeStateChanged") {
                    isStudioMode = json["d"]["eventData"]["studioModeEnabled"].toBool();

                } else if (eventType == "SceneCreated") {
                    createScene(json["d"]["eventData"]["sceneName"].toString());

                } else if (eventType == "SceneRemoved") {
                    removeScene(json["d"]["eventData"]["sceneName"].toString());

                } else if (eventType == "SceneNameChanged") {
                    removeScene(json["d"]["eventData"]["oldSceneName"].toString());
                    createScene(json["d"]["eventData"]["sceneName"].toString());
                }
#if 0
                std::cout << "---\n";
                for (const auto& p1 : sceneMap) {
                    for (const auto& p2: p1.second) {
                        std::cout << "SceneId: " << (int)p1.first << " CamId: " << (int)p2.first << " SceneName: " << p2.second.toStdString() << std::endl;
                    }
                }
                std::cout << "---\n";
#endif
                break;
            }

        case 7: //RequestResponse
            {
                if (json["d"]["requestType"].toString() == "GetSceneList") {
                    processSceneList(json["d"]["responseData"]["scenes"].toArray());
                    QString previewSceneName = json["d"]["responseData"]["currentPreviewSceneName"].toString();
                    QString programSceneName = json["d"]["responseData"]["currentProgramSceneName"].toString();
                    auto sId = getSceneIdFromName(previewSceneName.isEmpty()? programSceneName : previewSceneName);
                    emit currentSceneChanged(sId.first, sId.second);

                } else if (json["d"]["requestType"].toString() == "GetStudioModeEnabled") {
                    isStudioMode = json["d"]["responseData"]["studioModeEnabled"].toBool();
                }
                break;
            }
    };
}

void OBSConnect::processSceneList(QJsonArray&& sceneList)
{
    sceneMap.clear();
    for (QJsonValueRef scene: sceneList) {
        createScene (scene.toObject()["sceneName"].toString());
    }
}

std::pair<uint_fast8_t,uint_fast8_t> OBSConnect::getSceneIdFromName(const QString& sceneName)
{
    int nNum = 0;
    uint_fast8_t num[2] = {0, 0};
    for (const QChar& c : sceneName) {
        int d = c.digitValue();
        if (d >= 0) {
            uint_fast8_t newNum = num[nNum]*10+d;
            if (newNum < num[nNum]) return {0,0}; //overflow
            num[nNum] = newNum;
        } else {
            if (nNum == 0 && c == '.') {
                nNum = 1;
            } else {
                break;
            }
        }
    }
    return {num[0],num[1]};
}

void OBSConnect::createScene(QString&& sceneName)
{
    uint_fast8_t sceneId, camId;
    std::tie(sceneId, camId) = getSceneIdFromName(sceneName);
    if (sceneId == 0) return;
    sceneMap[sceneId][camId] = std::move(sceneName);
    //std::cout << "SceneId: " << (int)sceneId << " CamId: " << (int)camId << " SceneName: " << sceneMap[sceneId][camId].toStdString() << std::endl;
}

void OBSConnect::removeScene(const QString& sceneName)
{
    uint_fast8_t sceneId, camId;
    std::tie(sceneId, camId) = getSceneIdFromName(sceneName);
    auto iter1 = sceneMap.find(sceneId);
    if (iter1 != sceneMap.end()) {
        auto iter2 = iter1->second.find(camId);
        if (iter2 != iter1->second.end())
            iter1->second.erase(iter2);
        if (iter1->second.empty()) 
            sceneMap.erase(iter1);
    }
}

void OBSConnect::switchToScene(uint_fast8_t sceneId, uint_fast8_t camId)
{
    auto iter1 = sceneMap.find(sceneId);
    if (iter1 != sceneMap.end()) {
        auto iter2 = iter1->second.find(camId);
        if (iter2 == iter1->second.end())
            iter2 = iter1->second.find(0);
        if (iter2 != iter1->second.end()) {
            sendRequest(isStudioMode? "SetCurrentPreviewScene" : "SetCurrentProgramScene", QJsonObject{{"sceneName", iter2->second}});
            return;
        }
    }
    emit updateStatus("Scene " + QString::number(sceneId) + '.' + QString::number(camId) + " not existed.");
}

void OBSConnect::switchStudioMode()
{
    sendRequest("SetStudioModeEnabled", QJsonObject{{"studioModeEnabled", !isStudioMode}});
}

uint_fast8_t OBSConnect::getPrevSceneId(uint_fast8_t sceneId) const
{
    decltype(sceneMap)::const_reverse_iterator iter(sceneMap.lower_bound(sceneId));
    if (iter == sceneMap.rend()) {
        return 0;
    } else {
        return iter->first;
    }
}

uint_fast8_t OBSConnect::getNextSceneId(uint_fast8_t sceneId) const
{
    decltype(sceneMap)::const_iterator iter(sceneMap.upper_bound(sceneId));
    if (iter == sceneMap.end()) {
        return 0;
    } else {
        return iter->first;
    }
}

