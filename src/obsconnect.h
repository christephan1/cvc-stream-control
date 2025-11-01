// vim:ts=4:sw=4:et:cin

#pragma once

#include <map>
#include <unordered_map>
#include <QWebSocket>
#include <QJsonObject>

QT_BEGIN_NAMESPACE
class QJsonArray;
QT_END_NAMESPACE

class OBSSettings;

class OBSConnect : public QWebSocket {
    Q_OBJECT
    public:
        OBSConnect(const OBSSettings&);
        virtual ~OBSConnect() {}

        void switchToScene(uint_fast8_t sceneId, uint_fast8_t camId);
        void switchStudioMode();

        uint_fast8_t getPrevSceneId(uint_fast8_t sceneId) const;
        uint_fast8_t getNextSceneId(uint_fast8_t sceneId) const;

    public slots:
        void addSceneOverrides(const std::unordered_map<uint_fast8_t, uint_fast8_t>& overrides);
        void clearSceneOverrides();

    signals:
        void updateStatus(const QString& msg);
        void currentSceneChanged(uint_fast8_t sceneId, uint_fast8_t camId);
        void studioModeChanged(bool en);

    private:
        void connectOBS();

        void sendRequest(const char* requestType, QJsonObject&& requestData = QJsonObject());
        void sendRequest(const int op, QJsonObject&& d);

        void processSceneList(QJsonArray&&);
        void createScene(QString&& sceneName);
        void removeScene(const QString& sceneName);
        std::pair<uint_fast8_t, uint_fast8_t> getSceneIdFromName(const QString& sceneName);

    private slots:
        void processOBSMsg(const QString& msg);

    private:
        const OBSSettings& settings;

        std::map<uint_fast8_t, std::map<uint_fast8_t, QString>> sceneMap; //sceneId->camId->sceneName
        bool isStudioMode = false;

        std::unordered_map<uint_fast8_t, uint_fast8_t> sceneIdOverride; //sceneId->overrided sceneId
};

