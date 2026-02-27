#ifndef VIDEOWORKER_H
#define VIDEOWORKER_H

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QVideoFrame>

#include "opencvqtprocessor.hpp"
#include "videoeffects.h"

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

signals:
    void frameReady(const QImage &image);

private:
    void paintFPSOverlay(QImage &image, const QString &fpsText);

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
};

#endif // VIDEOWORKER_H
