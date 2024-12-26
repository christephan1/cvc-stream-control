// vim:ts=4:sw=4:et:cin

#pragma once

#include <vector>
#include <QString>

struct OBSSettings {
    QString OBS_HOST;
    int     OBS_PORT;
};

struct CameraSettings {
    enum class Protocal {
        VISCA_STRICT,
        VISCA_LOOSE
    };
    int      CAMERA_ID;
    QString  CAMERA_HOST;
    uint16_t CAMERA_PORT;
    Protocal CAMERA_PROTOCAL;
    unsigned MIN_ZOOM_SPEED;
    unsigned MAX_ZOOM_SPEED;
    unsigned MIN_PAN_SPEED;
    unsigned MAX_PAN_SPEED;
    unsigned MIN_TILT_SPEED;
    unsigned MAX_TILT_SPEED;
    unsigned MIN_FOCUS_SPEED;
    unsigned MAX_FOCUS_SPEED;
    unsigned MIN_PRESET_NO;
    unsigned MAX_PRESET_NO;
};

struct StreamDeckKeySettings {
    enum class KeyFunction {
        HOME
    };

};

struct StreamDeckSettings {
    QString  STREAM_DECK_HOST = "127.0.0.1";
    uint16_t STREAM_DECK_PORT = 9387;

};

struct CVCSettings {
    OBSSettings OBS;
    std::vector<CameraSettings> CAMERAS;
    StreamDeckSettings STREAM_DECK;

    void parseJSON(const QString& filename); //throw exception when error
};

