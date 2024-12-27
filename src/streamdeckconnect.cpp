// vim:ts=4:sw=4:et:cin

#include "streamdeckconnect.h"
#include <stdio.h>
#include <iostream>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QImage>
#include "cvcsetting.h"
#include "streamdeckkey.h"

StreamDeckConnect::StreamDeckConnect(const StreamDeckSettings& settings_, const std::vector<CameraSettings>& cameraSettings)
    : QWebSocket(), settings(settings_), CAMERAS(cameraSettings)
{
    connect(this, &QWebSocket::connected, this, [this]() { emit updateStatus("StreamDeck connecting."); });
    connect(this, &QWebSocket::disconnected, this, &StreamDeckConnect::onDisconnect);
    connect(this, &QWebSocket::textMessageReceived, this, &StreamDeckConnect::processStreamDeckMsg);

    connectStreamDeck();
}

void StreamDeckConnect::connectStreamDeck()
{
    QTimer::singleShot( 6789, this, [this] () {
        QUrl url;
        url.setScheme("ws");
        url.setHost(settings.STREAM_DECK_HOST);
        url.setPort(settings.STREAM_DECK_PORT);
        open(url);
    });
}

void StreamDeckConnect::onDisconnect()
{
    emit updateStatus("StreamDeck disconnected.");
    sceneKeyMap.clear();
    cameraKeyMap.clear();
    studioModeKey = nullptr;
    switchCamKey = nullptr;
    prevCamKey = nullptr;
    nextCamKey = nullptr;
    moveUpKey = nullptr;
    moveDownKey = nullptr;
    moveLeftKey = nullptr;
    moveRightKey = nullptr;
    zoomOutKey = nullptr;
    zoomInKey = nullptr;
    ptzStopKey = nullptr;
    focusFarKey = nullptr;
    focusNearKey = nullptr;
    focusAutoKey = nullptr;
    presetKeyMap = {};
    prevPresetKey = nullptr;
    nextPresetKey = nullptr;
    for (auto& page : key)
        for (auto& row : page)
            for (StreamDeckKey*& keyPtr : row)
                if (keyPtr) {
                    delete keyPtr;
                    keyPtr = nullptr;
                }
    connectStreamDeck();
}

void StreamDeckConnect::sendRequest(const char* event, QJsonObject&& payload)
{
    QJsonObject request {
        {"event", event},
        {"uuid", uuid},
        {"payload", std::move(payload)}
    };
    sendTextMessage (QString::fromUtf8 (QJsonDocument(std::move(request)).toJson(QJsonDocument::Compact)));
}

void StreamDeckConnect::processStreamDeckMsg(const QString& msg)
{
    //std::cout << "msg: " << msg.toStdString() << std::endl;
    QJsonDocument json = QJsonDocument::fromJson(msg.toUtf8());
    if (json.isNull()) return;

    QString event = json["event"].toString();
    if (event.isNull()) return;

    if (event == "connectElgatoStreamDeckSocket") {
        uuid = json["payload"]["inPluginUUID"];
        deckId.clear();
        for (const QJsonValue& device : json["payload"]["inInfo"]["devices"].toArray()) {
            if (device["size"]["columns"].toInt() != 8) continue;
            if (device["size"]["rows"]   .toInt() != 4) continue;
            deckId = device["id"].toString();
            break;
        }
        if (deckId.isEmpty()) {
            close();
            return;
        }
        sendRequest("registerPlugin");
        createKeyHandlers();
        emit updateStatus("StreamDeck connected.");

    } else if (event == "keyDown") {
        if (json["deck_id"].toString() != deckId) return;
        int page   = json["payload"]["page"].toInt(-1);
        int row    = json["payload"]["coordinates"]["row"]   .toInt(-1);
        int column = json["payload"]["coordinates"]["column"].toInt(-1);
        if (page < 0 || row < 0 || column < 0) return;
        if (page >= NUM_PAGE || row >= NUM_ROW || column >= NUM_COLUMN) return;
        StreamDeckKey* theKey = key[page][row][column];
        if (theKey) theKey->onKeyDown();

    } else if (event == "keyUp") {
        if (json["deck_id"].toString() != deckId) return;
        int page   = json["payload"]["page"].toInt(-1);
        int row    = json["payload"]["coordinates"]["row"]   .toInt(-1);
        int column = json["payload"]["coordinates"]["column"].toInt(-1);
        if (page < 0 || row < 0 || column < 0) return;
        if (page >= NUM_PAGE || row >= NUM_ROW || column >= NUM_COLUMN) return;
        StreamDeckKey* theKey = key[page][row][column];
        if (theKey) theKey->onKeyUp();
    }
}

void StreamDeckConnect::setPage(int page)
{
    sendRequest("setPage",
            QJsonObject{
                {"device", deckId},
                {"page", page}
            });
}

void StreamDeckConnect::clearButton(int page, int row, int column)
{
    sendRequest("setImage",
            QJsonObject{
                {"device", deckId},
                {"page", page},
                {"row", row},
                {"column", column}
            });
}

void StreamDeckConnect::createKeyHandlers()
{
#define DEFINE_KEY(p,r,c,imgPath) key[p][r][c] = new StreamDeckKey(this, deckId, p, r, c, QImage(QString::fromUtf8(imgPath)))
#define DEFINE_SWITCH(p,r,c,imgOff,imgOn,en) static_cast<StreamDeckKey_Switch*>(key[p][r][c] = new StreamDeckKey_Switch(this, deckId, p, r, c, QImage(QString::fromUtf8(imgOff)), QImage(QString::fromUtf8(imgOn)),en))
#define DEFINE_SCENE(p,r,c,imgOff,imgOn,scene) \
    key[p][r][c] = sceneKeyMap[scene] = new StreamDeckKey_Scene(this, deckId, p,r,c, QImage(QString::fromUtf8(imgOff)), QImage(QString::fromUtf8(imgOn)), scene); \
    connect(key[p][r][c], &StreamDeckKey::keyDown, this, \
        [this](){ \
            emit sceneChanged(scene, camIndex); \
            emit switchScene(); \
        });

#define DEFINE_TALLY(p,r,c,imgD,imgE,imgP,camId,isActive,isPreview) \
    key[p][r][c] = new StreamDeckKey_Tally(this, deckId, p,r,c, QImage(QString::fromUtf8(imgD)),QImage(QString::fromUtf8(imgE)),QImage(QString::fromUtf8(imgP)),camId,isActive,isPreview)
#define DEFINE_PRESET(p,r,c,img,presetId,isEnable) static_cast<StreamDeckKey_Preset*>(key[p][r][c] = new StreamDeckKey_Preset(this,deckId,p,r,c,QImage(QString::fromUtf8(img)),presetId,isEnable))

    // page 0
    // left pane
    DEFINE_KEY(0,0,0, ":/icon/icon/Home_E.png");
    DEFINE_KEY(0,1,0, ":/icon/icon/Util_D.png");
    connect(key[0][1][0], &StreamDeckKey::keyUp, this, [this](){ setPage(2); });
    studioModeKey = DEFINE_SWITCH(0,3,0, ":/icon/icon/StudioMode_D.png", ":/icon/icon/StudioMode_E.png", isStudioMode);
    connect(studioModeKey, &StreamDeckKey::keyDown, this, [this](){ emit switchStudioMode(); });
    clearButton(0,2,0);

    // scene area
    DEFINE_SCENE(0,0,1, ":/icon/icon/01D_CamOnly.png", ":/icon/icon/01E_CamOnly.png", 1);
    DEFINE_SCENE(0,0,2, ":/icon/icon/02D_TextBelow.png", ":/icon/icon/02E_TextBelow.png", 2);
    DEFINE_SCENE(0,0,3, ":/icon/icon/03D_TextAbove.png", ":/icon/icon/03E_TextAbove.png", 3);
    DEFINE_SCENE(0,0,4, ":/icon/icon/04D_RightSlide1.png", ":/icon/icon/04E_RightSlide1.png", 4);
    DEFINE_SCENE(0,0,5, ":/icon/icon/05D_LeftSlide1.png", ":/icon/icon/05E_LeftSlide1.png", 5);
    DEFINE_SCENE(0,0,6, ":/icon/icon/06D_RightSlide.png", ":/icon/icon/06E_RightSlide.png", 6);
    DEFINE_SCENE(0,0,7, ":/icon/icon/07D_LeftSlide.png", ":/icon/icon/07E_LeftSlide.png", 7);
    DEFINE_SCENE(0,1,1, ":/icon/icon/08D_SlideOnCam.png", ":/icon/icon/08E_SlideOnCam.png", 8);
    DEFINE_SCENE(0,1,2, ":/icon/icon/09D_SlideOnly.png", ":/icon/icon/09E_SlideOnly.png", 9);
    DEFINE_SCENE(0,1,3, ":/icon/icon/10D_Disable.png", ":/icon/icon/10E_Disable.png", 10);
    DEFINE_SCENE(0,1,4, ":/icon/icon/11D_Begin.png", ":/icon/icon/11E_Begin.png", 11);
    clearButton(0,1,5);
    clearButton(0,1,6);
    clearButton(0,1,7);
    clearButton(0,2,1);
    clearButton(0,2,2);
    clearButton(0,2,3);
    clearButton(0,2,4);
    clearButton(0,2,5);
    clearButton(0,2,6);
    clearButton(0,2,7);

    // camera area
    for (int i = 0; i < 7; i++) {
        if (i < CAMERAS.size()) {
            auto theKey = DEFINE_TALLY(0,3,i+1, ":/icon/icon/Tally_D.png", ":/icon/icon/Tally_E.png", ":/icon/icon/Tally_P.png", CAMERAS[i].CAMERA_ID, curCamIndex==i, camIndex==i);
            cameraKeyMap.push_back(static_cast<StreamDeckKey_Tally*>(theKey));
            connect(theKey, &StreamDeckKey::keyUp, this, [this, i](){ emit selectCam(i); setPage(1); });
        } else {
            clearButton(0,3,i+1);
        }
    }

    // page 1
    // left pane
    DEFINE_KEY(1,0,0, ":/icon/icon/Home_D.png");
    DEFINE_KEY(1,1,0, ":/icon/icon/Util_D.png");
    connect(key[1][0][0], &StreamDeckKey::keyUp, this, [this](){ setPage(0); });
    connect(key[1][1][0], &StreamDeckKey::keyUp, this, [this](){ setPage(2); });

    // camera switching area
    int camId = camIndex>=0 && camIndex<CAMERAS.size()? CAMERAS[camIndex].CAMERA_ID : -1;
    switchCamKey = static_cast<StreamDeckKey_Tally*> (DEFINE_TALLY(1,0,6,
            ":/icon/icon/Switch_Tally_D.png", ":/icon/icon/Switch_Tally_E.png", ":/icon/icon/Switch_Tally_P.png",
            camId, camId==curCamIndex, camId >= 0));
    connect(switchCamKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::switchScene);;
    prevCamKey = DEFINE_SWITCH(1,0,5,":/icon/icon/PrevCam_D.png",":/icon/icon/PrevCam_E.png",camIndex > 0);
    nextCamKey = DEFINE_SWITCH(1,0,7,":/icon/icon/NextCam_D.png",":/icon/icon/NextCam_E.png",camIndex >= 0 && camIndex<CAMERAS.size()-1);
    connect(prevCamKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::prevCam);
    connect(nextCamKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::nextCam);

    // camera PTZ area
    moveUpKey    = DEFINE_KEY(1,1,6, ":/icon/icon/Move_Up.png");
    moveDownKey  = DEFINE_KEY(1,3,6, ":/icon/icon/Move_Down.png");
    moveLeftKey  = DEFINE_KEY(1,2,5, ":/icon/icon/Move_Left.png");
    moveRightKey = DEFINE_KEY(1,2,7, ":/icon/icon/Move_Right.png");
    zoomOutKey   = DEFINE_KEY(1,3,5, ":/icon/icon/Zoom_Out.png");
    zoomInKey    = DEFINE_KEY(1,3,7, ":/icon/icon/Zoom_In.png");
    ptzStopKey   = DEFINE_KEY(1,2,6, ":/icon/icon/PTZ_Stop_E.png"); //[TODO] long press
    connect(moveUpKey,    &StreamDeckKey::keyDown, this, &StreamDeckConnect::moveUp);
    connect(moveDownKey,  &StreamDeckKey::keyDown, this, &StreamDeckConnect::moveDown);
    connect(moveLeftKey,  &StreamDeckKey::keyDown, this, &StreamDeckConnect::moveLeft);
    connect(moveRightKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::moveRight);
    connect(zoomOutKey,   &StreamDeckKey::keyDown, this, &StreamDeckConnect::zoomOut);
    connect(zoomInKey,    &StreamDeckKey::keyDown, this, &StreamDeckConnect::zoomIn);
    connect(ptzStopKey,   &StreamDeckKey::keyDown, this, &StreamDeckConnect::ptzStop);

    focusFarKey  = DEFINE_KEY(1,1,5, ":/icon/icon/FocusFar.png");
    focusNearKey = DEFINE_KEY(1,1,7, ":/icon/icon/FocusNear.png");
    focusAutoKey = DEFINE_KEY(1,2,0, ":/icon/icon/FocusAuto.png");
    connect(focusFarKey,  &StreamDeckKey::keyDown, this, &StreamDeckConnect::focusFar);
    connect(focusNearKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::focusNear);
    connect(focusAutoKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::focusAuto);

    // camera preset area
    for (size_t i = 0; i < 15; i ++) {
        presetKeyMap[i] = DEFINE_PRESET(1,i/4,i%4+1, ":/icon/icon/Preset.png", minPresetNo+i, i<nPresetNo);
        connect(presetKeyMap[i], &StreamDeckKey::keyUp, this,
            [this, i]() {
                unsigned thisPresetNo = curFirstPreset + i;
                if (thisPresetNo >= minPresetNo && thisPresetNo+1 < minPresetNo+nPresetNo)
                    emit callPreset(thisPresetNo);
            });
        connect(presetKeyMap[i], &StreamDeckKey_LongPress::longPressed, this,
            [this, i]() {
                unsigned thisPresetNo = curFirstPreset + i;
                if (thisPresetNo >= minPresetNo && thisPresetNo+1 < minPresetNo+nPresetNo)
                    emit setPreset(thisPresetNo);
            });
    }
    prevPresetKey = DEFINE_SWITCH(1,3,0,":/icon/icon/PrevPreset_D.png",":/icon/icon/PrevPreset_E.png",false);
    nextPresetKey = DEFINE_SWITCH(1,3,4,":/icon/icon/NextPreset_D.png",":/icon/icon/NextPreset_E.png",nPresetNo>15);
    connect(prevPresetKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::presetPrevPage);
    connect(nextPresetKey, &StreamDeckKey::keyDown, this, &StreamDeckConnect::presetNextPage);

    // page 2
    DEFINE_KEY(2,0,0, ":/icon/icon/Home_D.png");
    DEFINE_KEY(2,1,0, ":/icon/icon/Util_E.png");
    connect(key[2][0][0], &StreamDeckKey::keyUp, this, [this](){ setPage(0); });

#undef DEFINE_KEY
#undef DEFINE_SCENE

    for (auto& page : key)
        for (auto& row : page)
            for (StreamDeckKey*& keyPtr : row)
                if (keyPtr)
                    keyPtr->updateButton();

    setPage(0);
}

void StreamDeckConnect::updatePresetKeys()
{
    if (camIndex >= 0 && camIndex < CAMERAS.size()) {
        minPresetNo = CAMERAS[camIndex].MIN_PRESET_NO;
        nPresetNo = CAMERAS[camIndex].MAX_PRESET_NO - minPresetNo + 1;
        if (curFirstPreset < minPresetNo) curFirstPreset = minPresetNo;
        if (curFirstPreset+1 > minPresetNo+nPresetNo) curFirstPreset = nPresetNo > 15? minPresetNo+nPresetNo-15 : minPresetNo;
    } else {
        minPresetNo = 0;
        nPresetNo = 0;
    }
    for (size_t i = 0; i < 15; i ++) {
        unsigned thisPreset = curFirstPreset + i;
        presetKeyMap[i]->setPresetNo(thisPreset, thisPreset >= minPresetNo && thisPreset < minPresetNo + nPresetNo);
    }
    prevPresetKey->setEnable(curFirstPreset > minPresetNo);
    nextPresetKey->setEnable(curFirstPreset+15 < minPresetNo+nPresetNo);
}

void StreamDeckConnect::presetPrevPage()
{
    if (curFirstPreset > minPresetNo + 15) {
        curFirstPreset -= 15;
    } else {
        curFirstPreset = minPresetNo;
    }
    updatePresetKeys();
}

void StreamDeckConnect::presetNextPage()
{
    if (curFirstPreset+15 < minPresetNo+nPresetNo) curFirstPreset += 15;
    updatePresetKeys();
}

void StreamDeckConnect::setCurScene(uint_fast8_t scene, int camId)
{
    if (scene != curScene) {
        auto iter = sceneKeyMap.find(curScene);
        if (iter != sceneKeyMap.end())
            iter->second->setEnable(false);
        curScene = scene;
        iter = sceneKeyMap.find(curScene);
        if (iter != sceneKeyMap.end())
            iter->second->setEnable(true);
    }
    if (curCamIndex < 0 || curCamIndex >= CAMERAS.size() || camId != CAMERAS[curCamIndex].CAMERA_ID) {
        if (curCamIndex >= 0 && curCamIndex < cameraKeyMap.size())
            cameraKeyMap[curCamIndex]->setActive(false);
        if (curCamIndex == camIndex)
            if (switchCamKey) switchCamKey->setActive(false);
        curCamIndex = -1;
        //[TODO] Use better algorithm when number of cameras increases.
        for (size_t i = 0; i < CAMERAS.size(); i++) {
            if (camId == CAMERAS[i].CAMERA_ID) {
                curCamIndex = i;
                if (curCamIndex >= 0 && curCamIndex < cameraKeyMap.size())
                    cameraKeyMap[curCamIndex]->setActive(true);
                if (curCamIndex >= 0 && curCamIndex == camIndex)
                    if (switchCamKey) switchCamKey->setActive(true);
                break;
            }
        }
    }
}

void StreamDeckConnect::setCamIndex(int cam)
{
    if (cam == camIndex) return;

    // update prev/next camera keys
    if (prevCamKey) if ((camIndex > 0) != (cam > 0)) prevCamKey->setEnable(cam>0);
    if (nextCamKey) if ((camIndex >= 0 && camIndex<CAMERAS.size()-1) != (cam>=0 && cam<CAMERAS.size()-1)) nextCamKey->setEnable(cam>=0 && cam<CAMERAS.size()-1);
    
    // update camera keys
    if (camIndex >= 0 && camIndex < cameraKeyMap.size())
        cameraKeyMap[camIndex]->setPreview(false);
    camIndex = cam;
    if (camIndex >= 0 && camIndex < cameraKeyMap.size()) {
        cameraKeyMap[camIndex]->setPreview(true);
        if (switchCamKey) switchCamKey->setCamId(CAMERAS[camIndex].CAMERA_ID, camIndex == curCamIndex, true);
    } else {
        if (switchCamKey) switchCamKey->setCamId(-1, false, false);
    }
}

void StreamDeckConnect::setStudioMode(bool en)
{
    isStudioMode = en;
    if(studioModeKey) studioModeKey->setEnable(isStudioMode);
}

