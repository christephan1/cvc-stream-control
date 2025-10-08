// vim:ts=4:sw=4:et:cin

#include "streamdeckkey.h"
#include <iostream>
#include <QBuffer>
#include <QPen>
#include <QPainter>
#include <QTimer>
#include "streamdeckconnect.h"

StreamDeckKey::StreamDeckKey(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& icon)
    : QObject(owner)
    , deckConnect(owner)
    , deckId(deckId_), page(page_), row(row_), column(column_)
    , image(std::move(icon))
{
}

StreamDeckKey_LongPress::StreamDeckKey_LongPress(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& icon, QImage&& iconLongPress)
    : StreamDeckKey(owner, deckId_, page_, row_, column_, std::move(icon))
    , imageLongPress(std::move(iconLongPress))
{
    connect(this, &StreamDeckKey::keyDown, this, &StreamDeckKey_LongPress::detectLongPress_onKeyDown);
    connect(this, &StreamDeckKey::keyUp,   this, &StreamDeckKey_LongPress::detectLongPress_onKeyUp  );
}

StreamDeckKey_Switch::StreamDeckKey_Switch(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& iconOff, QImage&& iconOn,
        bool defaultEn)
    : StreamDeckKey(owner, deckId_, page_, row_, column_, std::move(iconOff))
    , imageOn(std::move(iconOn))
    , en(defaultEn)
{
}

StreamDeckKey_Switch_LongPress::StreamDeckKey_Switch_LongPress(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& iconOff, QImage&& iconOn, QImage&& iconLongPress,
        bool defaultEn)
    : StreamDeckKey_Switch(owner, deckId_, page_, row_, column_, std::move(iconOff), std::move(iconOn), defaultEn)
    , imageLongPress(std::move(iconLongPress))
{
    connect(this, &StreamDeckKey::keyDown, this, &StreamDeckKey_Switch_LongPress::detectLongPress_onKeyDown);
    connect(this, &StreamDeckKey::keyUp,   this, &StreamDeckKey_Switch_LongPress::detectLongPress_onKeyUp  );
}

StreamDeckKey_Scene::StreamDeckKey_Scene(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& iconOff, QImage&& iconOn,
        uint_fast8_t sceneId)
    : StreamDeckKey_Switch(owner, deckId_, page_, row_, column_, std::move(iconOff), std::move(iconOn)), scene(sceneId)
{
}

StreamDeckKey_Tally::StreamDeckKey_Tally(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& iconOff, QImage&& iconOn, QImage&& iconPreview,
        int camId_, bool isActive_, bool isPreview_)
    : StreamDeckKey(owner, deckId_, page_, row_, column_, QImage())
    , camId(camId_), isActive(isActive_), isPreview(isPreview_)
    , imageD(std::move(iconOff)), imageE(std::move(iconOn)), imageP(std::move(iconPreview))
{
    if (camId >= 0) {
        setText(QString::number(camId));
    }
}

StreamDeckKey_Preset::StreamDeckKey_Preset(
        StreamDeckConnect* owner,
        const QString& deckId_, int page_, int row_, int column_,
        QImage&& icon, unsigned presetNo_, bool isEnable_)
    : StreamDeckKey_LongPress(owner, deckId_, page_, row_, column_, std::move(icon), QImage())
    , presetNo(presetNo_), isEnable(isEnable_)
{
    setText(QString::number(presetNo));
}

void StreamDeckKey::onKeyDown()
{
    emit keyDown();
}

void StreamDeckKey::onKeyUp()
{
    emit keyUp();
}

void StreamDeckKey_LongPress::detectLongPress_onKeyDown()
{
    if (!m_longPressEnabled) return;
    if (!timingLongPress) {
        timingLongPress = new QTimer(this);
        timingLongPress->setSingleShot(true);
        connect(timingLongPress, &QTimer::timeout, this, [this]() {
            _longPressed = true;
            updateButton();
            emit longPressed();
        });
        timingLongPress->start(1000);
    }
}

void StreamDeckKey_LongPress::detectLongPress_onKeyUp()
{
    if (timingLongPress) {
        delete timingLongPress;
        timingLongPress = nullptr;
    }
    if (_longPressed) {
        _longPressed = false;
        updateButton();
    } else {
        emit shortPressed();
    }
}

void StreamDeckKey_LongPress::setLongPressEnable(bool en)
{
    m_longPressEnabled = en;
    if (!m_longPressEnabled && timingLongPress) {
        delete timingLongPress;
        timingLongPress = nullptr;
    }
}

void StreamDeckKey_Switch_LongPress::detectLongPress_onKeyDown()
{
    if (!m_longPressEnabled) return;
    if (!timingLongPress) {
        timingLongPress = new QTimer(this);
        timingLongPress->setSingleShot(true);
        connect(timingLongPress, &QTimer::timeout, this, [this]() {
            _longPressed = true;
            updateButton();
            emit longPressed();
        });
        timingLongPress->start(1000);
    }
}

void StreamDeckKey_Switch_LongPress::detectLongPress_onKeyUp()
{
    if (timingLongPress) {
        delete timingLongPress;
        timingLongPress = nullptr;
    }
    if (_longPressed) {
        _longPressed = false;
        updateButton();
    } else {
        emit shortPressed();
    }
}

void StreamDeckKey_Switch_LongPress::setLongPressEnable(bool en)
{
    m_longPressEnabled = en;
    if (!m_longPressEnabled && timingLongPress) {
        delete timingLongPress;
        timingLongPress = nullptr;
    }
}

QString StreamDeckKey::image2dataUri(const QImage& image) /* [static] */
{
    // function body is generated by Gemini
    if (image.isNull()) return QString();
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG"); 
    QString base64String = byteArray.toBase64();
    return "data:image/png;base64," + base64String; 
}

void StreamDeckKey::paintTextOnImage(QImage& image, const QString& str, const QString& title) /* [static] */
{
    QPainter painter(&image);
    painter.setPen(QPen(Qt::white));

    auto paintText = [&painter](const QString& text, const QRect& rect, int initialFontSize) {
        if (text.isEmpty()) return;

        QFont font("Noto Sans", initialFontSize);

        // Make the font smaller until the text fits the rectangle
        QFontMetrics metrics(font);
        int width = metrics.horizontalAdvance(text);
        if (width > rect.width()) {
            font.setPointSizeF(font.pointSizeF() * rect.width() / width);
        }
        painter.setFont(font);

        // Draw the text on the image
        painter.drawText(rect, Qt::AlignCenter, text);
    };

    paintText(str, QRect(20, 100, 248, 158), 96);
    paintText(title, QRect(25, 50, 238, 48), 36);
}

void StreamDeckKey::sendImage(const QImage& image)
{
    QJsonObject payload{
        {"device", deckId},
        {"page", page},
        {"row", row},
        {"column", column}
    };

    if (!image.isNull()) {
        QImage imageWithText = image;
        if (!m_text.isEmpty() || !m_title.isEmpty()) {
            paintTextOnImage(imageWithText, m_text, m_title);
        }
        payload["image"] = image2dataUri(imageWithText);
    }

    deckConnect->sendRequest("setImage", std::move(payload));
}

void StreamDeckKey::updateButton()
{
    sendImage(image);
}

void StreamDeckKey_LongPress::updateButton()
{
    if (_longPressed) {
        sendImage(imageLongPress);
    } else {
        StreamDeckKey::updateButton();
    }
}

void StreamDeckKey_Switch_LongPress::updateButton()
{
    if (_longPressed) {
        sendImage(imageLongPress);
    } else {
        StreamDeckKey_Switch::updateButton();
    }
}

void StreamDeckKey_Switch::updateButton()
{
    if (en) {
        sendImage(imageOn);
    } else {
        StreamDeckKey::updateButton();
    }
}

void StreamDeckKey_Tally::updateButton()
{
    sendImage(isActive? imageE : isPreview? imageP : imageD);
}

void StreamDeckKey_Preset::updateButton()
{
    if (isEnable && !isLongPressed()) {
        StreamDeckKey_LongPress::updateButton();
    } else {
        sendImage(QImage());
    }
}

void StreamDeckKey_Switch::setEnable(bool en_)
{
    if (en != en_) {
        en = en_;
        updateButton();
    }
}

void StreamDeckKey_Tally::setPreview(bool en)
{
    if (isPreview != en) {
        isPreview = en;
        updateButton();
    }
}

void StreamDeckKey_Tally::setActive(bool en)
{
    if (isActive != en) {
        isActive = en;
        updateButton();
    }
}

void StreamDeckKey_Tally::setCamId(int camId_, bool isActive_, bool isPreview_)
{
    if (camId != camId_ || isActive != isActive_ || isPreview != isPreview_) {
        camId = camId_;
        isActive = isActive_;
        isPreview = isPreview_;
        if (camId >= 0) {
            setText(QString::number(camId));
        } else {
            setText("");
        }
        updateButton();
    }
}

void StreamDeckKey_Preset::setPresetNo(unsigned presetNo_, bool isEnable_)
{
    if (presetNo != presetNo_ || isEnable != isEnable_) {
        presetNo = presetNo_;
        isEnable = isEnable_;
        setText(QString::number(presetNo));
        updateButton();
    }
}

