// vim:ts=4:sw=4:et:cin

#include "cvcsetting.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDebug>
#include <stdexcept>

//This function is generated by AI
void CVCSettings::parseJSON(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error("Couldn't open JSON file.");
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &jsonError);

    if (doc.isNull()) {
        throw std::runtime_error(("Invalid JSON document: " + jsonError.errorString()).toStdString());
    }

    QJsonObject root = doc.object();

    // Check for required keys in the root object
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

    // Parse camera settings
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

    // Parse StreamDeck settings if present
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

    // Parse Matrix settings if present
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
    }
}


