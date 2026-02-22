#include "videoplayer.h"
#include <QAudioOutput>
#include <QFontMetrics>
#include <QLabel>
#include <QPainter>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>
#include <iostream>

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
    , mediaPlayer(nullptr)
    , videoWidget(nullptr)
    , statusLabel(nullptr)
    , captureSink(nullptr)
    , frameUpdateTimer(nullptr)
    , frameCount(0)
    , currentFps(0.0)
{
    setupUI();

    // Initialize media player
    mediaPlayer = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    // Create separate sink for frame capture (non-blocking)
    captureSink = new QVideoSink(this);
    connect(captureSink, &QVideoSink::videoFrameChanged, this, &VideoPlayer::updateFrame);
    mediaPlayer->setVideoOutput(captureSink);

    // Create timer for periodic frame processing
    frameUpdateTimer = new QTimer(this);
    frameUpdateTimer->setInterval(33); // ~30 FPS

    // Connect media player signals
    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, &VideoPlayer::onMediaError);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &VideoPlayer::onMediaStatusChanged);
}

VideoPlayer::~VideoPlayer()
{
}

void VideoPlayer::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Create video widget for display
    videoWidget = new QVideoWidget(this);
    videoWidget->setMinimumHeight(400);
    videoWidget->setStyleSheet("QVideoWidget { background-color: #000000; border: 1px solid #ccc; }");

    layout->addWidget(videoWidget, 1);
    setLayout(layout);
}

void VideoPlayer::playStream(const QString &url)
{
    if (url.isEmpty()) {
        emit errorOccurred("Please provide an RTSP URL");
        return;
    }

    frameCount = 0;
    currentFps = 0.0;
    fpsTimer.start();

    mediaPlayer->setSource(QUrl(url));
    mediaPlayer->play();
    frameUpdateTimer->start();
    emit streamStarted();
    emit statusChanged("Playing");
}

void VideoPlayer::stopStream()
{
    mediaPlayer->stop();
    frameUpdateTimer->stop();
    emit streamStopped();
    emit statusChanged("Stopped");
}

void VideoPlayer::onMediaError(QMediaPlayer::Error error, const QString &errorString)
{
    emit errorOccurred("Playback Error: " + errorString + "\n\nMake sure the RTSP URL is correct and accessible.");
    stopStream();
}

void VideoPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadingMedia:
        emit statusChanged("Loading stream...");
        break;
    case QMediaPlayer::LoadedMedia:
        emit statusChanged("Stream loaded - Playing");
        break;
    case QMediaPlayer::EndOfMedia:
        emit statusChanged("Stream ended");
        emit streamStopped();
        break;
    case QMediaPlayer::NoMedia:
        emit statusChanged("No media");
        break;
    default:
        break;
    }
}

void VideoPlayer::updateFrame(const QVideoFrame &frame)
{
    // Calculate FPS
    frameCount++;
    qint64 elapsed = fpsTimer.elapsed();

    if (elapsed >= 1000) { // Update FPS every second
        currentFps = (frameCount * 1000.0) / elapsed;
        frameCount = 0;
        fpsTimer.restart();
    }

    QImage image = frame.toImage();
    if (effectsSidebar->getEffects()->isGrayscaleEnabled()) {
        image = image.convertToFormat(QImage::Format_Grayscale8);
    }
    paintFPSOverlay(image, QString("FPS: %1").arg(currentFps, 0, 'f', 1));
    videoWidget->videoSink()->setVideoFrame(QVideoFrame(image));
}

void VideoPlayer::paintFPSOverlay(QImage &image, const QString &fpsText)
{
    if (image.isNull())
        return;

    if (image.format() != QImage::Format_ARGB32 && image.format() != QImage::Format_ARGB32_Premultiplied) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QFont font;
    font.setPointSize(28);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);

    // 🔹 Build overlay text (FPS + resolution)
    QString resolutionText = QString("%1x%2").arg(image.width()).arg(image.height());

    QString overlayText = QString("%1 | %2").arg(fpsText).arg(resolutionText);

    // Measure text
    QRect textRect = fm.boundingRect(overlayText);

    const int paddingX = 20;
    const int paddingY = 12;

    textRect.adjust(-paddingX, -paddingY, paddingX, paddingY);

    const int margin = 20;
    textRect.moveTopRight(QPoint(image.width() - margin, textRect.height()));

    painter.fillRect(textRect, QColor(0, 0, 0, 180));
    painter.setPen(Qt::green);

    painter.drawText(textRect, Qt::AlignCenter, overlayText);

    painter.end();
}