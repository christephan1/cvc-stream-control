// vim:ts=4:sw=4:et:cin

#include "matrixconnect.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include "cvcsetting.h"

MatrixConnect::MatrixConnect(const MatrixSettings& settings_)
    : QNetworkAccessManager(), settings(settings_)
{
}

void MatrixConnect::captionSourceCaption()
{
    switchChannels_old(9,  {4, 7});
    switchChannels_old(10, {1});
}

void MatrixConnect::captionSourceProjector()
{
    switchChannels_old(9,  {4});
    switchChannels_old(10, {1, 7});
}

void MatrixConnect::captionSourceLectern()
{
    switchChannels_old(8, {1, 4, 7});
}

void MatrixConnect::captionSource1F()
{
    switchChannels_old(5, {4, 7});
    switchChannels_old(10, {1});
}

void MatrixConnect::captionSourceB1()
{
}

void MatrixConnect::switchChannel(unsigned src, unsigned dst)
{
    switchChannels(src, {dst});
}

void MatrixConnect::resetMatrix()
{
    for (const auto& mapping : settings.DEFAULT_MAPPING) {
        switchChannels(mapping.first, mapping.second);
    }
}

void MatrixConnect::switchChannels(unsigned src, const std::vector<unsigned>& dst)
{
    if (!settings.enabled) {
        emit updateStatus("Video Matrix device settings is not presented.");
        return;
    }

    if (dst.empty()) {
        emit updateStatus("Error: Destination channels list is empty.");
        return;
    }

    if (src >= settings.INPUTS.size()) {
        emit updateStatus("Error: Source channel out of range.");
        return;
    }
    for (unsigned destChannel : dst) {
        if (destChannel >= settings.OUTPUTS.size()) {
            emit updateStatus("Error: Destination channel out of range.");
            return;
        }
    }

    if (settings.MATRIX_PROTOCOL == MatrixSettings::Protocol::MT_VIKI) {
        QUrl url;
        url.setScheme("http");  // or https depending on your setup
        url.setHost(settings.MATRIX_HOST);
        url.setPort(settings.MATRIX_PORT);
        url.setPath("/goform/SetMatrixDirectCmd");

        // Construct the command string: "SW SRC DST1 DST2 DST3..."
        QString commandStr = QString("SW %1").arg(settings.INPUTS[src].PORT);
        for (unsigned destChannel : dst) {
            commandStr += QString(" %1").arg(settings.OUTPUTS[destChannel].PORT);
        }

        // Create the matrixdata JSON object
        QJsonObject matrixDataObj;
        matrixDataObj["COMMAND"] = commandStr;

        // Create the URL query with matrixdata parameter
        QUrlQuery query;
        query.addQueryItem("matrixdata", QJsonDocument(matrixDataObj).toJson(QJsonDocument::Compact));

        // Add the query to the URL
        url.setQuery(query);

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setTransferTimeout(1000);

        // Add authentication if credentials are provided
        if (!settings.MATRIX_USERNAME.isEmpty() && !settings.MATRIX_PASSWORD.isEmpty()) {
            request.setRawHeader("Authorization", 
                "Basic " + QByteArray(QString("%1:%2").arg(settings.MATRIX_USERNAME, settings.MATRIX_PASSWORD).toUtf8()).toBase64());
        }

        // Use GET request instead of POST
        QNetworkReply* reply = get(request);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                emit updateStatus(QString("Matrix request successful"));
            } else {
                emit updateStatus(QString("Matrix request failed: %2") .arg(reply->errorString()));
            }
            reply->deleteLater();
        });
    }
} 

void MatrixConnect::switchChannels_old(unsigned src, const std::vector<unsigned>& dst)
{
    if (!settings.enabled) {
        emit updateStatus("Video Matrix device settings is not presented.");
        return;
    }

    if (dst.empty()) {
        emit updateStatus("Error: Destination channels list is empty.");
        return;
    }

    if (settings.MATRIX_PROTOCOL == MatrixSettings::Protocol::MT_VIKI) {
        QUrl url;
        url.setScheme("http");  // or https depending on your setup
        url.setHost(settings.MATRIX_HOST);
        url.setPort(settings.MATRIX_PORT);
        url.setPath("/goform/SetMatrixDirectCmd");

        // Construct the command string: "SW SRC DST1 DST2 DST3..."
        QString commandStr = QString("SW %1").arg(src);
        for (unsigned destChannel : dst) {
            commandStr += QString(" %1").arg(destChannel);
        }

        // Create the matrixdata JSON object
        QJsonObject matrixDataObj;
        matrixDataObj["COMMAND"] = commandStr;

        // Create the URL query with matrixdata parameter
        QUrlQuery query;
        query.addQueryItem("matrixdata", QJsonDocument(matrixDataObj).toJson(QJsonDocument::Compact));

        // Add the query to the URL
        url.setQuery(query);

        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        request.setTransferTimeout(1000);

        // Add authentication if credentials are provided
        if (!settings.MATRIX_USERNAME.isEmpty() && !settings.MATRIX_PASSWORD.isEmpty()) {
            request.setRawHeader("Authorization", 
                "Basic " + QByteArray(QString("%1:%2").arg(settings.MATRIX_USERNAME, settings.MATRIX_PASSWORD).toUtf8()).toBase64());
        }

        // Use GET request instead of POST
        QNetworkReply* reply = get(request);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                emit updateStatus(QString("Matrix request successful"));
            } else {
                emit updateStatus(QString("Matrix request failed: %2") .arg(reply->errorString()));
            }
            reply->deleteLater();
        });
    }
} 