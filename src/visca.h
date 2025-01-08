#pragma once

#include <string>
#include <QUdpSocket>

class CameraSettings;

class CameraConnect : public QUdpSocket {
    Q_OBJECT
    public:
        CameraConnect(const CameraSettings&, QObject *parent = nullptr);
        virtual ~CameraConnect() {}

        void viscaMove (int x, int y);
        void viscaLeft (unsigned value);
        void viscaRight (unsigned value);
        void viscaUp (unsigned value);
        void viscaDown (unsigned value);
        void viscaStop ();
        void viscaIn (unsigned value);
        void viscaOut (unsigned value);
        void viscaZoomStop ();
        void viscaFar (unsigned value);
        void viscaNear (unsigned value);
        void viscaFocusStop ();
        void viscaAutoFocus ();
        void viscaManualFocus ();
        void viscaFocusAM ();
        void viscaSet (unsigned value);
        void viscaGo (unsigned value);
        void viscaOn ();
        void viscaOff ();
        void viscaMenu ();
        void viscaMenuUp ();
        void viscaMenuDown ();
        void viscaMenuLeft ();
        void viscaMenuRight ();
        void viscaMenuEnter ();
        void viscaMenuBack ();

    private:
        const CameraSettings& settings;
        void voiSend(const std::string& visca_cmd);

        uint32_t seqNo;
};

