#ifndef VIDEOWORKER_H
#define VIDEOWORKER_H

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVideoFrame>

#include "opencvqtprocessor.hpp"
#include "videoeffects.h"

#ifdef HAVE_FFMPEG
// Forward declarations for FFmpeg types (full headers only pulled in by .cpp)
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwsContext;
#endif

// ---------------------------------------------------------------------------
// VideoWorker – lives on a dedicated QThread, owns all heavy frame processing
// ---------------------------------------------------------------------------
class VideoWorker : public QObject
{
    Q_OBJECT
public:
    explicit VideoWorker(QObject *parent = nullptr);

    void setVideoEffects(VideoEffects *effects);

public slots:
    void processFrame(const QVideoFrame &frame);
    void setPaused(bool p);
    void setOverlayEnabled(bool enabled);
    void startRecording(const QString &path, const QString &codec, double fps);
    void stopRecording();

signals:
    void frameReady(const QImage &image);
    void recordingStarted();
    void recordingFinished(const QString &path);
    void recordingError(const QString &message);

private:
    void paintFPSOverlay(QImage &image, const QString &fpsText);
#ifdef HAVE_FFMPEG
    bool openRecorder(int width, int height);
    void closeRecorder();
    void encodeAndWrite(AVFrame *frame);
    void writeRecordingFrame(const QImage &image);
#endif

    VideoEffects       *videoEffects   = nullptr;
    OpenCVQtProcessor   openCVProcessor;
    bool                paused         = false;
    bool                overlayEnabled = true;
    QVideoFrame         lastFrame;
    QImage              frozenFrame;
    QImage              previousFrame;
    QElapsedTimer       fpsTimer;
    int                 frameCount     = 0;
    double              currentFps     = 0.0;

    // Recording
    bool                recording      = false;
    bool                recOpen        = false;
    QString             recPath;
    QString             recCodec       = "libx264";
    double              recFps         = 25.0;
    int64_t             recFrameIndex  = 0;
#ifdef HAVE_FFMPEG
    AVFormatContext    *fmtCtx         = nullptr;
    AVCodecContext     *codecCtx       = nullptr;
    AVStream           *videoStream    = nullptr;
    AVFrame            *yuvFrame       = nullptr;
    AVPacket           *pkt            = nullptr;
    SwsContext         *swsCtx         = nullptr;
#endif
};

#endif // VIDEOWORKER_H
