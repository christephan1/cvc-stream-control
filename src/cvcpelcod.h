// vim:ts=4:sw=4:et:cin

#ifndef CVCPELCOD_H
#define CVCPELCOD_H

#include <QMainWindow>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <queue>
#include <functional>
#include "cvcsetting.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CVCPelcoD; }
class QGamepad;
class QPushButton;
class QUdpSocket;
class QTimer;
QT_END_NAMESPACE

class OBSConnect;
class CameraConnect;
class StreamDeckConnect;

class CVCPelcoD : public QMainWindow
{
    Q_OBJECT

public:
    CVCPelcoD(QWidget *parent = nullptr);
    ~CVCPelcoD();

private slots:
    void selectPrevCam(bool en);
    void selectNextCam(bool en);
    void selectCam(int camIndex);
    void selectPreset();
    void callPreset(bool en);
    void setPreset(bool en);

    void selectPrevOBSScene(bool en);
    void selectNextOBSScene(bool en);
    void selectOBSScene1(bool en);
    void selectOBSScene2(bool en);
    void selectOBSScene(uint_fast8_t sceneId, uint_fast8_t);
    void switchOBSScene(bool en);
    void switchOBSStudioMode(bool en);

    void ptzCam();
    void zoomCam();
    void focusCam();

    //stream deck commands - No cache
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
    void callPresetByNo(unsigned presetNo);
    void setPresetByNo(unsigned presetNo);
    void menuPressed();
    void menuUp();
    void menuDown();
    void menuLeft();
    void menuRight();
    void menuEnter();
    void menuBack();
    void camOn();
    void camOff();
    void autoFramingOn();
    void autoFramingOff();

    void execNextCommand();

private:
    Ui::CVCPelcoD *ui;
    QGamepad *gamepad = nullptr;
    OBSConnect *obsConnect = nullptr;
    StreamDeckConnect *streamDeckConnect = nullptr;
    CVCSettings settings;

    std::array<QPushButton*,11> obsScene;
    unsigned curScene = 0;

    std::map<int,bool> isCamMoving;// = {{2,false}, {3,false}, {4,false}};
    std::map<int,bool> isManualFocus;// = {{2,false}, {3,false}, {4,false}};
    int camIndex = -1;

    int prevZoomValue = 0;
    int prevFocusValue = 0;
    int prevX = 0;
    int prevY = 0;
    int prevZoomSpeed = 0;
    int prevCam = 0;

    // Socket related
    std::vector<CameraConnect*> cameraConnect;
    QTimer* udp_timeout;
    std::queue<std::function<bool()>> cmdQueue;
    bool is_sock_idle = true;
    bool is_zoom_in_queue = false;
    bool is_focus_in_queue = false;
    bool is_ptz_in_queue = false;
    bool is_manual_focus_in_queue = false;
    bool is_auto_focus_in_queue = false;
    bool is_call_preset_in_queue = false;
    bool is_set_preset_in_queue = false;
    void addCommandToQueue(const std::function<bool()>&);
};
#endif // CVCPELCOD_H
