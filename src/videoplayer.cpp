#include "videoplayer.h"
#include "opencvqtprocessor.hpp"
#include <QAudioOutput>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFontMetrics>
#include <QPainter>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>

// ---------------------------------------------------------------------------
// VideoWorker – lives on a dedicated QThread, owns all heavy frame processing
// ---------------------------------------------------------------------------
class VideoWorker : public QObject
{
    Q_OBJECT
public:
    explicit VideoWorker(QObject *parent = nullptr) : QObject(parent)
    {
        fpsTimer.start();
    }

    void setVideoEffects(VideoEffects *effects) { videoEffects = effects; }

public slots:
    void processFrame(const QVideoFrame &frame)
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

    void setPaused(bool p)
    {
        paused = p;
        if (p && lastFrame.isValid()) {
            frozenFrame = lastFrame.toImage();
        }
    }

    void setOverlayEnabled(bool enabled) { overlayEnabled = enabled; }

signals:
    void frameReady(const QImage &image);

private:
    void paintFPSOverlay(QImage &image, const QString &fpsText)
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
        const int paddingX    = 20;
        const int paddingY    = 12;
        const int margin      = 20;
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

    VideoEffects       *videoEffects  = nullptr;
    OpenCVQtProcessor   openCVProcessor;
    bool                paused        = false;
    bool                overlayEnabled = true;
    QVideoFrame         lastFrame;
    QImage              frozenFrame;
    QImage              previousFrame;
    QElapsedTimer       fpsTimer;
    int                 frameCount    = 0;
    double              currentFps    = 0.0;
};

#include "videoplayer.moc"

// ---------------------------------------------------------------------------
// VideoPlayer
// ---------------------------------------------------------------------------

VideoPlayer::VideoPlayer(QWidget *parent)
    : QWidget(parent)
    , mediaPlayer(nullptr)
    , videoWidget(nullptr)
    , captureSink(nullptr)
    , videoEffects(nullptr)
    , workerThread(nullptr)
    , worker(nullptr)
{
    setupUI();

    mediaPlayer = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);

    captureSink = new QVideoSink(this);
    mediaPlayer->setVideoOutput(captureSink);

    connect(mediaPlayer, &QMediaPlayer::errorOccurred,    this, &VideoPlayer::onMediaError);
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &VideoPlayer::onMediaStatusChanged);
}

VideoPlayer::~VideoPlayer()
{
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void VideoPlayer::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    videoWidget = new QVideoWidget(this);
    videoWidget->setMinimumHeight(400);
    videoWidget->setStyleSheet("QVideoWidget { background-color: #000000; border: 1px solid #ccc; }");

    layout->addWidget(videoWidget, 1);
    setLayout(layout);
}

void VideoPlayer::startWorker()
{
    if (workerThread)
        return; // already started

    workerThread = new QThread(this);
    worker       = new VideoWorker(); // no parent – will be moved to thread
    worker->setVideoEffects(videoEffects);
    worker->setOverlayEnabled(overlayEnabled);
    worker->moveToThread(workerThread);

    // Worker is deleted automatically when the thread finishes
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);

    // captureSink lives on the main thread; videoFrameChanged → worker (queued)
    connect(captureSink, &QVideoSink::videoFrameChanged,
            worker,      &VideoWorker::processFrame);

    // Processed frame comes back to main thread for display
    connect(worker, &VideoWorker::frameReady,
            this,   &VideoPlayer::displayFrame);

    // Propagate pause state changes to worker
    connect(this,   &VideoPlayer::pauseStateChanged,
            worker, &VideoWorker::setPaused);

    workerThread->start();
}

void VideoPlayer::setVideoEffects(VideoEffects *effects)
{
    videoEffects = effects;
    startWorker();
}

void VideoPlayer::setOverlayEnabled(bool enabled)
{
    overlayEnabled = enabled;
    if (worker)
        QMetaObject::invokeMethod(worker, "setOverlayEnabled",
                                  Qt::QueuedConnection, Q_ARG(bool, enabled));
}

void VideoPlayer::playStream(const QString &url)
{
    if (url.isEmpty()) {
        emit errorOccurred("Please provide an RTSP URL");
        return;
    }

    mediaPlayer->setSource(QUrl(url));
    mediaPlayer->play();
    emit streamStarted();
    emit statusChanged("Playing");
}

void VideoPlayer::stopStream()
{
    mediaPlayer->stop();
    emit streamStopped();
    emit statusChanged("Stopped");
}

void VideoPlayer::pauseVideo()
{
    emit pauseStateChanged(true);
}

void VideoPlayer::resumeVideo()
{
    emit pauseStateChanged(false);
}

void VideoPlayer::displayFrame(const QImage &image)
{
    videoWidget->videoSink()->setVideoFrame(QVideoFrame(image));
}

void VideoPlayer::onMediaError(QMediaPlayer::Error error, const QString &errorString)
{
    Q_UNUSED(error)
    emit errorOccurred("Playback Error: " + errorString
                       + "\n\nMake sure the RTSP URL is correct and accessible.");
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
