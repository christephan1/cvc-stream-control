#pragma once

#include "visca.h"
#include <stdio.h>
#include <string>
#include <QUdpSocket>
#include <QHostAddress>
#include <QString>
#include "cvcsetting.h"

CameraConnect::CameraConnect(const CameraSettings& cameraSettings, QObject* parent)
    : QUdpSocket(parent), settings(cameraSettings), seqNo(0)
{
}

void CameraConnect::voiSend (const std::string& visca_cmd)
{
    // Read Previous data
    while (QUdpSocket::hasPendingDatagrams()) {
        auto n = QUdpSocket::pendingDatagramSize();
        char c[n];
        if (QUdpSocket::readDatagram (c, n) >= 0) {
        } else {
            perror ("UDP Read Error: ");
            return;
        }
    }

    // Send this data
    const std::string* to_send = nullptr;
    std::string visca_with_header(8+visca_cmd.size(), '\0');
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        seqNo++;
        visca_with_header[0] = 0x01;
        visca_with_header[1] = 0x00;
        visca_with_header[2] = (char) (visca_cmd.size() >> 8) & 0xff;
        visca_with_header[3] = (char) (visca_cmd.size()     ) & 0xff;
        visca_with_header[4] = (char) (seqNo >> 24) & 0xff;
        visca_with_header[5] = (char) (seqNo >> 16) & 0xff;
        visca_with_header[6] = (char) (seqNo >>  8) & 0xff;
        visca_with_header[7] = (char) (seqNo      ) & 0xff;
        for (size_t i = 0; i < visca_cmd.size(); i++)
            visca_with_header[8+i] = visca_cmd[i];
        to_send = &visca_with_header;
    } else {
        to_send = &visca_cmd;
    }
    if (QUdpSocket::writeDatagram(to_send->c_str(), to_send->size(), QHostAddress(settings.CAMERA_HOST), settings.CAMERA_PORT) < 0) {
        perror ("UDP Send Error: ");
        return;
    }
}

void CameraConnect::viscaMove (int x, int y)
{
    char cX = (x>0? 0x02 : x<0? 0x01 : 0x03);
    char cY = (y>0? 0x02 : y<0? 0x01 : 0x03);
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    if (x < settings.MIN_PAN_SPEED) x = settings.MIN_PAN_SPEED;
    if (x > settings.MAX_PAN_SPEED) x = settings.MAX_PAN_SPEED;
    if (y < settings.MIN_TILT_SPEED) y = settings.MIN_TILT_SPEED;
    if (y > settings.MAX_TILT_SPEED) y = settings.MAX_TILT_SPEED;
    std::string cmd (9, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x01;
    cmd [4] = (char) x;
    cmd [5] = (char) y;
    cmd [6] = cX;
    cmd [7] = cY;
    cmd [8] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaLeft (unsigned value)
{
    if (value < settings.MIN_PAN_SPEED) value = settings.MIN_PAN_SPEED;
    if (value > settings.MAX_PAN_SPEED) value = settings.MAX_PAN_SPEED;
    std::string cmd (9, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x01;
    cmd [4] = (char) value;
    cmd [5] = 0x01;
    cmd [6] = 0x01;
    cmd [7] = 0x03;
    cmd [8] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaRight (unsigned value)
{
    if (value < settings.MIN_PAN_SPEED) value = settings.MIN_PAN_SPEED;
    if (value > settings.MAX_PAN_SPEED) value = settings.MAX_PAN_SPEED;
    std::string cmd (9, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x01;
    cmd [4] = (char) value;
    cmd [5] = 0x01;
    cmd [6] = 0x02;
    cmd [7] = 0x03;
    cmd [8] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaUp (unsigned value)
{
    if (value < settings.MIN_TILT_SPEED) value = settings.MIN_TILT_SPEED;
    if (value > settings.MAX_TILT_SPEED) value = settings.MAX_TILT_SPEED;
    std::string cmd (9, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x01;
    cmd [4] = 0x01;
    cmd [5] = (char) value;
    cmd [6] = 0x03;
    cmd [7] = 0x01;
    cmd [8] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaDown (unsigned value)
{
    if (value < settings.MIN_TILT_SPEED) value = settings.MIN_TILT_SPEED;
    if (value > settings.MAX_TILT_SPEED) value = settings.MAX_TILT_SPEED;
    std::string cmd (9, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x01;
    cmd [4] = 0x01;
    cmd [5] = (char) value;
    cmd [6] = 0x03;
    cmd [7] = 0x02;
    cmd [8] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaStop ()
{
    std::string cmd (9, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x01;
    cmd [4] = 0x01;
    cmd [5] = 0x01;
    cmd [6] = 0x03;
    cmd [7] = 0x03;
    cmd [8] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaIn (unsigned value)
{
    if (value < settings.MIN_ZOOM_SPEED) value = settings.MIN_ZOOM_SPEED;
    if (value > settings.MAX_ZOOM_SPEED) value = settings.MAX_ZOOM_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x07;
    cmd [4] = 0x20 | value;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaOut (unsigned value)
{
    if (value < settings.MIN_ZOOM_SPEED) value = settings.MIN_ZOOM_SPEED;
    if (value > settings.MAX_ZOOM_SPEED) value = settings.MAX_ZOOM_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x07;
    cmd [4] = 0x30 | value;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaZoomStop ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x07;
    cmd [4] = 0x00;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaFar (unsigned value)
{
    if (value < settings.MIN_FOCUS_SPEED) value = settings.MIN_FOCUS_SPEED;
    if (value > settings.MAX_FOCUS_SPEED) value = settings.MAX_FOCUS_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x08;
    cmd [4] = 0x20 | value;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaNear (unsigned value)
{
    if (value < settings.MIN_FOCUS_SPEED) value = settings.MIN_FOCUS_SPEED;
    if (value > settings.MAX_FOCUS_SPEED) value = settings.MAX_FOCUS_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x08;
    cmd [4] = 0x30 | value;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaFocusStop ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x08;
    cmd [4] = 0x00;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaAutoFocus ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x38;
    cmd [4] = 0x02;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaManualFocus ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x38;
    cmd [4] = 0x03;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaFocusAM ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x38;
    cmd [4] = 0x10;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaSet (unsigned value)
{
    if (value < settings.MIN_PRESET_NO) { perror ("Wrong Preset No."); return; }
    if (value > settings.MAX_PRESET_NO) { perror ("Wrong Preset No."); return; }
    std::string cmd (7, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x3f;
    cmd [4] = 0x01;
    cmd [5] = value;
    cmd [6] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaGo (unsigned value)
{
    if (value < settings.MIN_PRESET_NO) { perror ("Wrong Preset No."); return; }
    if (value > settings.MAX_PRESET_NO) { perror ("Wrong Preset No."); return; }
    std::string cmd (7, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x3f;
    cmd [4] = 0x02;
    cmd [5] = value;
    cmd [6] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaOn ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x00;
    cmd [4] = 0x02;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaOff ()
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x00;
    cmd [4] = 0x03;
    cmd [5] = 0xff;
    voiSend (cmd);
}

void CameraConnect::viscaMenu ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (6, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x06;
        cmd [3] = 0x06;
        cmd [4] = 0x10;
        cmd [5] = 0xff;
        voiSend (cmd);
    } else {
        std::string cmd (7, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x04;
        cmd [3] = 0x3f;
        cmd [4] = 0x02;
        cmd [5] = 0x5f;
        cmd [6] = 0xff;
        voiSend (cmd);
    }
}

void CameraConnect::viscaMenuUp ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (9, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x7e;
        cmd [3] = 0x04;
        cmd [4] = 0x40;
        cmd [5] = 0x03;
        cmd [6] = 0x01;
        cmd [7] = 0x01;
        cmd [8] = 0xff;
        voiSend (cmd);
        cmd [7] = 0x00;
        voiSend (cmd);
    } else {
        viscaUp(0x0E);
    }
}

void CameraConnect::viscaMenuDown ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (9, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x7e;
        cmd [3] = 0x04;
        cmd [4] = 0x40;
        cmd [5] = 0x03;
        cmd [6] = 0x02;
        cmd [7] = 0x01;
        cmd [8] = 0xff;
        voiSend (cmd);
        cmd [7] = 0x00;
        voiSend (cmd);
    } else {
        viscaDown(0x0E);
    }
}

void CameraConnect::viscaMenuLeft ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (9, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x7e;
        cmd [3] = 0x04;
        cmd [4] = 0x40;
        cmd [5] = 0x01;
        cmd [6] = 0x03;
        cmd [7] = 0x01;
        cmd [8] = 0xff;
        voiSend (cmd);
        cmd [7] = 0x00;
        voiSend (cmd);
    } else {
        viscaLeft(0x0E);
    }
}

void CameraConnect::viscaMenuRight ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (9, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x7e;
        cmd [3] = 0x04;
        cmd [4] = 0x40;
        cmd [5] = 0x02;
        cmd [6] = 0x03;
        cmd [7] = 0x01;
        cmd [8] = 0xff;
        voiSend (cmd);
        cmd [7] = 0x00;
        voiSend (cmd);
    } else {
        viscaRight(0x0E);
    }
}

void CameraConnect::viscaMenuEnter ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (9, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x7e;
        cmd [3] = 0x04;
        cmd [4] = 0x40;
        cmd [5] = 0x07;
        cmd [6] = 0x00;
        cmd [7] = 0x01;
        cmd [8] = 0xff;
        voiSend (cmd);
        cmd [7] = 0x00;
        voiSend (cmd);
    } else {
        std::string cmd (6, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x06;
        cmd [3] = 0x06;
        cmd [4] = 0x05;
        cmd [5] = 0xff;
        voiSend (cmd);
    }
}

void CameraConnect::viscaMenuBack ()
{
    if (settings.CAMERA_PROTOCAL == CameraSettings::Protocal::VISCA_STRICT) {
        std::string cmd (9, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x7e;
        cmd [3] = 0x04;
        cmd [4] = 0x40;
        cmd [5] = 0x07;
        cmd [6] = 0x01;
        cmd [7] = 0x01;
        cmd [8] = 0xff;
        voiSend (cmd);
        cmd [7] = 0x00;
        voiSend (cmd);
    } else {
        std::string cmd (6, '\0');
        cmd [0] = 0x81;
        cmd [1] = 0x01;
        cmd [2] = 0x06;
        cmd [3] = 0x06;
        cmd [4] = 0x04;
        cmd [5] = 0xff;
        voiSend (cmd);
    }
}

