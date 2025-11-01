// vim:ts=4:sw=4:et:cin

#pragma once

#include <vector>
#include <unordered_map>
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

struct MatrixMacro {
    QString TITLE;
    QString NAME;
    std::unordered_map<unsigned, std::vector<unsigned>> MAPPING; //INPUT -> OUTPUTS
};

struct CustomRule {
    // Conditions
    bool has_mapping_condition = false;
    unsigned INPUT_IDX;
    unsigned OUTPUT_IDX;

    // Actions
    std::unordered_map<uint_fast8_t, uint_fast8_t> OBS_SCENE_OVERRIDE_LIST;
    bool OBS_SCENE_OVERRIDE_CLEAR = false;
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
    std::unordered_map<unsigned, std::vector<unsigned>> DEFAULT_MAPPING; //INPUT -> OUTPUTS
    std::vector<MatrixMacro> MACROS;
    std::unordered_map<unsigned, unsigned> INPUT_PORT_TO_IDX;
    std::unordered_map<unsigned, unsigned> OUTPUT_PORT_TO_IDX;
    std::vector<CustomRule> CUSTOM_RULES;
};

struct CVCSettings {
    OBSSettings OBS;
    std::vector<CameraSettings> CAMERAS;
    StreamDeckSettings STREAM_DECK;
    MatrixSettings MATRIX;

    void parseJSON(const QString& filename); //throw exception when error
};

