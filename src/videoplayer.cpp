#include "videoplayer.h"
#include "videoworker.h"
#include <QAudioOutput>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoFrame>

#include <iostream>

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
    std::cout << "Starting VideoWorker thread..." << std::endl;

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

    // Forward recording signals from worker to VideoPlayer
    connect(worker, &VideoWorker::recordingStarted,
            this,   &VideoPlayer::recordingStarted);
    connect(worker, &VideoWorker::recordingFinished,
            this,   &VideoPlayer::recordingFinished);
    connect(worker, &VideoWorker::recordingError,
            this,   &VideoPlayer::recordingError);
    connect(worker, &VideoWorker::autoRecordingStarted,
            this,   &VideoPlayer::autoRecordingStarted);
    connect(worker, &VideoWorker::autoRecordingStopped,
            this,   &VideoPlayer::autoRecordingStopped);

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
    notifyStreamActive(true);  // Tell worker stream is now active
    emit streamStarted();
    emit statusChanged("Playing");
}

void VideoPlayer::stopStream()
{
    mediaPlayer->stop();
    notifyStreamActive(false);  // Tell worker stream is now inactive
    emit streamStopped();
    emit statusChanged("Stopped");
}

void VideoPlayer::notifyStreamActive(bool active)
{
    if (!worker) return;
    QMetaObject::invokeMethod(worker, "setStreamActive",
                              Qt::QueuedConnection, Q_ARG(bool, active));
}

void VideoPlayer::pauseVideo()
{
    emit pauseStateChanged(true);
}

void VideoPlayer::resumeVideo()
{
    emit pauseStateChanged(false);
}

void VideoPlayer::startRecording(const QString &path, const QString &codec, double fps)
{
    if (!worker) return;
    QMetaObject::invokeMethod(worker, "startRecording",
                              Qt::QueuedConnection,
                              Q_ARG(QString, path),
                              Q_ARG(QString, codec),
                              Q_ARG(double,  fps));
}

void VideoPlayer::stopRecording()
{
    if (!worker) return;
    QMetaObject::invokeMethod(worker, "stopRecording", Qt::QueuedConnection);
}

void VideoPlayer::setAutoRecordEnabled(bool enabled)
{
    if (!worker) return;
    QMetaObject::invokeMethod(worker, "setAutoRecordEnabled",
                              Qt::QueuedConnection, Q_ARG(bool, enabled));
}

void VideoPlayer::setAutoRecordDir(const QString &dir)
{
    if (!worker) return;
    QMetaObject::invokeMethod(worker, "setAutoRecordDir",
                              Qt::QueuedConnection, Q_ARG(QString, dir));
}

void VideoPlayer::setAutoRecordTimeout(int seconds)
{
    if (!worker) return;
    QMetaObject::invokeMethod(worker, "setAutoRecordTimeout",
                              Qt::QueuedConnection, Q_ARG(int, seconds));
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
