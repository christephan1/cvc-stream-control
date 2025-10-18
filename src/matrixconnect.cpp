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

void MatrixConnect::switchChannel(unsigned src, unsigned dst)
{
    switchChannels(src, {dst});
}

void MatrixConnect::execMacro(unsigned macroIndex)
{
    if (macroIndex >= settings.MACROS.size()) {
        emit updateStatus(QString("Error: Macro index %1 out of range.").arg(macroIndex));
        return;
    }

    for (const auto& mapping : settings.MACROS[macroIndex].MAPPING) {
        switchChannels(mapping.first, mapping.second);
    }
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
                emit updateStatus(QString("Matrix request failed: %1") .arg(reply->errorString()));
            }
            reply->deleteLater();
        });
    }
} 

void MatrixConnect::getMapping()
{
    if (!settings.enabled) {
        emit updateStatus("Video Matrix device settings is not presented.");
        return;
    }

    if (settings.MATRIX_PROTOCOL == MatrixSettings::Protocol::MT_VIKI) {
        QUrl url;
        url.setScheme("http");  // or https depending on your setup
        url.setHost(settings.MATRIX_HOST);
        url.setPort(settings.MATRIX_PORT);
        url.setPath("/goform/GetMatrixSwitcher");

        // Create the matrixdata JSON object
        QJsonObject matrixDataObj;
        matrixDataObj["COMMAND"] = "GETSWS";

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
                QByteArray responseData = reply->readAll();
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

                //parse the response
                //response format: "SWS SRC1 SRC2 SRC3 ..."
                if (jsonDoc.isObject()) {
                    QJsonObject jsonObj = jsonDoc.object();
                    if (jsonObj.contains("SWS") && jsonObj["SWS"].isString()) {
                        QString sws = jsonObj["SWS"].toString();
                        QStringList inputPorts = sws.split(' ', Qt::SkipEmptyParts);
                        
                        std::unordered_map<unsigned, std::vector<unsigned>> input_to_outputs_mapping;
                        QStringList warnings;

                        for (int i = 0; i < inputPorts.size(); ++i) {
                            // The SWS response lists input ports sequentially for each output port, starting from port 1.
                            // So, the loop index `i` corresponds to `output_port - 1`.
                            unsigned output_port = i + 1;
                            auto it_out = settings.OUTPUT_PORT_TO_IDX.find(output_port);
                            if (it_out == settings.OUTPUT_PORT_TO_IDX.end()) {
                                // It's normal that some output ports are not configured. skip it.
                                continue;
                            }
                            unsigned outputIdx = it_out->second;
                            
                            // Convert the input port from the response to its corresponding index.
                            bool ok;
                            unsigned inputPort = inputPorts[i].toUInt(&ok);
                            if (ok) {
                                auto it = settings.INPUT_PORT_TO_IDX.find(inputPort);
                                if (it != settings.INPUT_PORT_TO_IDX.end()) {
                                    unsigned inputIdx = it->second;
                                    // Build the mapping from input index to a list of output indices.
                                    input_to_outputs_mapping[inputIdx].push_back(outputIdx);
                                } else {
                                    warnings.append(QString("Unknown input port %1 for output port %2.").arg(inputPort).arg(output_port));
                                }
                            } else {
                                warnings.append(QString("Invalid port value '%1' for output port %2.").arg(inputPorts[i]).arg(output_port));
                            }
                        }

                        emit mappingUpdated(std::move(input_to_outputs_mapping));
                        
                        QString statusMsg = "Matrix request successful";
                        if (!warnings.isEmpty()) {
                            statusMsg += " with warnings: " + warnings.join("; ");
                        }
                        emit updateStatus(std::move(statusMsg));
                    } else {
                        emit updateStatus(QString("Matrix getMapping failed: Missing 'SWS' key in JSON response."));
                    }
                } else {
                     emit updateStatus(QString("Matrix getMapping failed: Invalid JSON response."));
                }
            } else {
                emit updateStatus(QString("Matrix request failed: %1").arg(reply->errorString()));
            }
            reply->deleteLater();
        });
    }
}
