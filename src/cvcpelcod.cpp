// vim:ts=4:sw=4:et:cin

#include <unistd.h>
#include <QtGamepad/QGamepad>
#include <QTimer>
#include "cvcpelcod.h"
#include "ui_cvcpelcod.h"
#include "visca.h"
#include "obsconnect.h"

CVCPelcoD::CVCPelcoD(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CVCPelcoD)
{
    ui->setupUi(this);
    ui->camNo->setNum(camId);
    ui->sceneNo->setText(QString::number(curScene+1));
    auto gamepads = QGamepadManager::instance()->connectedGamepads();
    if (gamepads.isEmpty()) {
        ui->statusbar->showMessage("No gamepad connected.");
    } else {
        gamepad = new QGamepad(*gamepads.begin(), this);
        ui->statusbar->showMessage("Gamepad connected.");

        connect(gamepad,&QGamepad::axisLeftXChanged, this, &CVCPelcoD::ptzCam);
        connect(gamepad,&QGamepad::axisLeftYChanged, this, &CVCPelcoD::ptzCam);
        connect(gamepad,&QGamepad::buttonL2Changed, this, &CVCPelcoD::zoomCam);
        connect(gamepad,&QGamepad::buttonR2Changed, this, &CVCPelcoD::zoomCam);
        connect(gamepad,&QGamepad::buttonYChanged, this, &CVCPelcoD::focusCam);
        connect(gamepad,&QGamepad::buttonAChanged, this, &CVCPelcoD::focusCam);
        connect(gamepad,&QGamepad::buttonL1Changed, this, &CVCPelcoD::selectPrevCam);
        connect(gamepad,&QGamepad::buttonR1Changed, this, &CVCPelcoD::selectNextCam);
        connect(gamepad,&QGamepad::axisRightXChanged, this, &CVCPelcoD::selectPreset);
        connect(gamepad,&QGamepad::axisRightYChanged, this, &CVCPelcoD::selectPreset);
        connect(gamepad,&QGamepad::buttonBChanged, this, &CVCPelcoD::callPreset);
        connect(gamepad,&QGamepad::buttonStartChanged, this, &CVCPelcoD::setPreset);
        connect(gamepad,&QGamepad::buttonLeftChanged, this, &CVCPelcoD::selectPrevOBSScene);
        connect(gamepad,&QGamepad::buttonRightChanged, this, &CVCPelcoD::selectNextOBSScene);
        connect(gamepad,&QGamepad::buttonDownChanged, this, &CVCPelcoD::selectOBSScene1);
        connect(gamepad,&QGamepad::buttonUpChanged, this, &CVCPelcoD::selectOBSScene2);
        connect(gamepad,&QGamepad::buttonXChanged, this, &CVCPelcoD::switchOBSScene);
        connect(gamepad,&QGamepad::buttonGuideChanged, this, &CVCPelcoD::switchOBSStudioMode);

        udp_sock = new QUdpSocket (this);
        bool r = udp_sock->bind(QHostAddress(LOCAL_IP));
        connect(udp_sock,&QUdpSocket::readyRead, this, &CVCPelcoD::execNextCommand);
        setUdpSock(udp_sock);

        udp_timeout = new QTimer (this);
        connect(udp_timeout,&QTimer::timeout, this, &CVCPelcoD::execNextCommand);
    }

    obsScene = {{
        ui->obsScene1,
        ui->obsScene2,
        ui->obsScene3,
        ui->obsScene4,
        ui->obsScene5,
        ui->obsScene6,
        ui->obsScene7,
        ui->obsScene8,
        ui->obsScene9,
        ui->obsScene10,
        ui->obsScene11
    }};
    obsConnect = new OBSConnect;
    connect(obsConnect, &OBSConnect::updateStatus, this, [this](const QString& str) {ui->statusbar->showMessage(str);});
    connect(obsConnect, &OBSConnect::currentSceneChanged, this, &CVCPelcoD::selectOBSScene);
}

CVCPelcoD::~CVCPelcoD()
{
    if(gamepad) delete gamepad;
    if(obsConnect) delete obsConnect;
    delete ui;
}

void CVCPelcoD::selectPrevCam(bool en)
{
    if (en) {
        if (camId > MIN_CAM_ID) {
            ui->camNo->setNum(--camId);
        }
    }
}

void CVCPelcoD::selectNextCam(bool en)
{
    if (en) {
        if (camId < MAX_CAM_ID) {
            ui->camNo->setNum(++camId);
        }
    }
}

void CVCPelcoD::selectPreset()
{
    double axisX = gamepad->axisRightX();
    double axisY = gamepad->axisRightY();

    static QTimer *timerAdd1 = nullptr;
    static QTimer *timerSub1 = nullptr;
    static QTimer *timerAdd10 = nullptr;
    static QTimer *timerSub10 = nullptr;

    //+1
    if (true) {
        if (axisX > 0.5) {
            if (!timerAdd1) {
                auto add1 = [this] () {
                    int presetNo = ui->presetNo->text().toInt();
                    if (presetNo >= MAX_PRESET_NO) return;
                    ui->presetNo->setNum(presetNo+1);
                };
                timerAdd1 = new QTimer (this);
                connect (timerAdd1, &QTimer::timeout, this, add1);
                add1();
                timerAdd1 -> start (150);
            }
        } else {
            if (timerAdd1) {
                delete timerAdd1;
                timerAdd1 = nullptr;
            }
        }
    }

    //-1
    if (true) {
        if (axisX < -0.5) {
            if (!timerSub1) {
                auto sub1 = [this] () {
                    int presetNo = ui->presetNo->text().toInt();
                    if (presetNo <= MIN_PRESET_NO) return;
                    ui->presetNo->setNum(presetNo-1);
                };
                timerSub1 = new QTimer (this);
                connect (timerSub1, &QTimer::timeout, this, sub1);
                sub1();
                timerSub1 -> start (150);
            }
        } else {
            if (timerSub1) {
                delete timerSub1;
                timerSub1 = nullptr;
            }
        }
    }

    //+10
    if (true) {
        if (axisY > 0.5) {
            if (!timerAdd10) {
                auto add10 = [this] () {
                    int presetNo = ui->presetNo->text().toInt();
                    if (presetNo > MAX_PRESET_NO-10) return;
                    ui->presetNo->setNum(presetNo+10);
                };
                timerAdd10 = new QTimer (this);
                connect (timerAdd10, &QTimer::timeout, this, add10);
                add10();
                timerAdd10 -> start (150);
            }
        } else {
            if (timerAdd10) {
                delete timerAdd10;
                timerAdd10 = nullptr;
            }
        }
    }

    //-10
    if (true) {
        if (axisY < -0.5) {
            if (!timerSub10) {
                auto sub10 = [this] () {
                    int presetNo = ui->presetNo->text().toInt();
                    if (presetNo < MIN_PRESET_NO+10) return;
                    ui->presetNo->setNum(presetNo-10);
                };
                timerSub10 = new QTimer (this);
                connect (timerSub10, &QTimer::timeout, this, sub10);
                sub10();
                timerSub10 -> start (150);
            }
        } else {
            if (timerSub10) {
                delete timerSub10;
                timerSub10 = nullptr;
            }
        }
    }
}

void CVCPelcoD::execNextCommand()
{
    udp_timeout->stop();
    if (!cmdQueue.empty()) {
        bool isCmdSent = cmdQueue.front()();
        cmdQueue.pop();
        if (isCmdSent) {
            udp_timeout->start (500);
        } else {
            execNextCommand();
        }
    } else {
        is_sock_idle = true;
    }
}

void CVCPelcoD::addCommandToQueue(const std::function<bool()>& f)
{
    cmdQueue.push(f);
    if (is_sock_idle) {
        is_sock_idle = false;
        execNextCommand();
    }
    return;
}

void CVCPelcoD::zoomCam()
{
    if (is_zoom_in_queue) return;
    auto zoomFn = [this] () -> bool {
        is_zoom_in_queue = false;
        unsigned zoomSpeedRange = MAX_ZOOM_SPEED - MIN_ZOOM_SPEED + 1;
        double zoomIn  = gamepad->buttonR2();
        double zoomOut = gamepad->buttonL2();
        double zoom = zoomIn - zoomOut;
        int zoomValue = ((zoom > 0)? 1 : (zoom < 0)? -1 : 0);

        double zoomSpeed = zoom;
        if (zoomSpeed < 0) zoomSpeed = -zoomSpeed;
        int newZoomSpeed = MIN_ZOOM_SPEED + static_cast<int>(zoomSpeed * zoomSpeedRange + 0.5) - 1;
        if (newZoomSpeed < MIN_ZOOM_SPEED) newZoomSpeed = MIN_ZOOM_SPEED;

        if (camId != prevCam || newZoomSpeed != prevZoomSpeed || zoomValue != prevZoomValue) {
            //if (isManualFocus[camId]) {
                //viscaAutoFocus(camId);
                //isManualFocus[camId] = false;
            //}

            if (zoomValue > 0) {
                viscaIn (camId, newZoomSpeed);
                ui->statusbar->showMessage(QString("Zoom In Speed: ") + QString::number(newZoomSpeed));
            } else if (zoomValue < 0) {
                viscaOut (camId, newZoomSpeed);
                ui->statusbar->showMessage(QString("Zoom Out Speed: ") + QString::number(newZoomSpeed));
            } else {
                viscaZoomStop (camId);
                ui->statusbar->showMessage(QString("Stop Zoom"));
            }
            
            prevZoomValue = zoomValue;
            prevZoomSpeed = newZoomSpeed;
            prevCam = camId;
            return true;
        } else {
            return false;
        }
        //ui->statusbar->showMessage(QString("Zoom In = ") + QString::number(zoomIn) + " Zoom Out = " + QString::number(zoomOut));
    };
    is_zoom_in_queue = true;
    addCommandToQueue(zoomFn);
}

void CVCPelcoD::focusCam()
{
    if (!is_manual_focus_in_queue) {
        is_manual_focus_in_queue = true;
        addCommandToQueue ([this] () -> bool {
            is_manual_focus_in_queue = false;
            viscaManualFocus(camId);
            isManualFocus[camId] = true;
            return true;
        });
    }

    if (!is_focus_in_queue) {
        is_focus_in_queue = true;
        addCommandToQueue ([this] () -> bool {
            is_focus_in_queue = false;
            bool focusFar  = gamepad->buttonY();
            bool focusNear = gamepad->buttonA();
            if (focusFar == focusNear) focusFar = focusNear = false;
            int focusValue = (focusFar? 1 : focusNear? -1 : 0);

            if (camId != prevCam || focusValue != prevFocusValue) {
                if (focusFar) {
                    viscaFar (camId, MIN_FOCUS_SPEED);
                } else if (focusNear) {
                    viscaNear (camId, MIN_FOCUS_SPEED);
                } else {
                    viscaZoomStop (camId);
                }
                
                prevFocusValue = focusValue;
                prevCam = camId;
                return true;
            } else {
                return false;
            }
        });
    }
}

void CVCPelcoD::ptzCam()
{
    if (!is_ptz_in_queue) {
        is_ptz_in_queue = true;
        addCommandToQueue ([this] () -> bool {
            is_ptz_in_queue = false;
            double moveX = gamepad->axisLeftX();
            double moveY = gamepad->axisLeftY();
            int newMoveX = (MAX_PAN_SPEED - MIN_PAN_SPEED + 1) * moveX;
            int newMoveY = (MAX_TILT_SPEED - MIN_TILT_SPEED + 1) * moveY;
            if (moveX > 0) {
                newMoveX += MIN_PAN_SPEED - 1;
                if (newMoveX < MIN_PAN_SPEED) newMoveX = MIN_PAN_SPEED;
            }
            if (moveY > 0) {
                newMoveY += MIN_TILT_SPEED - 1;
                if (newMoveY < MIN_TILT_SPEED) newMoveY = MIN_TILT_SPEED;
            }
            if (moveX < 0) {
                newMoveX -= MIN_PAN_SPEED - 1;
                if (newMoveX > -MIN_PAN_SPEED) newMoveX = -MIN_PAN_SPEED;
            }
            if (moveY < 0) {
                newMoveY -= MIN_TILT_SPEED - 1;
                if (newMoveY > -MIN_TILT_SPEED) newMoveY = -MIN_TILT_SPEED;
            }

            if (newMoveX == 0 && newMoveY == 0) {
                if (isCamMoving[camId]) {
                    viscaStop(camId);
                    ui->statusbar->showMessage("Move STOP");
                    isCamMoving[camId] = false;
                    prevCam = camId;
                    prevX = newMoveX;
                    prevY = newMoveY;
                    return true;
                } else {
                    return false;
                }
            } else {
                if (camId != prevCam || newMoveX != prevX || newMoveY != prevY) {
                    if (!is_auto_focus_in_queue) {
                        is_auto_focus_in_queue = true;
                        addCommandToQueue ([this] () -> bool {
                            is_auto_focus_in_queue = false;
                            if (isManualFocus[camId]) {
                                viscaAutoFocus(camId);
                                isManualFocus[camId] = false;
                                return true;
                            } else {
                                return false;
                            }
                        });
                    }
                    viscaMove(camId, newMoveX, newMoveY);
                    ui->statusbar->showMessage(QString("X Speed = ") + QString::number(newMoveX) + " Y Speed = " + QString::number(newMoveY));
                    isCamMoving[camId] = true;
                    prevCam = camId;
                    prevX = newMoveX;
                    prevY = newMoveY;
                    return true;
                } else {
                    return false;
                }
            }
            //ui->statusbar->showMessage(QString("MoveX = ") + QString::number(moveX) + " MoveY = " + QString::number(moveY));
        });
    }
}

void CVCPelcoD::callPreset(bool en)
{
    if (en) {
        if (!is_call_preset_in_queue) {
            is_call_preset_in_queue = true;
            addCommandToQueue ([this] () -> bool {
                is_call_preset_in_queue = false;
                int presetNo = ui->presetNo->text().toInt();
                viscaGo(camId,presetNo);
                ui->statusbar->showMessage("CALL");
                return true;
            });
        }
    }
}

void CVCPelcoD::setPreset(bool en)
{
    if (en) {
        if (!is_set_preset_in_queue) {
            is_set_preset_in_queue = true;
            addCommandToQueue ([this] () -> bool {
                is_set_preset_in_queue = false;
                int presetNo = ui->presetNo->text().toInt();
                viscaSet(camId,presetNo);
                ui->statusbar->showMessage("PRESET");
                return true;
            });
        }
    }
}

void CVCPelcoD::selectPrevOBSScene(bool en)
{
    if (gamepad->buttonRight()) en = false;

    static QTimer *timerPrevOBSScene = nullptr;
    if (en) {
        if (!timerPrevOBSScene) {
            auto selectPrev = [this] () {
                uint_fast8_t prevScene = obsConnect->getPrevSceneId(curScene+1);
                if (prevScene == 0) return;
                if (curScene < obsScene.size()) obsScene[curScene]->setChecked(false);
                curScene = prevScene-1;
                if (curScene < obsScene.size()) obsScene[curScene]->setChecked(true);
                ui->sceneNo->setText(QString::number(curScene+1));
            };
            timerPrevOBSScene = new QTimer (this);
            connect (timerPrevOBSScene, &QTimer::timeout, this, selectPrev);
            selectPrev();
            timerPrevOBSScene -> start (350);
        }
    } else {
        if (timerPrevOBSScene) {
            delete timerPrevOBSScene;
            timerPrevOBSScene = nullptr;
        }
    }
}

void CVCPelcoD::selectNextOBSScene(bool en)
{
    if (gamepad->buttonLeft()) en = false;

    static QTimer *timerNextOBSScene = nullptr;
    if (en) {
        if (!timerNextOBSScene) {
            auto selectNext = [this] () {
                uint_fast8_t nextScene = obsConnect->getNextSceneId(curScene+1);
                if (nextScene == 0) return;
                if (curScene < obsScene.size()) obsScene[curScene]->setChecked(false);
                curScene = nextScene-1;
                if (curScene < obsScene.size()) obsScene[curScene]->setChecked(true);
                ui->sceneNo->setText(QString::number(curScene+1));
            };
            timerNextOBSScene = new QTimer (this);
            connect (timerNextOBSScene, &QTimer::timeout, this, selectNext);
            selectNext();
            timerNextOBSScene -> start (350);
        }
    } else {
        if (timerNextOBSScene) {
            delete timerNextOBSScene;
            timerNextOBSScene = nullptr;
        }
    }
}

void CVCPelcoD::selectOBSScene1(bool en)
{
    if (en) {
        if (curScene < obsScene.size()) obsScene[curScene]->setChecked(false);
        curScene = (curScene == 1? 0 : 1);
        if (curScene < obsScene.size()) obsScene[curScene]->setChecked(true);
        ui->sceneNo->setText(QString::number(curScene+1));
    }
}

void CVCPelcoD::selectOBSScene2(bool en)
{
    if (en) {
        if (curScene < obsScene.size()) obsScene[curScene]->setChecked(false);
        curScene = (curScene == 2? 0 : 2);
        if (curScene < obsScene.size()) obsScene[curScene]->setChecked(true);
        ui->sceneNo->setText(QString::number(curScene+1));
    }
}

void CVCPelcoD::selectOBSScene(uint_fast8_t sceneId, uint_fast8_t)
{
    if (sceneId == 0) return;
    if (curScene < obsScene.size()) obsScene[curScene]->setChecked(false);
    curScene = sceneId-1;
    if (curScene < obsScene.size()) obsScene[curScene]->setChecked(true);
    ui->sceneNo->setText(QString::number(curScene+1));
}

void CVCPelcoD::switchOBSScene(bool en)
{
    if (en) {
        obsConnect->switchToScene(curScene+1, camId);
    }
}

void CVCPelcoD::switchOBSStudioMode(bool en)
{
    if (en) {
        obsConnect->switchStudioMode();
    }
}

