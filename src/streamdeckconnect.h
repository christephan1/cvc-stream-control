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
class StreamDeckKey_LongPress;
class StreamDeckKey_Switch;
class StreamDeckKey_Switch_LongPress;
class StreamDeckKey_Scene;
class StreamDeckKey_Tally;
class StreamDeckKey_Preset;

class StreamDeckConnect : public QWebSocket {
    Q_OBJECT
    public:
        StreamDeckConnect(const StreamDeckSettings&, const std::vector<CameraSettings>&, const MatrixSettings&);
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

        //menu signals
        void menuPressed();
        void menuUp();
        void menuDown();
        void menuLeft();
        void menuRight();
        void menuEnter();
        void menuBack();
        void camOn();
        void camOff();

        //camera features signals
        void autoFramingOn();
        void autoFramingOff();

        //caption source selection signals
        void captionSourceCaption();
        void captionSourceProjector();
        void captionSourceLectern();
        void captionSource1F();
        void captionSourceB1();

        //matrix signals
        void matrixSwitchChannel(unsigned src, unsigned dst);

    public slots:
        void setCurScene(uint_fast8_t scene, int camId);
        void setCamIndex(int cam);
        void setStudioMode(bool en);

    private:
        void connectStreamDeck();
        void createKeyHandlers();

        friend StreamDeckKey;
        friend StreamDeckKey_LongPress;
        friend StreamDeckKey_Switch;
        friend StreamDeckKey_Switch_LongPress;
        friend StreamDeckKey_Tally;
        friend StreamDeckKey_Preset;
        void sendRequest(const char* event, QJsonObject&& payload = QJsonObject());
        void setPage(int page);
        void clearButton(int page, int row, int column);

    private slots:
        void processStreamDeckMsg(const QString& msg);
        void onDisconnect();
        void presetPrevPage();
        void presetNextPage();
        void selectMatrixInput(unsigned input);
        void selectMatrixOutput(unsigned output);

    private:
        const StreamDeckSettings& settings;
        const std::vector<CameraSettings>& CAMERAS;
        const MatrixSettings& MATRIX;

        QJsonValue uuid;
        QString deckId;

        static constexpr size_t NUM_PAGE = 4;
        static constexpr size_t NUM_ROW = 4;
        static constexpr size_t NUM_COLUMN = 8;
        StreamDeckKey* key[NUM_PAGE][NUM_ROW][NUM_COLUMN] = {};

        uint_fast8_t curScene = 0;
        int curCamIndex = 0; //Active Cam
        int camIndex = 0;    //Preview Cam
        std::map<uint_fast8_t, StreamDeckKey_Scene*> sceneKeyMap; //scene->key
        std::vector<StreamDeckKey_Tally*> cameraKeyMap; //camIndex->key
        StreamDeckKey_Tally* switchCamKey1 = nullptr;
        StreamDeckKey_Tally* switchCamKey2 = nullptr;
        StreamDeckKey_Switch* prevCamKey1 = nullptr;
        StreamDeckKey_Switch* prevCamKey2 = nullptr;
        StreamDeckKey_Switch* nextCamKey1 = nullptr;
        StreamDeckKey_Switch* nextCamKey2 = nullptr;

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

        //menu keys
        StreamDeckKey* menuKey = nullptr;
        StreamDeckKey* menuUpKey = nullptr;
        StreamDeckKey* menuDownKey = nullptr;
        StreamDeckKey* menuLeftKey = nullptr;
        StreamDeckKey* menuRightKey = nullptr;
        StreamDeckKey* menuEnterKey = nullptr;
        StreamDeckKey* menuBackKey = nullptr;
        StreamDeckKey* camOnKey = nullptr;
        StreamDeckKey* camOffKey = nullptr;

        //camara features keys
        StreamDeckKey* autoFramingOnKey = nullptr;
        StreamDeckKey* autoFramingOffKey = nullptr;

        //caption source selection keys
        StreamDeckKey* captionSourceCaptionKey = nullptr;
        StreamDeckKey* captionSourceProjectorKey = nullptr;
        StreamDeckKey* captionSourceLecternKey = nullptr;
        StreamDeckKey* captionSource1FKey = nullptr;
        StreamDeckKey* captionSourceB1Key = nullptr;

        //matrix ports keys
        std::vector<StreamDeckKey_Switch_LongPress*> matrixInputKeys;
        std::vector<StreamDeckKey_Switch_LongPress*> matrixOutputKeys;
        int selectedMatrixInput = -1;
        int selectedMatrixOutput = -1;
};
