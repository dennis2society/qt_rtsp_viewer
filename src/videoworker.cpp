#include "videoworker.h"

#include <QDateTime>
#include <QFontMetrics>
#include <QPainter>

#ifdef HAVE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#endif

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
            image = openCVProcessor.applyBrightnessContrast(image, brightness, contrast);

        int colorTemp = videoEffects->getColorTemperature();
        if (colorTemp != 0) {
            openCVProcessor.setColorTemperature(colorTemp);
            image = openCVProcessor.applyColorTemperature(image);
        }

        if (videoEffects->isGrayscaleEnabled())
            image = image.convertToFormat(QImage::Format_Grayscale8);

        if (videoEffects->getBlurAmount() > 0)
            image = openCVProcessor.applyGaussBlur(
                image,
                videoEffects->getBlurAmount() * 2 + 1,
                videoEffects->getBlurAmount() * 0.5);

        // Snapshot clean processed frame – this is the only source all
        // detection/overlay functions are allowed to compute from.
        const QImage cleanImage = image;

        // Start compositing: each overlay draws on top of the previous result
        // but always computes from the clean pair (cleanImage / cleanPreviousFrame).
        QImage result = cleanImage;

        if (videoEffects->isMotionDetectionEnabled() && !cleanPreviousFrame.isNull())
            result = openCVProcessor.applyMotionDetectionOverlay(
                result, cleanImage, cleanPreviousFrame,
                videoEffects->getMotionSensitivity());

        if (videoEffects->isMotionVectorsEnabled() && !cleanPreviousFrame.isNull())
            result = openCVProcessor.applyMotionVectorsOverlay(
                result, cleanImage, cleanPreviousFrame);

        if (videoEffects->isFaceDetectionEnabled())
            result = openCVProcessor.applyFaceDetection(result, cleanImage);

        if (videoEffects->isMotionGraphEnabled() && !cleanPreviousFrame.isNull()) {
            double level = openCVProcessor.computeMotionLevel(
                cleanImage, cleanPreviousFrame,
                videoEffects->getMotionGraphSensitivity());
            // Grid highlight overlay (uses lastCellLevels populated by computeMotionLevel above)
            result = openCVProcessor.applyGridMotionOverlay(result);
            result = openCVProcessor.applyMotionGraphOverlay(result, level);

#ifdef HAVE_FFMPEG
            // --- Auto-record on motion ---
            if (autoRecordEnabled && !autoRecordDir.isEmpty()) {
                const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                if (level >= kAutoRecTrigger)
                    lastMotionAboveMs = nowMs;

                if (!autoRecording && level >= kAutoRecTrigger && !recording) {
                    // Start a new motion-triggered recording
                    autoRecStartTime = QDateTime::currentDateTime();
                    QString filename = autoRecStartTime.toString("yyyy-MM-dd_HH:mm:ss")
                                       + "_motion_recording.mp4";
                    QString fullPath = autoRecordDir + "/" + filename;
                    recPath  = fullPath;
                    recCodec = "libx265";
                    recFps   = 25.0;
                    recording     = true;
                    autoRecording = true;
                    emit autoRecordingStarted();
                } else if (autoRecording && (nowMs - lastMotionAboveMs) > autoRecTimeoutMs) {
                    // Motion dropped below threshold for cooldown period – stop
                    stopRecording();
                    QString finishedPath = recPath;
                    autoRecording = false;
                    emit autoRecordingStopped(finishedPath);
                }
            }
#endif
        }

        cleanPreviousFrame = cleanImage; // store clean frame for next iteration
        frozenFrame = result;
        image = result;

        paintFPSOverlay(image, QString("FPS: %1").arg(currentFps, 0, 'f', 1));
#ifdef HAVE_FFMPEG
        if (recording)
            writeRecordingFrame(image);
#endif
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

void VideoWorker::setAutoRecordEnabled(bool enabled)
{
    autoRecordEnabled = enabled;
    // If disabled while auto-recording, stop the current auto-recording
    if (!enabled && autoRecording) {
        stopRecording();
        QString finishedPath = recPath;
        autoRecording = false;
        emit autoRecordingStopped(finishedPath);
    }
}

void VideoWorker::setAutoRecordDir(const QString &dir)
{
    autoRecordDir = dir;
}

void VideoWorker::setAutoRecordTimeout(int seconds)
{
    autoRecTimeoutMs = qBound(1, seconds, 120) * 1000;
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

// ---------------------------------------------------------------------------
// Recording – libavcodec / libavformat / libswscale
// ---------------------------------------------------------------------------

#ifdef HAVE_FFMPEG

void VideoWorker::startRecording(const QString &path, const QString &codec, double fps)
{
#ifdef HAVE_FFMPEG
    if (recording)
        stopRecording();

    recPath  = path;
    recCodec = codec.isEmpty() ? "libx264" : codec;
    recFps   = (fps > 0) ? fps : 25.0;
    // openRecorder() is called lazily on the first frame so we know the frame size
    recording = true;
#else
    Q_UNUSED(path)
    Q_UNUSED(codec)
    Q_UNUSED(fps)
    emit recordingError(
        "Recording is not available.\n"
        "This application was built without FFmpeg support.\n"
        "Rebuild with libavcodec, libavformat, libavutil and libswscale installed.");
#endif
}

void VideoWorker::stopRecording()
{
#ifdef HAVE_FFMPEG
    if (!recording && !recOpen)
        return;

    recording = false;
    if (recOpen) {
        encodeAndWrite(nullptr); // flush encoder
        av_write_trailer(fmtCtx);
        closeRecorder();
    }
    emit recordingFinished(recPath);
#endif
}

bool VideoWorker::openRecorder(int width, int height)
{
    char errbuf[256];

    // Locate encoder
    const AVCodec *codec = avcodec_find_encoder_by_name(recCodec.toStdString().c_str());
    if (!codec)
        codec = avcodec_find_encoder_by_name("libx264"); // fallback
    if (!codec) {
        emit recordingError("Could not find H.264 encoder – is libx264 installed?");
        return false;
    }

    // Output format context (container is derived from filename extension)
    int ret = avformat_alloc_output_context2(&fmtCtx, nullptr, nullptr,
                                             recPath.toStdString().c_str());
    if (ret < 0 || !fmtCtx) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        emit recordingError(QString("Could not create output context: %1").arg(errbuf));
        return false;
    }

    // Codec context
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        emit recordingError("Could not allocate codec context.");
        closeRecorder();
        return false;
    }
    int fpsInt          = qMax(1, qRound(recFps));
    codecCtx->width     = width;
    codecCtx->height    = height;
    codecCtx->pix_fmt   = AV_PIX_FMT_YUV420P;
    codecCtx->time_base = { 1, fpsInt };
    codecCtx->framerate = { fpsInt, 1 };
    codecCtx->bit_rate  = 4'000'000;
    if (fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "preset", "fast", 0);
    av_dict_set(&opts, "crf",    "23",   0);
    ret = avcodec_open2(codecCtx, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        emit recordingError(QString("Could not open codec: %1").arg(errbuf));
        closeRecorder();
        return false;
    }

    // Video stream
    videoStream = avformat_new_stream(fmtCtx, nullptr);
    if (!videoStream) {
        emit recordingError("Could not create video stream.");
        closeRecorder();
        return false;
    }
    avcodec_parameters_from_context(videoStream->codecpar, codecCtx);
    videoStream->time_base = codecCtx->time_base;

    // Open output file for writing
    if (!(fmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmtCtx->pb, recPath.toStdString().c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            emit recordingError(QString("Could not open output file: %1").arg(errbuf));
            closeRecorder();
            return false;
        }
    }

    ret = avformat_write_header(fmtCtx, nullptr);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        emit recordingError(QString("Could not write file header: %1").arg(errbuf));
        closeRecorder();
        return false;
    }

    // YUV420P destination frame – dimensions/format must be set BEFORE av_frame_get_buffer
    yuvFrame = av_frame_alloc();
    if (!yuvFrame) {
        emit recordingError("Could not allocate YUV frame.");
        closeRecorder();
        return false;
    }
    yuvFrame->format = AV_PIX_FMT_YUV420P;
    yuvFrame->width  = width;
    yuvFrame->height = height;
    if (av_frame_get_buffer(yuvFrame, 0) < 0) {
        emit recordingError("Could not allocate YUV frame buffer.");
        closeRecorder();
        return false;
    }

    // RGB24 → YUV420P conversion
    swsCtx = sws_getContext(width, height, AV_PIX_FMT_RGB24,
                            width, height, AV_PIX_FMT_YUV420P,
                            SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsCtx) {
        emit recordingError("Could not create colour-space conversion context.");
        closeRecorder();
        return false;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        emit recordingError("Could not allocate AVPacket.");
        closeRecorder();
        return false;
    }

    recFrameIndex = 0;
    recOpen       = true;
    emit recordingStarted();
    return true;
}

void VideoWorker::closeRecorder()
{
    if (swsCtx)   { sws_freeContext(swsCtx);        swsCtx      = nullptr; }
    if (yuvFrame) { av_frame_free(&yuvFrame);        yuvFrame    = nullptr; }
    if (pkt)      { av_packet_free(&pkt);            pkt         = nullptr; }
    if (codecCtx) { avcodec_free_context(&codecCtx); codecCtx    = nullptr; }
    if (fmtCtx)   {
        if (fmtCtx->pb) avio_closep(&fmtCtx->pb);
        avformat_free_context(fmtCtx);
        fmtCtx = nullptr;
    }
    videoStream   = nullptr;
    recOpen       = false;
    recFrameIndex = 0;
}

void VideoWorker::encodeAndWrite(AVFrame *frame)
{
    if (!codecCtx || !pkt) return;

    int ret = avcodec_send_frame(codecCtx, frame);
    if (ret < 0 && ret != AVERROR_EOF) return;

    while (true) {
        ret = avcodec_receive_packet(codecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        if (ret < 0)
            break;
        av_packet_rescale_ts(pkt, codecCtx->time_base, videoStream->time_base);
        pkt->stream_index = videoStream->index;
        av_interleaved_write_frame(fmtCtx, pkt);
        av_packet_unref(pkt);
    }
}

void VideoWorker::writeRecordingFrame(const QImage &image)
{
    if (!recOpen) {
        if (!openRecorder(image.width(), image.height())) {
            recording = false;
            return;
        }
    }

    // swscale requires contiguous RGB24
    QImage rgb = (image.format() == QImage::Format_RGB888)
                     ? image
                     : image.convertToFormat(QImage::Format_RGB888);

    if (av_frame_make_writable(yuvFrame) < 0)
        return;

    const uint8_t *srcData[1]   = { rgb.constBits() };
    int            srcStride[1] = { static_cast<int>(rgb.bytesPerLine()) };
    sws_scale(swsCtx, srcData, srcStride, 0, rgb.height(),
              yuvFrame->data, yuvFrame->linesize);

    yuvFrame->pts = recFrameIndex++;
    encodeAndWrite(yuvFrame);
}

#endif // HAVE_FFMPEG
