#pragma once

#include <stdio.h>
#include <map>
#include <cstring>
#include <string>
#include <QUdpSocket>
#include <QHostAddress>
#include <QString>
#include "cvcsetting.h"

namespace {

QUdpSocket *udp_sock;
void setUdpSock (QUdpSocket* sock)
{
    udp_sock = sock;
}

void voiSend ( const int camera_id, const std::string& visca_cmd  )
{
    // Read Previous data
    while (udp_sock->hasPendingDatagrams()) {
        auto n = udp_sock->pendingDatagramSize();
        char c[n];
        if (udp_sock->readDatagram (c, n) >= 0) {
        } else {
            perror ("UDP Read Error: ");
            return;
        }
    }

    // Send this data
    auto iter = CAMERA_IP.find(camera_id);
    if (iter == CAMERA_IP.end()) {
        puts ("Wrong camera ID.");
        return;
    }
    const char* camera_ip = iter->second;

    if (udp_sock->writeDatagram(visca_cmd.c_str(), visca_cmd.size(), QHostAddress(QString(camera_ip)), VISCA_PORT) < 0) {
        perror ("UDP Send Error: ");
        return;
    }
}

void viscaMove (const int camera_id, int x, int y)
{
    char cX = (x>0? 0x02 : x<0? 0x01 : 0x03);
    char cY = (y>0? 0x02 : y<0? 0x01 : 0x03);
    if (x < 0) x = -x;
    if (y < 0) y = -y;
    if (x < MIN_PAN_SPEED) x = MIN_PAN_SPEED;
    if (x > MAX_PAN_SPEED) x = MAX_PAN_SPEED;
    if (y < MIN_TILT_SPEED) y = MIN_TILT_SPEED;
    if (y > MAX_TILT_SPEED) y = MAX_TILT_SPEED;
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
    voiSend (camera_id, cmd);
}

void viscaLeft (const int camera_id, unsigned value)
{
    if (value < MIN_PAN_SPEED) value = MIN_PAN_SPEED;
    if (value > MAX_PAN_SPEED) value = MAX_PAN_SPEED;
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
    voiSend (camera_id, cmd);
}

void viscaRight (const int camera_id, unsigned value)
{
    if (value < MIN_PAN_SPEED) value = MIN_PAN_SPEED;
    if (value > MAX_PAN_SPEED) value = MAX_PAN_SPEED;
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
    voiSend (camera_id, cmd);
}

void viscaUp (const int camera_id, unsigned value)
{
    if (value < MIN_TILT_SPEED) value = MIN_TILT_SPEED;
    if (value > MAX_TILT_SPEED) value = MAX_TILT_SPEED;
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
    voiSend (camera_id, cmd);
}

void viscaDown (const int camera_id, unsigned value)
{
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
    voiSend (camera_id, cmd);
}

void viscaStop (const int camera_id)
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
    voiSend (camera_id, cmd);
}

void viscaIn (const int camera_id, unsigned value)
{
    if (value < MIN_ZOOM_SPEED) value = MIN_ZOOM_SPEED;
    if (value > MAX_ZOOM_SPEED) value = MAX_ZOOM_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x07;
    cmd [4] = 0x20 | value;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaOut (const int camera_id, unsigned value)
{
    if (value < MIN_ZOOM_SPEED) value = MIN_ZOOM_SPEED;
    if (value > MAX_ZOOM_SPEED) value = MAX_ZOOM_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x07;
    cmd [4] = 0x30 | value;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaZoomStop (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x07;
    cmd [4] = 0x00;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaFar (const int camera_id, unsigned value)
{
    if (value < MIN_FOCUS_SPEED) value = MIN_FOCUS_SPEED;
    if (value > MAX_FOCUS_SPEED) value = MAX_FOCUS_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x08;
    cmd [4] = 0x20 | value;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaNear (const int camera_id, unsigned value)
{
    if (value < MIN_FOCUS_SPEED) value = MIN_FOCUS_SPEED;
    if (value > MAX_FOCUS_SPEED) value = MAX_FOCUS_SPEED;
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x08;
    cmd [4] = 0x30 | value;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaFocusStop (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x08;
    cmd [4] = 0x00;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaAutoFocus (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x38;
    cmd [4] = 0x02;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaManualFocus (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x38;
    cmd [4] = 0x03;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaFocusAM (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x38;
    cmd [4] = 0x10;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaSet (const int camera_id, unsigned value)
{
    if (value < MIN_PRESET_NO) { perror ("Wrong Preset No."); return; }
    if (value > MAX_PRESET_NO) { perror ("Wrong Preset No."); return; }
    std::string cmd (7, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x3f;
    cmd [4] = 0x01;
    cmd [5] = value;
    cmd [6] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaGo (const int camera_id, unsigned value)
{
    if (value < MIN_PRESET_NO) { perror ("Wrong Preset No."); return; }
    if (value > MAX_PRESET_NO) { perror ("Wrong Preset No."); return; }
    std::string cmd (7, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x3f;
    cmd [4] = 0x02;
    cmd [5] = value;
    cmd [6] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaOn (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x00;
    cmd [4] = 0x02;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaOff (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x00;
    cmd [4] = 0x03;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaMenu (const int camera_id)
{
    std::string cmd (7, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x04;
    cmd [3] = 0x3f;
    cmd [4] = 0x02;
    cmd [5] = 0x5f;
    cmd [6] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaEnter (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x06;
    cmd [4] = 0x05;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

void viscaReturn (const int camera_id)
{
    std::string cmd (6, '\0');
    cmd [0] = 0x81;
    cmd [1] = 0x01;
    cmd [2] = 0x06;
    cmd [3] = 0x06;
    cmd [4] = 0x04;
    cmd [5] = 0xff;
    voiSend (camera_id, cmd);
}

}
