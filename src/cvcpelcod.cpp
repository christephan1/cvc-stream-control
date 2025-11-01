// vim:ts=4:sw=4:et:cin

#ifdef _WIN32
// Windows-specific includes if needed
#else
#include <unistd.h>
#endif
#include <QtGamepad/QGamepad>
#include <QTimer>
#include <QDir>
#include <QMessageBox>
#include <QCloseEvent>
#include "cvcpelcod.h"
#include "ui_cvcpelcod.h"
#include "visca.h"
#include "obsconnect.h"
#include "streamdeckconnect.h"
#include "matrixconnect.h"

CVCPelcoD::CVCPelcoD(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CVCPelcoD)
{
    ui->setupUi(this);
    try {
        settings.parseJSON(QDir::homePath() + "/.cvc-stream-control.conf");
    } catch (const std::exception &e) {
        ui->statusbar->showMessage(e.what());
        return;
    }

    if (settings.CAMERAS.empty()) {
        camIndex = -1;
        ui->camNo->setText("");
    } else {
        camIndex = 0;
        ui->camNo->setNum(settings.CAMERAS[camIndex].CAMERA_ID);
    }
    ui->sceneNo->setText(QString::number(curScene+1));

    //Gamepad
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
    }

    //Camera
    for (const CameraSettings& camera : settings.CAMERAS) {
        CameraConnect* connection = new CameraConnect(camera, this);
        cameraConnect.push_back(connection);
        connect(connection,&QUdpSocket::readyRead, this, &CVCPelcoD::execNextCommand);
    }
    cameraConnect.shrink_to_fit();
    udp_timeout = new QTimer (this);
    connect(udp_timeout,&QTimer::timeout, this, &CVCPelcoD::execNextCommand);

    //OBS
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
    obsConnect = new OBSConnect(settings.OBS);
    connect(obsConnect, &OBSConnect::updateStatus, this, [this](const QString& str) {ui->statusbar->showMessage(str);});
    connect(obsConnect, &OBSConnect::currentSceneChanged, this, &CVCPelcoD::selectOBSScene);

    //Stream Deck
    streamDeckConnect = new StreamDeckConnect(settings.STREAM_DECK, settings.CAMERAS, settings.MATRIX);
    connect(streamDeckConnect, &StreamDeckConnect::updateStatus, this, [this](const QString& str) {ui->statusbar->showMessage(str);});

    connect(obsConnect, &OBSConnect::currentSceneChanged, streamDeckConnect, &StreamDeckConnect::setCurScene);
    connect(streamDeckConnect, &StreamDeckConnect::sceneChanged, this, &CVCPelcoD::selectOBSScene);
    connect(streamDeckConnect, &StreamDeckConnect::switchScene, this, [this]() {switchOBSScene(true);});

    connect(obsConnect, &OBSConnect::studioModeChanged, streamDeckConnect, &StreamDeckConnect::setStudioMode);
    connect(streamDeckConnect, &StreamDeckConnect::switchStudioMode, this, [this]() {switchOBSStudioMode(true);});

    connect(streamDeckConnect, &StreamDeckConnect::selectCam, this, &CVCPelcoD::selectCam);
    connect(streamDeckConnect, &StreamDeckConnect::prevCam, this, [this]() {selectPrevCam(true);});
    connect(streamDeckConnect, &StreamDeckConnect::nextCam, this, [this]() {selectNextCam(true);});

    connect(streamDeckConnect, &StreamDeckConnect::moveUp,    this, &CVCPelcoD::moveUp);
    connect(streamDeckConnect, &StreamDeckConnect::moveDown,  this, &CVCPelcoD::moveDown);
    connect(streamDeckConnect, &StreamDeckConnect::moveLeft,  this, &CVCPelcoD::moveLeft);
    connect(streamDeckConnect, &StreamDeckConnect::moveRight, this, &CVCPelcoD::moveRight);
    connect(streamDeckConnect, &StreamDeckConnect::zoomOut,   this, &CVCPelcoD::zoomOut);
    connect(streamDeckConnect, &StreamDeckConnect::zoomIn,    this, &CVCPelcoD::zoomIn);
    connect(streamDeckConnect, &StreamDeckConnect::ptzStop,   this, &CVCPelcoD::ptzStop);
    connect(streamDeckConnect, &StreamDeckConnect::focusFar,  this, &CVCPelcoD::focusFar);
    connect(streamDeckConnect, &StreamDeckConnect::focusNear, this, &CVCPelcoD::focusNear);
    connect(streamDeckConnect, &StreamDeckConnect::focusStop, this, &CVCPelcoD::focusStop);
    connect(streamDeckConnect, &StreamDeckConnect::focusAuto, this, &CVCPelcoD::focusAuto);

    connect(streamDeckConnect, &StreamDeckConnect::callPreset, this, &CVCPelcoD::callPresetByNo);
    connect(streamDeckConnect, &StreamDeckConnect::setPreset,  this, &CVCPelcoD::setPresetByNo);

    connect(streamDeckConnect, &StreamDeckConnect::menuPressed, this, &CVCPelcoD::menuPressed);
    connect(streamDeckConnect, &StreamDeckConnect::menuUp,      this, &CVCPelcoD::menuUp);
    connect(streamDeckConnect, &StreamDeckConnect::menuDown,    this, &CVCPelcoD::menuDown);
    connect(streamDeckConnect, &StreamDeckConnect::menuLeft,    this, &CVCPelcoD::menuLeft);
    connect(streamDeckConnect, &StreamDeckConnect::menuRight,   this, &CVCPelcoD::menuRight);
    connect(streamDeckConnect, &StreamDeckConnect::menuEnter,   this, &CVCPelcoD::menuEnter);
    connect(streamDeckConnect, &StreamDeckConnect::menuBack,    this, &CVCPelcoD::menuBack);
    connect(streamDeckConnect, &StreamDeckConnect::camOn,       this, &CVCPelcoD::camOn);
    connect(streamDeckConnect, &StreamDeckConnect::camOff,      this, &CVCPelcoD::camOff);

    connect(streamDeckConnect, &StreamDeckConnect::autoFramingOn,  this, &CVCPelcoD::autoFramingOn);
    connect(streamDeckConnect, &StreamDeckConnect::autoFramingOff, this, &CVCPelcoD::autoFramingOff);

    streamDeckConnect->setCamIndex(camIndex);

    //Matrix
    matrixConnect = new MatrixConnect(settings.MATRIX);
    connect(matrixConnect,     &MatrixConnect::updateStatus,     this, [this](const QString& str) {ui->statusbar->showMessage(str);});
    connect(streamDeckConnect, &StreamDeckConnect::matrixSwitchChannel, matrixConnect,     &MatrixConnect::switchChannel);
    connect(streamDeckConnect, &StreamDeckConnect::matrixExecMacro,     matrixConnect,     &MatrixConnect::execMacro);
    connect(streamDeckConnect, &StreamDeckConnect::matrixReset,         matrixConnect,     &MatrixConnect::resetMatrix);
    connect(streamDeckConnect, &StreamDeckConnect::matrixGetMapping,    matrixConnect,     &MatrixConnect::getMapping);
    connect(matrixConnect,     &MatrixConnect::mappingUpdated,          streamDeckConnect, &StreamDeckConnect::matrixUpdateMapping);
    connect(matrixConnect,     &MatrixConnect::connectionFailed,        this,              &CVCPelcoD::onMatrixConnectionFailed);
    connect(matrixConnect,     &MatrixConnect::addOBSSceneOverrides,    obsConnect,        &OBSConnect::addSceneOverrides);
    connect(matrixConnect,     &MatrixConnect::clearOBSSceneOverrides,  obsConnect,        &OBSConnect::clearSceneOverrides);
}

CVCPelcoD::~CVCPelcoD()
{
    if(obsConnect) delete obsConnect;
    //[TODO] this will call seg Fault
    //if(streamDeckConnect) delete streamDeckConnect;
    if(matrixConnect) delete matrixConnect;
    delete ui;
}

void CVCPelcoD::closeEvent(QCloseEvent *event)
{
    if (final_close) {
        event->accept();
        return;
    }
    if (is_shutting_down) {
        event->ignore();
        return;
    }

    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "CVC Stream Control",
                                                                tr("Turn off all cameras and reset matrix?\n"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes) {
        is_shutting_down = true;
        setEnabled(false);
        ui->statusbar->showMessage("Shutting down...");

        // 1. clear pending commands
        std::queue<std::function<bool()>> empty;
        cmdQueue.swap(empty);
        udp_timeout->stop();
        is_sock_idle = true;

        // 2. wait for 0.5 seconds.
        QTimer::singleShot(UDP_TIMEOUT_MS, this, [this]() {
            // 3. Turn off cameras directly and reset matrix.
            for (CameraConnect* conn : cameraConnect) {
                conn->viscaOff();
            }
            if (matrixConnect) {
                matrixConnect->resetMatrix();
            }

            // 4. Wait a bit more for packets to be sent before closing.
            QTimer::singleShot(UDP_TIMEOUT_MS, this, [this]() {
                final_close = true;
                close();
            });
        });

        event->ignore();
    } else if (resBtn == QMessageBox::No) {
        event->accept();
    } else {
        event->ignore();
    }
}

void CVCPelcoD::startup()
{
    for (size_t i = 0; i < cameraConnect.size(); ++i) {
        addCommandToQueue([this, i]() -> bool {
            cameraConnect[i]->viscaOn();
            return true;
        });
    }

    if (matrixConnect) matrixConnect->resetMatrix();
}

void CVCPelcoD::selectPrevCam(bool en)
{
    if (en) {
        if (!settings.CAMERAS.empty() && camIndex > 0) {
            ui->camNo->setNum(settings.CAMERAS[--camIndex].CAMERA_ID);
            if(streamDeckConnect) streamDeckConnect->setCamIndex(camIndex);
        }
    }
}

void CVCPelcoD::selectNextCam(bool en)
{
    if (en) {
        if (!settings.CAMERAS.empty() && camIndex < settings.CAMERAS.size()-1) {
            ui->camNo->setNum(settings.CAMERAS[++camIndex].CAMERA_ID);
            if(streamDeckConnect) streamDeckConnect->setCamIndex(camIndex);
        }
    }
}

void CVCPelcoD::selectCam(int camIndex_)
{
    if (camIndex_ >= 0 && camIndex_ < settings.CAMERAS.size()) {
        ui->camNo->setNum(settings.CAMERAS[camIndex = camIndex_].CAMERA_ID);
        if(streamDeckConnect) streamDeckConnect->setCamIndex(camIndex);
    }
}

void CVCPelcoD::selectPreset()
{
    if (settings.CAMERAS.empty()) return;
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
                    if (presetNo >= settings.CAMERAS[camIndex].MAX_PRESET_NO) return;
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
                    if (presetNo <= settings.CAMERAS[camIndex].MIN_PRESET_NO) return;
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
                    if (presetNo > settings.CAMERAS[camIndex].MAX_PRESET_NO-10) return;
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
                    if (presetNo < settings.CAMERAS[camIndex].MIN_PRESET_NO+10) return;
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
            udp_timeout->start (CVCPelcoD::UDP_TIMEOUT_MS);
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
    if (settings.CAMERAS.empty()) return;
    if (is_zoom_in_queue) return;
    auto zoomFn = [this] () -> bool {
        is_zoom_in_queue = false;
        unsigned MAX_ZOOM_SPEED = settings.CAMERAS[camIndex].MAX_ZOOM_SPEED;
        unsigned MIN_ZOOM_SPEED = settings.CAMERAS[camIndex].MIN_ZOOM_SPEED;
        unsigned zoomSpeedRange = MAX_ZOOM_SPEED - MIN_ZOOM_SPEED + 1;
        double zoomIn  = gamepad->buttonR2();
        double zoomOut = gamepad->buttonL2();
        double zoom = zoomIn - zoomOut;
        int zoomValue = ((zoom > 0)? 1 : (zoom < 0)? -1 : 0);

        double zoomSpeed = zoom;
        if (zoomSpeed < 0) zoomSpeed = -zoomSpeed;
        int newZoomSpeed = MIN_ZOOM_SPEED + static_cast<int>(zoomSpeed * (zoomSpeedRange-1));
        if (newZoomSpeed < MIN_ZOOM_SPEED) newZoomSpeed = MIN_ZOOM_SPEED;
        if (newZoomSpeed > MAX_ZOOM_SPEED) newZoomSpeed = MAX_ZOOM_SPEED;

        if (camIndex != prevCam || newZoomSpeed != prevZoomSpeed || zoomValue != prevZoomValue) {
            //if (isManualFocus[camIndex]) {
                //cameraConnect[camIndex]->viscaAutoFocus();
                //isManualFocus[camIndex] = false;
            //}

            if (zoomValue > 0) {
                cameraConnect[camIndex]->viscaIn (newZoomSpeed);
                ui->statusbar->showMessage(QString("Zoom In Speed: ") + QString::number(newZoomSpeed));
            } else if (zoomValue < 0) {
                cameraConnect[camIndex]->viscaOut (newZoomSpeed);
                ui->statusbar->showMessage(QString("Zoom Out Speed: ") + QString::number(newZoomSpeed));
            } else {
                cameraConnect[camIndex]->viscaZoomStop ();
                ui->statusbar->showMessage(QString("Stop Zoom"));
            }
            
            prevZoomValue = zoomValue;
            prevZoomSpeed = newZoomSpeed;
            prevCam = camIndex;
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
    if (settings.CAMERAS.empty()) return;
    if (!is_manual_focus_in_queue) {
        is_manual_focus_in_queue = true;
        addCommandToQueue ([this] () -> bool {
            is_manual_focus_in_queue = false;
            cameraConnect[camIndex]->viscaManualFocus();
            isManualFocus[camIndex] = true;
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

            if (camIndex != prevCam || focusValue != prevFocusValue) {
                if (focusFar) {
                    cameraConnect[camIndex]->viscaFar (settings.CAMERAS[camIndex].MIN_FOCUS_SPEED);
                } else if (focusNear) {
                    cameraConnect[camIndex]->viscaNear (settings.CAMERAS[camIndex].MIN_FOCUS_SPEED);
                } else {
                    cameraConnect[camIndex]->viscaFocusStop ();
                }
                
                prevFocusValue = focusValue;
                prevCam = camIndex;
                return true;
            } else {
                return false;
            }
        });
    }
}

void CVCPelcoD::ptzCam()
{
    if (settings.CAMERAS.empty()) return;
    if (!is_ptz_in_queue) {
        is_ptz_in_queue = true;
        addCommandToQueue ([this] () -> bool {
            is_ptz_in_queue = false;
            unsigned MAX_PAN_SPEED = settings.CAMERAS[camIndex].MAX_PAN_SPEED;
            unsigned MIN_PAN_SPEED = settings.CAMERAS[camIndex].MIN_PAN_SPEED;
            unsigned MAX_TILT_SPEED = settings.CAMERAS[camIndex].MAX_TILT_SPEED;
            unsigned MIN_TILT_SPEED = settings.CAMERAS[camIndex].MIN_TILT_SPEED;
            double moveX = gamepad->axisLeftX();
            double moveY = gamepad->axisLeftY();
            int newMoveX = (MAX_PAN_SPEED - MIN_PAN_SPEED + 1) * moveX;
            int newMoveY = (MAX_TILT_SPEED - MIN_TILT_SPEED + 1) * moveY;
            if (newMoveX > 0) {
                newMoveX += MIN_PAN_SPEED - 1;
                if (newMoveX < MIN_PAN_SPEED) newMoveX = MIN_PAN_SPEED;
            }
            if (newMoveY > 0) {
                newMoveY += MIN_TILT_SPEED - 1;
                if (newMoveY < MIN_TILT_SPEED) newMoveY = MIN_TILT_SPEED;
            }
            if (newMoveX < 0) {
                newMoveX -= MIN_PAN_SPEED - 1;
                if (newMoveX > -MIN_PAN_SPEED) newMoveX = -MIN_PAN_SPEED;
            }
            if (newMoveY < 0) {
                newMoveY -= MIN_TILT_SPEED - 1;
                if (newMoveY > -MIN_TILT_SPEED) newMoveY = -MIN_TILT_SPEED;
            }

            if (newMoveX == 0 && newMoveY == 0) {
                if (isCamMoving[camIndex]) {
                    cameraConnect[camIndex]->viscaStop();
                    ui->statusbar->showMessage("Move STOP");
                    isCamMoving[camIndex] = false;
                    prevCam = camIndex;
                    prevX = newMoveX;
                    prevY = newMoveY;
                    return true;
                } else {
                    return false;
                }
            } else {
                if (camIndex != prevCam || newMoveX != prevX || newMoveY != prevY) {
                    if (!is_auto_focus_in_queue) {
                        is_auto_focus_in_queue = true;
                        addCommandToQueue ([this] () -> bool {
                            is_auto_focus_in_queue = false;
                            if (isManualFocus[camIndex]) {
                                cameraConnect[camIndex]->viscaAutoFocus();
                                isManualFocus[camIndex] = false;
                                return true;
                            } else {
                                return false;
                            }
                        });
                    }
                    cameraConnect[camIndex]->viscaMove(newMoveX, newMoveY);
                    ui->statusbar->showMessage(QString("X Speed = ") + QString::number(newMoveX) + " Y Speed = " + QString::number(newMoveY));
                    isCamMoving[camIndex] = true;
                    prevCam = camIndex;
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

void CVCPelcoD::moveUp()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaUp(7);
        ui->statusbar->showMessage("Up");
        return true;
    });
}

void CVCPelcoD::moveDown()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaDown(7);
        ui->statusbar->showMessage("Down");
        return true;
    });
}

void CVCPelcoD::moveLeft()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaLeft(7);
        ui->statusbar->showMessage("Left");
        return true;
    });
}

void CVCPelcoD::moveRight()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaRight(7);
        ui->statusbar->showMessage("Right");
        return true;
    });
}

void CVCPelcoD::zoomOut()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaOut(0);
        ui->statusbar->showMessage("Out");
        return true;
    });
}

void CVCPelcoD::zoomIn()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaIn(0);
        ui->statusbar->showMessage("In");
        return true;
    });
}

void CVCPelcoD::ptzStop()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaStop();
        return true;
    });
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaZoomStop();
        ui->statusbar->showMessage("Stop");
        return true;
    });
}

void CVCPelcoD::focusFar()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaManualFocus();
        return true;
    });
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaFar (settings.CAMERAS[camIndex].MIN_FOCUS_SPEED);
        ui->statusbar->showMessage("Far");
        return true;
    });
}

void CVCPelcoD::focusNear()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaManualFocus();
        return true;
    });
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaNear (settings.CAMERAS[camIndex].MIN_FOCUS_SPEED);
        ui->statusbar->showMessage("Near");
        return true;
    });
}

void CVCPelcoD::focusStop()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaFocusStop();
        ui->statusbar->showMessage("Stop Focus");
        return true;
    });
}

void CVCPelcoD::focusAuto()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaAutoFocus();
        ui->statusbar->showMessage("Auto Focus");
        return true;
    });
}

void CVCPelcoD::callPreset(bool en)
{
    if (settings.CAMERAS.empty()) return;
    if (en) {
        if (!is_call_preset_in_queue) {
            is_call_preset_in_queue = true;
            addCommandToQueue ([this] () -> bool {
                is_call_preset_in_queue = false;
                int presetNo = ui->presetNo->text().toInt();
                cameraConnect[camIndex]->viscaGo(presetNo);
                ui->statusbar->showMessage("CALL");
                return true;
            });
        }
    }
}

void CVCPelcoD::setPreset(bool en)
{
    if (settings.CAMERAS.empty()) return;
    if (en) {
        if (!is_set_preset_in_queue) {
            is_set_preset_in_queue = true;
            addCommandToQueue ([this] () -> bool {
                is_set_preset_in_queue = false;
                int presetNo = ui->presetNo->text().toInt();
                cameraConnect[camIndex]->viscaSet(presetNo);
                ui->statusbar->showMessage("PRESET");
                return true;
            });
        }
    }
}

void CVCPelcoD::callPresetByNo(unsigned presetNo)
{
    if (settings.CAMERAS.empty()) return;
    if (settings.CAMERAS[camIndex].CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        //Strict protocol requires to stop moving before calling preset, otherwise the call command will be ignored.
        addCommandToQueue ([this] () -> bool {
            if (isCamMoving[camIndex]) {
                cameraConnect[camIndex]->viscaStop();
                return true;
            }
        });
    }
    addCommandToQueue ([this, presetNo] () -> bool {
        cameraConnect[camIndex]->viscaGo(presetNo);
        ui->statusbar->showMessage("CALL " + QString::number(presetNo));
        return true;
    });
}

void CVCPelcoD::setPresetByNo(unsigned presetNo)
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this, presetNo] () -> bool {
        cameraConnect[camIndex]->viscaSet(presetNo);
        ui->statusbar->showMessage("PRESET " + QString::number(presetNo));
        return true;
    });
}

void CVCPelcoD::menuPressed()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenu();
        ui->statusbar->showMessage("Menu");
        return true;
    });
}

void CVCPelcoD::menuUp()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenuUp();
        ui->statusbar->showMessage("Menu Up");
        return true;
    });
}

void CVCPelcoD::menuDown()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenuDown();
        ui->statusbar->showMessage("Menu Down");
        return true;
    });
}

void CVCPelcoD::menuLeft()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenuLeft();
        ui->statusbar->showMessage("Menu Left");
        return true;
    });
}

void CVCPelcoD::menuRight()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenuRight();
        ui->statusbar->showMessage("Menu Right");
        return true;
    });
}

void CVCPelcoD::menuEnter()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenuEnter();
        ui->statusbar->showMessage("Menu Enter");
        return true;
    });
}

void CVCPelcoD::menuBack()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaMenuBack();
        ui->statusbar->showMessage("Menu Back");
        return true;
    });
}

void CVCPelcoD::camOn()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaOn();
        ui->statusbar->showMessage("Cam On");
        return true;
    });
}

void CVCPelcoD::camOff()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaOff();
        ui->statusbar->showMessage("Cam Off");
        return true;
    });
}

void CVCPelcoD::autoFramingOn()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaAutoFramingStart();
        ui->statusbar->showMessage("Auto Framing On");
        return true;
    });
}

void CVCPelcoD::autoFramingOff()
{
    if (settings.CAMERAS.empty()) return;
    addCommandToQueue ([this] () -> bool {
        cameraConnect[camIndex]->viscaAutoFramingStop();
        ui->statusbar->showMessage("Auto Framing Off");
        return true;
    });
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
        obsConnect->switchToScene(curScene+1, settings.CAMERAS.empty()? 0 : settings.CAMERAS[camIndex].CAMERA_ID);
    }
}

void CVCPelcoD::switchOBSStudioMode(bool en)
{
    if (en) {
        obsConnect->switchStudioMode();
    }
}

void CVCPelcoD::onMatrixConnectionFailed()
{
    static bool errorDialogShown = false;
    if (!errorDialogShown) {
        errorDialogShown = true;
        QMessageBox::warning(this, "Matrix Connection Error", "Please restart the matrix, or you are not able to control it.");
    }
}

