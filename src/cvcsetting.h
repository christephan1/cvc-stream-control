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

struct StreamDeckSettings {
    QString  STREAM_DECK_HOST = "127.0.0.1";
    uint16_t STREAM_DECK_PORT = 9387;

};

struct MatrixPort {
    QString  NAME;
    unsigned PORT;
};

struct MatrixSettings {
    bool     enabled = false;  // Indicates whether Matrix settings are present in JSON
    QString  MATRIX_HOST;
    uint16_t MATRIX_PORT;
    QString  MATRIX_USERNAME;
    QString  MATRIX_PASSWORD;
    enum class Protocol {
        MT_VIKI
    };
    Protocol MATRIX_PROTOCOL;
    std::vector<MatrixPort> INPUTS;
    std::vector<MatrixPort> OUTPUTS;
};

struct CVCSettings {
    OBSSettings OBS;
    std::vector<CameraSettings> CAMERAS;
    StreamDeckSettings STREAM_DECK;
    MatrixSettings MATRIX;

    void parseJSON(const QString& filename); //throw exception when error
};

