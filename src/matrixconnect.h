// vim:ts=4:sw=4:et:cin

#pragma once

#include <vector>
#include <unordered_map>
#include <QNetworkAccessManager>

class MatrixSettings;

class MatrixConnect : public QNetworkAccessManager {
    Q_OBJECT
    public:
        MatrixConnect(const MatrixSettings&);
        virtual ~MatrixConnect() {}

    signals:
        void updateStatus(const QString& msg);
        void mappingUpdated(const std::unordered_map<unsigned, std::vector<unsigned>>& mapping);

    public slots:
        void captionSourceCaption();
        void captionSourceProjector();
        void captionSourceLectern();
        void captionSource1F();
        void captionSourceB1();
        void switchChannel(unsigned src, unsigned dst);
        void getMapping();
        void resetMatrix();

    private:
        void switchChannels(unsigned src, const std::vector<unsigned>& dst);
        void switchChannels_old(unsigned src, const std::vector<unsigned>& dst);

    private:
        const MatrixSettings& settings;
};

