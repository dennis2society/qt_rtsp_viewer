#include "videoworker.h"

#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>

VideoWorker::VideoWorker(QObject *parent)
    : QObject(parent)
{
    fpsTimer.start();
}

void VideoWorker::setVideoEffects(VideoEffects *effects)
{
    videoEffects = effects;
}

void VideoWorker::processFrame(const QVideoFrame &frame)
{
    if (!videoEffects)
        return;

    if (!paused) {
        frameCount++;
        qint64 elapsed = fpsTimer.elapsed();
        if (elapsed >= 1000) {
            currentFps = (frameCount * 1000.0) / elapsed;
            frameCount = 0;
            fpsTimer.restart();
        }

        lastFrame = frame;
        QImage image = frame.toImage();

        int brightness = videoEffects->getBrightnessAmount();
        int contrast   = videoEffects->getContrastAmount();
        if (brightness != 0 || contrast != 0)
            image = videoEffects->applyBrightnessContrastToImage(image);

        if (videoEffects->isGrayscaleEnabled())
            image = image.convertToFormat(QImage::Format_Grayscale8);

        if (videoEffects->getBlurAmount() > 0)
            image = openCVProcessor.applyGaussBlur(
                image,
                videoEffects->getBlurAmount() * 2 + 1,
                videoEffects->getBlurAmount() * 0.5);

        if (videoEffects->isMotionDetectionEnabled() && !previousFrame.isNull())
            image = openCVProcessor.applyMotionDetectionOverlay(
                image, previousFrame, videoEffects->getMotionSensitivity());

        if (videoEffects->isMotionVectorsEnabled() && !previousFrame.isNull())
            image = openCVProcessor.applyMotionVectorsOverlay(image, previousFrame);

        if (videoEffects->isFaceDetectionEnabled())
            image = openCVProcessor.applyFaceDetection(image);

        previousFrame = image.copy();
        frozenFrame   = image;

        paintFPSOverlay(image, QString("FPS: %1").arg(currentFps, 0, 'f', 1));
        emit frameReady(image);
    } else {
        if (!frozenFrame.isNull()) {
            QImage displayImage = frozenFrame.copy();
            paintFPSOverlay(displayImage, QString("FPS: %1").arg(currentFps, 0, 'f', 1));
            emit frameReady(displayImage);
        }
    }
}

void VideoWorker::setPaused(bool p)
{
    paused = p;
    if (p && lastFrame.isValid()) {
        frozenFrame = lastFrame.toImage();
    }
}

void VideoWorker::setOverlayEnabled(bool enabled)
{
    overlayEnabled = enabled;
}

void VideoWorker::paintFPSOverlay(QImage &image, const QString &fpsText)
{
    if (image.isNull() || !overlayEnabled)
        return;

    if (image.format() != QImage::Format_ARGB32
        && image.format() != QImage::Format_ARGB32_Premultiplied)
        image = image.convertToFormat(QImage::Format_ARGB32);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont font;
    font.setPointSize(28);
    font.setBold(true);
    painter.setFont(font);
    QFontMetrics fm(font);

    // Date/time – top left
    const int dtMargin   = 20;
    const int dtPaddingX = 20;
    const int dtPaddingY = 12;
    QString dateTimeText = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QRect dateTimeRect   = fm.boundingRect(dateTimeText);
    dateTimeRect.adjust(-dtPaddingX, -dtPaddingY, dtPaddingX, dtPaddingY);
    dateTimeRect.moveTopLeft(QPoint(dtMargin, dtMargin));
    painter.fillRect(dateTimeRect, QColor(0, 0, 0, 180));
    painter.setPen(Qt::green);
    painter.drawText(dateTimeRect, Qt::AlignLeft | Qt::AlignVCenter, dateTimeText);

    // FPS + resolution – top right
    const int paddingX     = 20;
    const int paddingY     = 12;
    const int margin       = 20;
    QString resolutionText = QString("%1x%2").arg(image.width()).arg(image.height());
    QString overlayText    = QString("%1 | %2").arg(fpsText).arg(resolutionText);
    QRect textRect         = fm.boundingRect(overlayText);
    textRect.adjust(-paddingX, -paddingY, paddingX, paddingY);
    textRect.moveTopRight(QPoint(image.width() - margin, dtMargin));
    painter.fillRect(textRect, QColor(0, 0, 0, 180));
    painter.setPen(Qt::green);
    painter.drawText(textRect, Qt::AlignCenter, overlayText);

    painter.end();
}
