// vim:ts=4:su=4:et:cin

#pragma once

#include <QObject>
#include <QImage>
#include <QString>

class StreamDeckConnect;

class StreamDeckKey : public QObject {
    Q_OBJECT
    public:
        StreamDeckKey(StreamDeckConnect* owner, const QString& deckId_, int page_, int row_, int column_, QImage&& icon);
        virtual ~StreamDeckKey() {}

    signals:
        void keyDown();
        void keyUp();

    public:
        virtual void onKeyDown();
        virtual void onKeyUp();
        virtual void updateButton();

    protected:
        static QString image2dataUri(const QImage&);
        static void paintTextOnImage(QImage&, const QString&);

        StreamDeckConnect* deckConnect;
        const QString& deckId;
        int page, row, column;

    private:
        QImage image;
};

class StreamDeckKey_LongPress : public StreamDeckKey {
    Q_OBJECT
    public:
        StreamDeckKey_LongPress(StreamDeckConnect* owner,
                const QString& deckId_, int page_, int row_, int column_,
                QImage&& icon, QImage&& iconLongPress);
        virtual ~StreamDeckKey_LongPress() {}

    signals:
        void longPressed();

    private slots:
        void detectLongPress_onKeyDown();
        void detectLongPress_onKeyUp();

    public:
        void updateButton() override;

    protected:
        bool isLongPressed() const { return _longPressed; }

    private:
        QImage imageLongPress;
        bool _longPressed = false;
        QTimer* timingLongPress = nullptr;

};

class StreamDeckKey_Switch : public StreamDeckKey {
    Q_OBJECT
    public:
        StreamDeckKey_Switch(StreamDeckConnect* owner, const QString& deckId_, int page_, int row_, int column_, QImage&& iconOff, QImage&& iconOn, bool defaultEn = false);
        virtual ~StreamDeckKey_Switch() {}

        //[TODO] to make this private, need to delay execution after constructor
        void updateButton() override;

        void setEnable(bool en);

    private:
        bool en;
        QImage imageOn;
};

class StreamDeckKey_Scene : public StreamDeckKey_Switch {
    Q_OBJECT
    public:
        StreamDeckKey_Scene(StreamDeckConnect* owner, const QString& deckId_, int page_, int row_, int column_, QImage&& iconOff, QImage&& iconOn, uint_fast8_t sceneId);
        virtual ~StreamDeckKey_Scene() {}

        uint_fast8_t getSceneId() { return scene; }

    private:
        uint_fast8_t scene;
};

class StreamDeckKey_Tally : public StreamDeckKey {
    Q_OBJECT
    public:
        StreamDeckKey_Tally(
                StreamDeckConnect* owner,
                const QString& deckId_, int page_, int row_, int column_,
                QImage&& iconOff, QImage&& iconOn, QImage&& iconPreview,
                int camID, bool isActive, bool isPreview);
        virtual ~StreamDeckKey_Tally() {}

        void updateButton() override;

        void setCamId(int camId, bool isActive, bool isPreview);
        void setPreview(bool en);
        void setActive(bool en);

    private:
        int camId;
        bool isActive, isPreview;

        QImage imageD, imageE, imageP;
};

class StreamDeckKey_Preset : public StreamDeckKey_LongPress {
    Q_OBJECT
    public:
        StreamDeckKey_Preset(
                StreamDeckConnect* owner,
                const QString& deckId_, int page_, int row_, int column_,
                QImage&& icon, unsigned presetNo, bool isEnable);
        virtual ~StreamDeckKey_Preset() {}

        void updateButton() override;

        void setPresetNo(unsigned presetNo, bool isEnable);

    private:
        unsigned presetNo;
        bool isEnable;
        QImage image;
};

