// vim:ts=4:sw=4:et:cin

#pragma once

namespace {

//----------- OBS SETTINGS --------------//
#define OBS_SERVER "127.0.0.1"
#define OBS_PORT 4455

//----------- CAMERA SETTINGS --------------//
#define MIN_CAM_ID 2
#define MAX_CAM_ID 4

#define LOCAL_IP "192.168.100.231"
const std::map<int,const char*> CAMERA_IP = {
    { 2, "192.168.100.2" },
    { 3, "192.168.100.3" },
    { 4, "192.168.100.4" }
};
#define VISCA_PORT 1259
#define MIN_ZOOM_SPEED 0x00
#define MAX_ZOOM_SPEED 0x07
#define MIN_PAN_SPEED  0x01
#define MAX_PAN_SPEED  0x18
#define MIN_TILT_SPEED 0x01
#define MAX_TILT_SPEED 0x14
#define MIN_FOCUS_SPEED 0x00
#define MAX_FOCUS_SPEED 0x07
#define MIN_PRESET_NO  0
#define MAX_PRESET_NO  89

}

