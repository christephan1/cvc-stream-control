// vim:ts=4:sw=4:et:cin

#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <QNetworkAccessManager>

class MatrixSettings;

class MatrixConnect : public QNetworkAccessManager {
    Q_OBJECT
    public:
        MatrixConnect(const MatrixSettings&);
        virtual ~MatrixConnect() {}

    signals:
        void updateStatus(const QString& msg);
        void connectionFailed();
        void mappingUpdated(const std::unordered_map<unsigned, std::vector<unsigned>>& mapping);
        void addOBSSceneOverrides(const std::unordered_map<uint_fast8_t, uint_fast8_t>& overrides);
        void clearOBSSceneOverrides();

    public slots:
        void switchChannel(unsigned src, unsigned dst);
        void execMacro(unsigned macroIndex);
        void getMapping();
        void resetMatrix();

    private slots:
        void _executeCustomRules(const std::unordered_map<unsigned, std::vector<unsigned>>& current_mapping);

    private:
        void switchChannels(unsigned src, const std::vector<unsigned>& dst);
        void getMapping_impl(
            std::function<void(const std::unordered_map<unsigned, std::vector<unsigned>>&)> onSuccess,
            std::function<void()> onFailure);

        void execMapping(const std::unordered_map<unsigned, std::vector<unsigned>>& mappings_to_exec);

    private:
        const MatrixSettings& settings;
};

