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

    private:
        void switchChannels(const std::vector<int>& src, const std::vector<int>& dst);

    private:
        const MatrixSettings& settings;
};

