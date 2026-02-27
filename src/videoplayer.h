#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QImage>
#include <QMediaPlayer>
#include <QThread>
#include <QVideoFrame>
#include <QVideoSink>
#include <QVideoWidget>
#include <QWidget>

#include "videoeffects.h"
#include "videoworker.h"

class QLabel;


class VideoPlayer : public QWidget
{
    Q_OBJECT

public:
    VideoPlayer(QWidget *parent = nullptr);
    ~VideoPlayer();

    void playStream(const QString &url);
    void stopStream();
    void pauseVideo();
    void resumeVideo();
    QMediaPlayer *getMediaPlayer() const { return mediaPlayer; }
    void setVideoEffects(VideoEffects *effects);
    void setOverlayEnabled(bool enabled);
    bool isOverlayEnabled() const { return overlayEnabled; }
    void startRecording(const QString &path, const QString &codec, double fps);
    void stopRecording();

signals:
    void errorOccurred(const QString &errorMessage);
    void statusChanged(const QString &status);
    void streamStarted();
    void streamStopped();
    void startPlayback(const QString &url);
    void stopPlayback();
    void pauseStateChanged(bool paused);
    void recordingStarted();
    void recordingFinished(const QString &path);
    void recordingError(const QString &message);

private slots:
    void onMediaError(QMediaPlayer::Error error, const QString &errorString);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void displayFrame(const QImage &image);

private:
    void setupUI();
    void startWorker();

    QMediaPlayer *mediaPlayer;
    QVideoWidget *videoWidget;
    QVideoSink *captureSink;
    VideoEffects *videoEffects;
    QThread *workerThread;
    VideoWorker *worker;
    bool overlayEnabled = true;

};

#endif // VIDEOPLAYER_H
