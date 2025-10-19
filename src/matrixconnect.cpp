// vim:ts=4:sw=4:et:cin

#include "matrixconnect.h"
#include <unordered_set>
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

    execMapping(settings.MACROS[macroIndex].MAPPING);
}

void MatrixConnect::resetMatrix()
{
    execMapping(settings.DEFAULT_MAPPING);
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
    getMapping_impl(
        [this](const std::unordered_map<unsigned, std::vector<unsigned>>& mapping) {
            emit mappingUpdated(mapping);
        },
        []() {
            // Failure is handled and reported inside getMapping_impl,
            // so nothing to do here.
        });
}

void MatrixConnect::getMapping_impl(
    std::function<void(const std::unordered_map<unsigned, std::vector<unsigned>>&)> onSuccess,
    std::function<void()> onFailure)
{
    if (!settings.enabled) {
        emit updateStatus("Video Matrix device settings is not presented.");
        if (onFailure) onFailure();
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

        connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess, onFailure]() {
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
                        
                        if (onSuccess) onSuccess(std::move(input_to_outputs_mapping));
                        
                        QString statusMsg = "Matrix request successful";
                        if (!warnings.isEmpty()) {
                            statusMsg += " with warnings: " + warnings.join("; ");
                        }
                        emit updateStatus(std::move(statusMsg));
                    } else {
                        emit updateStatus(QString("Matrix getMapping failed: Missing 'SWS' key in JSON response."));
                        if (onFailure) onFailure();
                    }
                } else {
                     emit updateStatus(QString("Matrix getMapping failed: Invalid JSON response."));
                     if (onFailure) onFailure();
                }
            } else {
                emit updateStatus(QString("Matrix request failed: %1").arg(reply->errorString()));
                if (onFailure) onFailure();
            }
            reply->deleteLater();
        });
    } else {
        if (onFailure) onFailure();
    }
}

/**
 * @brief Asynchronously executes a list of channel mappings.
 *
 * This function first fetches the current mapping from the matrix device.
 *
 * On success, it compares the desired mappings with the current ones. For each source,
 * it calculates which destination channels are not already active and sends a command
 * to switch only those new channels.
 *
 * If fetching the current mapping fails, it falls back to executing the full
 * list of provided mappings to ensure the desired state is attempted.
 *
 * @param mappings_to_exec A map where the key is the source index and the value
 *                         is a vector of destination indices to be activated.
 */
void MatrixConnect::execMapping(const std::unordered_map<unsigned, std::vector<unsigned>>& mappings_to_exec)
{
    // Define the success callback for when the current mapping is fetched successfully.
    auto onSuccess = [this, mappings_to_exec](const std::unordered_map<unsigned, std::vector<unsigned>>& current_mapping) {
        // Iterate over each source-to-destinations mapping requested for execution.
        for (const auto& mapping_item : mappings_to_exec) {
            unsigned src = mapping_item.first;
            const std::vector<unsigned>& dsts = mapping_item.second;

            // Create a set of current destinations for the source for efficient lookup.
            std::unordered_set<unsigned> current_dsts_set;
            auto it = current_mapping.find(src);
            if (it != current_mapping.end()) {
                current_dsts_set.insert(it->second.begin(), it->second.end());
            }

            // Determine which destinations are new and need to be added.
            std::vector<unsigned> dsts_to_add;
            for (unsigned dst : dsts) {
                // If a desired destination is not in the current set, add it to the list.
                if (current_dsts_set.find(dst) == current_dsts_set.end()) {
                    dsts_to_add.push_back(dst);
                }
            }

            // If there are any new destinations to add, send the switch command.
            if (!dsts_to_add.empty()) {
                switchChannels(src, dsts_to_add);
            }
        }
    };

    // Define the failure callback for when fetching the current mapping fails.
    auto onFailure = [this, mappings_to_exec]() {
        // Fallback: execute the full list of mappings without comparison.
        for (const auto& mapping : mappings_to_exec) {
            switchChannels(mapping.first, mapping.second);
        }
    };

    // Asynchronously get the current mapping and trigger the appropriate callback.
    getMapping_impl(onSuccess, onFailure);
}
