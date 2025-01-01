// vim:ts=4:su=4:et:cin

#pragma once

#include <array>
#include <vector>
#include <QWebSocket>
#include <QJsonValue>
#include <QJsonObject>
#include <QString>
#include "cvcsetting.h"

class StreamDeckSettings;
class StreamDeckKey;
class StreamDeckKey_Switch;
class StreamDeckKey_Scene;
class StreamDeckKey_Tally;
class StreamDeckKey_LongPress;
class StreamDeckKey_Preset;

class StreamDeckConnect : public QWebSocket {
    Q_OBJECT
    public:
        StreamDeckConnect(const StreamDeckSettings&, const std::vector<CameraSettings>&);
        virtual ~StreamDeckConnect() {}

    signals:
        void updateStatus(const QString& msg);
        void sceneChanged(uint_fast8_t scene, uint_fast8_t camIndex);
        void switchScene();
        void switchStudioMode();
        void selectCam(int camIndex);
        void prevCam();
        void nextCam();

        //ptz signals
        void moveUp();
        void moveDown();
        void moveLeft();
        void moveRight();
        void zoomOut();
        void zoomIn();
        void ptzStop();
        void focusFar();
        void focusNear();
        void focusStop();
        void focusAuto();

        //preset signals
        void callPreset(unsigned presetNo);
        void setPreset(unsigned presetNo);

    public slots:
        void setCurScene(uint_fast8_t scene, int camId);
        void setCamIndex(int cam);
        void setStudioMode(bool en);

    private:
        void connectStreamDeck();
        void createKeyHandlers();

        friend StreamDeckKey;
        friend StreamDeckKey_Switch;
        friend StreamDeckKey_Tally;
        friend StreamDeckKey_LongPress;
        friend StreamDeckKey_Preset;
        void sendRequest(const char* event, QJsonObject&& payload = QJsonObject());
        void setPage(int page);
        void clearButton(int page, int row, int column);

    private slots:
        void processStreamDeckMsg(const QString& msg);
        void onDisconnect();
        void presetPrevPage();
        void presetNextPage();

    private:
        const StreamDeckSettings& settings;
        const std::vector<CameraSettings>& CAMERAS;

        QJsonValue uuid;
        QString deckId;

        static constexpr size_t NUM_PAGE = 3;
        static constexpr size_t NUM_ROW = 4;
        static constexpr size_t NUM_COLUMN = 8;
        StreamDeckKey* key[3][4][8] = {};

        uint_fast8_t curScene = 0;
        int curCamIndex = 0; //Active Cam
        int camIndex = 0;    //Preview Cam
        std::map<uint_fast8_t, StreamDeckKey_Scene*> sceneKeyMap; //scene->key
        std::vector<StreamDeckKey_Tally*> cameraKeyMap; //camIndex->key
        StreamDeckKey_Tally* switchCamKey = nullptr;
        StreamDeckKey_Switch* prevCamKey = nullptr;
        StreamDeckKey_Switch* nextCamKey = nullptr;

        bool isStudioMode = 0;
        StreamDeckKey_Switch* studioModeKey = nullptr;

        //ptz keys
        StreamDeckKey* moveUpKey = nullptr;
        StreamDeckKey* moveDownKey = nullptr;
        StreamDeckKey* moveLeftKey = nullptr;
        StreamDeckKey* moveRightKey = nullptr;
        StreamDeckKey* zoomOutKey = nullptr;
        StreamDeckKey* zoomInKey = nullptr;
        StreamDeckKey* ptzStopKey = nullptr;
        StreamDeckKey* focusFarKey = nullptr;
        StreamDeckKey* focusNearKey = nullptr;
        StreamDeckKey* focusAutoKey = nullptr;

        //preset keys
        unsigned minPresetNo = 0;
        unsigned nPresetNo = 20;
        unsigned curFirstPreset = 0;
        std::array<StreamDeckKey_Preset*,15> presetKeyMap = {};
        StreamDeckKey_Switch* prevPresetKey = nullptr;
        StreamDeckKey_Switch* nextPresetKey = nullptr;
        void updatePresetKeys();
};

