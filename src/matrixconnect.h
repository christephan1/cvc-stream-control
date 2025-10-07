// vim:ts=4:sw=4:et:cin

#pragma once

#include <vector>
#include <QNetworkAccessManager>

class MatrixSettings;

class MatrixConnect : public QNetworkAccessManager {
    Q_OBJECT
    public:
        MatrixConnect(const MatrixSettings&);
        virtual ~MatrixConnect() {}

    signals:
        void updateStatus(const QString& msg);

    public slots:
        void captionSourceCaption();
        void captionSourceProjector();
        void captionSourceLectern();
        void captionSource1F();
        void captionSourceB1();
        void switchChannel(unsigned src, unsigned dst);

    private:
        void switchChannels(unsigned src, const std::vector<unsigned>& dst);

    private:
        const MatrixSettings& settings;
};

