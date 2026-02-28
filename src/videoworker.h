#ifndef VIDEOWORKER_H
#define VIDEOWORKER_H

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVideoFrame>

#include "opencvqtprocessor.hpp"
#include "videoeffects.h"

#include <QDateTime>

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
    void setCameraName(const QString &name);
#ifdef HAVE_FFMPEG
    void startRecording(const QString &path, const QString &codec, double fps);
    void stopRecording();
    void setAutoRecordEnabled(bool enabled);
    void setAutoRecordDir(const QString &dir);
    void setAutoRecordTimeout(int seconds);
#endif
    void setStreamActive(bool active);

signals:
    void frameReady(const QImage &image);
#ifdef HAVE_FFMPEG
    void recordingStarted();
    void recordingFinished(const QString &path);
    void recordingError(const QString &message);
    void autoRecordingStarted();
    void autoRecordingStopped(const QString &path);
#endif

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
    // cleanPreviousFrame: overlay-free snapshot, used by ALL detection/graph functions
    QImage              cleanPreviousFrame;
    bool                motionLogResetOnStream = false; // Track if we've reset log for this stream
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
    QString             cameraName     = "cam_01";

    // Auto-record on motion
    bool                autoRecordEnabled  = false;
    QString             autoRecordDir;
    bool                autoRecording      = false;
    QDateTime           autoRecStartTime;
    // Timeout: keep recording for N seconds after the last motion peak
    int                 autoRecTimeoutMs   = 5000;
    qint64              lastMotionAboveMs  = 0;
    bool                streamActive       = false;
    static constexpr double kAutoRecTrigger = 0.50; // 50% of graph scale
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
