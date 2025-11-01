// vim:ts=4:sw=4:et:cin

#include "cvcsetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>
#include <stdexcept>
#include <unordered_set>

/**
 * @brief Parses the application settings from a JSON file.
 *
 * This function reads the specified JSON file, parses its contents, and populates the
 * fields of the CVCSettings struct. It performs validation to ensure that required
 * keys are present and that the data is well-formed. If any error occurs during
 * parsing (e.g., file not found, invalid JSON, missing keys), it throws a
 * std::runtime_error with a descriptive message.
 *
 * @param filename The path to the JSON configuration file.
 * @throws std::runtime_error if parsing fails for any reason.
 */
void CVCSettings::parseJSON(const QString& filename) {
    // Open and read the JSON file
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Couldn't open JSON file.");
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // Parse the JSON document
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &jsonError);

    if (doc.isNull()) {
        throw std::runtime_error(("Invalid JSON document: " + jsonError.errorString()).toStdString());
    }

    QJsonObject root = doc.object();

    // Check for required root keys
    if (!root.contains("OBS") || !root.contains("CAMERAS")) {
        throw std::runtime_error("Missing 'OBS' or 'CAMERAS' key in JSON.");
    }

    // Parse OBS settings
    QJsonObject obsObject = root["OBS"].toObject();
    if (!obsObject.contains("OBS_HOST") || !obsObject.contains("OBS_PORT")) {
        throw std::runtime_error("Missing 'OBS_HOST' or 'OBS_PORT' key in JSON.");
    }
    OBS.OBS_HOST = obsObject["OBS_HOST"].toString();
    OBS.OBS_PORT = obsObject["OBS_PORT"].toInt();

    // Parse camera settings from the array
    QJsonArray camerasArray = root["CAMERAS"].toArray();
    for (const QJsonValue& value : camerasArray) {
        QJsonObject cameraObject = value.toObject();

        // Check for required keys in each camera object
        QStringList requiredKeys = { "CAMERA_ID", "CAMERA_HOST", "CAMERA_PORT", 
                                     "CAMERA_PROTOCAL", "MIN_ZOOM_SPEED", "MAX_ZOOM_SPEED", 
                                     "MIN_PAN_SPEED", "MAX_PAN_SPEED", "MIN_TILT_SPEED", 
                                     "MAX_TILT_SPEED", "MIN_FOCUS_SPEED", "MAX_FOCUS_SPEED", 
                                     "MIN_PRESET_NO", "MAX_PRESET_NO" };

        for (const QString& key : requiredKeys) {
            if (!cameraObject.contains(key)) {
                throw std::runtime_error(QString("Missing '%1' key in camera settings.").arg(key).toStdString());
            }
        }

        CameraSettings camera;
        camera.CAMERA_ID = cameraObject["CAMERA_ID"].toInt();
        camera.CAMERA_HOST = cameraObject["CAMERA_HOST"].toString();
        camera.CAMERA_PORT = cameraObject["CAMERA_PORT"].toInt();

        QString protocolString = cameraObject["CAMERA_PROTOCAL"].toString();
        if (protocolString == "VISCA_STRICT") {
            camera.CAMERA_PROTOCAL = CameraSettings::Protocal::VISCA_STRICT;
        } else if (protocolString == "VISCA_LOOSE") {
            camera.CAMERA_PROTOCAL = CameraSettings::Protocal::VISCA_LOOSE;
        } else {
            throw std::runtime_error(QString("Unknown camera protocol: %1").arg(protocolString).toStdString());
        }

        camera.MIN_ZOOM_SPEED = cameraObject["MIN_ZOOM_SPEED"].toInt();
        camera.MAX_ZOOM_SPEED = cameraObject["MAX_ZOOM_SPEED"].toInt();
        camera.MIN_PAN_SPEED = cameraObject["MIN_PAN_SPEED"].toInt();
        camera.MAX_PAN_SPEED = cameraObject["MAX_PAN_SPEED"].toInt();
        camera.MIN_TILT_SPEED = cameraObject["MIN_TILT_SPEED"].toInt();
        camera.MAX_TILT_SPEED = cameraObject["MAX_TILT_SPEED"].toInt();
        camera.MIN_FOCUS_SPEED = cameraObject["MIN_FOCUS_SPEED"].toInt();
        camera.MAX_FOCUS_SPEED = cameraObject["MAX_FOCUS_SPEED"].toInt();
        camera.MIN_PRESET_NO = cameraObject["MIN_PRESET_NO"].toInt();
        camera.MAX_PRESET_NO = cameraObject["MAX_PRESET_NO"].toInt();

        CAMERAS.push_back(camera);
    }

    // Parse StreamDeck settings if the section is present
    if (root.contains("STREAM_DECK")) {
        QJsonObject streamDeckObject = root["STREAM_DECK"].toObject();
        
        // Check for required keys in stream deck object
        QStringList requiredKeys = { "STREAM_DECK_HOST", "STREAM_DECK_PORT" };

        for (const QString& key : requiredKeys) {
            if (!streamDeckObject.contains(key)) {
                throw std::runtime_error(QString("Missing '%1' key in stream deck settings.").arg(key).toStdString());
            }
        }

        STREAM_DECK.STREAM_DECK_HOST = streamDeckObject["STREAM_DECK_HOST"].toString();
        STREAM_DECK.STREAM_DECK_PORT = streamDeckObject["STREAM_DECK_PORT"].toInt();
    }

    // Parse Matrix settings if the section is present
    if (root.contains("MATRIX")) {
        MATRIX.enabled = true;  // Set enabled flag when Matrix section exists
        QJsonObject matrixObject = root["MATRIX"].toObject();
        
        // Check for required keys in matrix object
        QStringList requiredKeys = { "MATRIX_HOST", "MATRIX_PORT", "MATRIX_USERNAME", 
                                   "MATRIX_PASSWORD", "MATRIX_PROTOCOL" };

        for (const QString& key : requiredKeys) {
            if (!matrixObject.contains(key)) {
                throw std::runtime_error(QString("Missing '%1' key in matrix settings.").arg(key).toStdString());
            }
        }

        MATRIX.MATRIX_HOST = matrixObject["MATRIX_HOST"].toString();
        MATRIX.MATRIX_PORT = matrixObject["MATRIX_PORT"].toInt();
        MATRIX.MATRIX_USERNAME = matrixObject["MATRIX_USERNAME"].toString();
        MATRIX.MATRIX_PASSWORD = matrixObject["MATRIX_PASSWORD"].toString();

        QString protocolString = matrixObject["MATRIX_PROTOCOL"].toString();
        if (protocolString == "MT-VIKI") {
            MATRIX.MATRIX_PROTOCOL = MatrixSettings::Protocol::MT_VIKI;
        } else {
            throw std::runtime_error(QString("Unknown matrix protocol: %1").arg(protocolString).toStdString());
        }

        // Helper lambda to parse an array of ports (either INPUTS or OUTPUTS)
        auto parsePorts = [](const QJsonObject& parent, const QString& key) {
            std::vector<MatrixPort> ports;
            if (parent.contains(key)) {
                QJsonArray portArray = parent[key].toArray();
                for (const QJsonValue& value : portArray) {
                    QJsonObject portObject = value.toObject();
                    if (portObject.contains("NAME") && portObject.contains("PORT")) {
                        MatrixPort port;
                        port.NAME = portObject["NAME"].toString();
                        port.PORT = portObject["PORT"].toVariant().toUInt();
                        ports.push_back(port);
                    } else {
                        throw std::runtime_error(QString("Missing 'NAME' or 'PORT' key in matrix %1 settings.").arg(key).toStdString());
                    }
                }
            }
            return ports;
        };

        MATRIX.INPUTS = parsePorts(matrixObject, "INPUTS");
        MATRIX.OUTPUTS = parsePorts(matrixObject, "OUTPUTS");

        // Create fast lookup maps for input/output names and ports to their indices.
        // This also serves to validate that there are no duplicate names or ports.
        std::unordered_map<QString, size_t> inputNameToIndex;
        MATRIX.INPUT_PORT_TO_IDX.clear();
        for (size_t i = 0; i < MATRIX.INPUTS.size(); ++i) {
            const auto& input = MATRIX.INPUTS[i];
            if (!inputNameToIndex.emplace(input.NAME, i).second) {
                throw std::runtime_error("Duplicate MATRIX INPUT NAME found: " + input.NAME.toStdString());
            }
            if (!MATRIX.INPUT_PORT_TO_IDX.emplace(input.PORT, i).second) {
                throw std::runtime_error("Duplicate MATRIX INPUT PORT found: " + std::to_string(input.PORT));
            }
        }

        std::unordered_map<QString, size_t> outputNameToIndex;
        MATRIX.OUTPUT_PORT_TO_IDX.clear();
        for (size_t i = 0; i < MATRIX.OUTPUTS.size(); ++i) {
            const auto& output = MATRIX.OUTPUTS[i];
            if (!outputNameToIndex.emplace(output.NAME, i).second) {
                throw std::runtime_error("Duplicate MATRIX OUTPUT NAME found: " + output.NAME.toStdString());
            }
            if (!MATRIX.OUTPUT_PORT_TO_IDX.emplace(output.PORT, i).second) {
                throw std::runtime_error("Duplicate MATRIX OUTPUT PORT found: " + std::to_string(output.PORT));
            }
        }

        // Helper lambda to parse a mapping array (e.g., DEFAULT_MAPPING or a macro's MAPPING).
        // It resolves input/output names or ports to their corresponding indices.
        auto parseMapping = [this, &inputNameToIndex, &outputNameToIndex](const QJsonArray& mappingArray, const QString& errorPrefix) {
            std::unordered_map<unsigned, std::vector<unsigned>> mapping;
            std::unordered_set<unsigned> allOutputIndices;

            for (const QJsonValue& mappingValue : mappingArray) {
                QJsonObject mappingObject = mappingValue.toObject();
                if (!mappingObject.contains("INPUT") || !mappingObject.contains("OUTPUT")) {
                    throw std::runtime_error(errorPrefix.toStdString() + " item must contain 'INPUT' and 'OUTPUT' keys.");
                }

                // Resolve the input, which can be specified by name (string) or port (integer)
                unsigned inputIndex;
                QJsonValue inputValue = mappingObject["INPUT"];
                if (inputValue.isString()) {
                    auto it = inputNameToIndex.find(inputValue.toString());
                    if (it == inputNameToIndex.end()) {
                        throw std::runtime_error(errorPrefix.toStdString() + " INPUT name not found: " + inputValue.toString().toStdString());
                    }
                    inputIndex = it->second;
                } else if (inputValue.isDouble()) {
                    auto it = MATRIX.INPUT_PORT_TO_IDX.find(inputValue.toInt());
                    if (it == MATRIX.INPUT_PORT_TO_IDX.end()) {
                        throw std::runtime_error(errorPrefix.toStdString() + " INPUT port not found: " + std::to_string(inputValue.toInt()));
                    }
                    inputIndex = it->second;
                } else {
                    throw std::runtime_error(errorPrefix.toStdString() + " INPUT must be a string (name) or integer (port).");
                }

                // Resolve the output(s), which is an array of names or ports
                std::vector<unsigned> outputIndices;
                QJsonValue outputValue = mappingObject["OUTPUT"];
                if (!outputValue.isArray()) {
                    throw std::runtime_error(errorPrefix.toStdString() + " OUTPUT must be an array.");
                }
                QJsonArray outputArray = outputValue.toArray();
                for (const QJsonValue& outputItemValue : outputArray) {
                    unsigned outputIndex;
                    if (outputItemValue.isString()) {
                        auto it = outputNameToIndex.find(outputItemValue.toString());
                        if (it == outputNameToIndex.end()) {
                            throw std::runtime_error(errorPrefix.toStdString() + " OUTPUT name not found: " + outputItemValue.toString().toStdString());
                        }
                        outputIndex = it->second;
                    } else if (outputItemValue.isDouble()) {
                        auto it = MATRIX.OUTPUT_PORT_TO_IDX.find(outputItemValue.toInt());
                        if (it == MATRIX.OUTPUT_PORT_TO_IDX.end()) {
                            throw std::runtime_error(errorPrefix.toStdString() + " OUTPUT port not found: " + std::to_string(outputItemValue.toInt()));
                        }
                        outputIndex = it->second;
                    } else {
                        throw std::runtime_error(errorPrefix.toStdString() + " OUTPUT item must be a string (name) or integer (port).");
                    }

                    // Ensure that each output port is only mapped to one input port within this mapping configuration
                    if (!allOutputIndices.insert(outputIndex).second) {
                        const auto& port = MATRIX.OUTPUTS.at(outputIndex).PORT;
                        throw std::runtime_error("Duplicate OUTPUT in " + errorPrefix.toStdString() + " for port: " + std::to_string(port));
                    }
                    outputIndices.push_back(outputIndex);
                }
                // Ensure that each input is only listed once in this mapping
                if (!mapping.emplace(inputIndex, std::move(outputIndices)).second) {
                    const auto& port = MATRIX.INPUTS.at(inputIndex).PORT;
                    throw std::runtime_error("Duplicate INPUT in " + errorPrefix.toStdString() + " for port: " + std::to_string(port));
                }
            }
            return mapping;
        };

        // Parse the default input-to-output mapping configuration
        if (matrixObject.contains("DEFAULT_MAPPING")) {
            QJsonArray defaultMappingArray = matrixObject["DEFAULT_MAPPING"].toArray();
            MATRIX.DEFAULT_MAPPING = parseMapping(defaultMappingArray, "DEFAULT_MAPPING");
        }

        // Parse the array of macros
        if (matrixObject.contains("MACROS")) {
            QJsonArray macrosArray = matrixObject["MACROS"].toArray();
            for (const QJsonValue& macroValue : macrosArray) {
                QJsonObject macroObject = macroValue.toObject();
                // Allow for empty objects in the array as placeholders
                if (macroObject.isEmpty()) {
                    MATRIX.MACROS.emplace_back();
                    continue;
                }

                // NAME is required, but TITLE is optional
                MatrixMacro macro;
                if (!macroObject.contains("NAME")) {
                    throw std::runtime_error("MACROS item must contain 'NAME' key.");
                }
                macro.NAME = macroObject["NAME"].toString();

                if (macroObject.contains("TITLE")) {
                    macro.TITLE = macroObject["TITLE"].toString();
                }

                if (macroObject.contains("MAPPING")) {
                    QJsonArray mappingArray = macroObject["MAPPING"].toArray();
                    macro.MAPPING = parseMapping(mappingArray, "MACROS." + macro.NAME + ".MAPPING");
                }
                MATRIX.MACROS.push_back(macro);
            }
        }

        // Parse CUSTOM_RULES
        if (matrixObject.contains("CUSTOM_RULES")) {
            QJsonArray rulesArray = matrixObject["CUSTOM_RULES"].toArray();
            for (const QJsonValue& ruleValue : rulesArray) {
                QJsonObject ruleObject = ruleValue.toObject();
                CustomRule rule;

                bool has_input = ruleObject.contains("INPUT");
                bool has_output = ruleObject.contains("OUTPUT");

                if (has_input != has_output) {
                    throw std::runtime_error("Custom rule must have both 'INPUT' and 'OUTPUT', or neither.");
                }

                if (has_input) { // && has_output is implied
                    rule.has_mapping_condition = true;
                    // Parse conditions
                    QJsonValue inputValue = ruleObject["INPUT"];
                    if (inputValue.isString()) {
                        auto it = inputNameToIndex.find(inputValue.toString());
                        if (it == inputNameToIndex.end()) {
                            throw std::runtime_error("CUSTOM_RULES INPUT name not found: " + inputValue.toString().toStdString());
                        }
                        rule.INPUT_IDX = it->second;
                    } else if (inputValue.isDouble()) {
                        auto it = MATRIX.INPUT_PORT_TO_IDX.find(inputValue.toInt());
                        if (it == MATRIX.INPUT_PORT_TO_IDX.end()) {
                            throw std::runtime_error("CUSTOM_RULES INPUT port not found: " + std::to_string(inputValue.toInt()));
                        }
                        rule.INPUT_IDX = it->second;
                    } else {
                        throw std::runtime_error("CUSTOM_RULES INPUT must be a string (name) or integer (port).");
                    }
                    QJsonValue outputValue = ruleObject["OUTPUT"];
                    if (outputValue.isString()) {
                        auto it = outputNameToIndex.find(outputValue.toString());
                        if (it == outputNameToIndex.end()) {
                            throw std::runtime_error("CUSTOM_RULES OUTPUT name not found: " + outputValue.toString().toStdString());
                        }
                        rule.OUTPUT_IDX = it->second;
                    } else if (outputValue.isDouble()) {
                        auto it = MATRIX.OUTPUT_PORT_TO_IDX.find(outputValue.toInt());
                        if (it == MATRIX.OUTPUT_PORT_TO_IDX.end()) {
                            throw std::runtime_error("CUSTOM_RULES OUTPUT port not found: " + std::to_string(outputValue.toInt()));
                        }
                        rule.OUTPUT_IDX = it->second;
                    } else {
                        throw std::runtime_error("CUSTOM_RULES OUTPUT must be a string (name) or integer (port).");
                    }
                }

                // Parse actions
                if (ruleObject.contains("OBS_SCENE_OVERRIDE_LIST")) {
                    QJsonObject overrideObject = ruleObject["OBS_SCENE_OVERRIDE_LIST"].toObject();
                    for (auto it = overrideObject.begin(); it != overrideObject.end(); ++it) {
                        bool key_ok;
                        uint_fast8_t key = it.key().toUInt(&key_ok);
                        if (!key_ok) {
                            throw std::runtime_error("Invalid key in OBS_SCENE_OVERRIDE_LIST: '" + it.key().toStdString() + "'. Must be an integer string.");
                        }

                        bool value_ok;
                        uint_fast8_t value = it.value().toVariant().toUInt(&value_ok);

                        if (!value_ok) {
                            throw std::runtime_error("Invalid value for key '" + it.key().toStdString() + "' in OBS_SCENE_OVERRIDE_LIST. Must be an unsigned integer that fits in uint_fast8_t.");
                        }
                        rule.OBS_SCENE_OVERRIDE_LIST[key] = value;
                    }
                }
                if (ruleObject.contains("OBS_SCENE_OVERRIDE_CLEAR")) {
                    QJsonValue clearValue = ruleObject["OBS_SCENE_OVERRIDE_CLEAR"];
                    if (!clearValue.isBool()) {
                        throw std::runtime_error("Invalid value for OBS_SCENE_OVERRIDE_CLEAR, must be a boolean.");
                    }
                    rule.OBS_SCENE_OVERRIDE_CLEAR = clearValue.toBool();
                }
                MATRIX.CUSTOM_RULES.push_back(rule);
            }
        }
    }
}


