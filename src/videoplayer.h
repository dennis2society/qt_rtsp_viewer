#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QLabel>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>

#include "videoeffects.h"

class VideoWorker;


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
    void setVideoEffects(VideoEffects *effects) { videoEffects = effects; }

signals:
    void errorOccurred(const QString &errorMessage);
    void statusChanged(const QString &status);
    void streamStarted();
    void streamStopped();
    void startPlayback(const QString &url);
    void stopPlayback();

private slots:
    void onMediaError(QMediaPlayer::Error error, const QString &errorString);
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void updateFrame(const QVideoFrame &frame);

private:
    void setupUI();
    void paintFPSOverlay(QImage &image, const QString &fpsText);

    QMediaPlayer *mediaPlayer;
    QVideoWidget *videoWidget;
    QLabel *statusLabel;
    QVideoSink *captureSink;
    QVideoFrame lastFrame;
    QTimer *frameUpdateTimer;
    QElapsedTimer fpsTimer;
    int frameCount;
    double currentFps;
    VideoEffects *videoEffects;
    QThread *workerThread;
    VideoWorker *worker;
    bool paused = false;
    QImage frozenFrame;
};

#endif // VIDEOPLAYER_H
