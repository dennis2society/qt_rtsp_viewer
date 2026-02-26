#include "videoeffects.h"
#include <QImage>
#include <algorithm>

const int VideoEffects::MAX_BLUR_AMOUNT;

VideoEffects::VideoEffects(QObject *parent)
    : QObject(parent)
    , blurAmount(0)
    , grayscaleEnabled(false)
    , brightnessAmount(0)
    , contrastAmount(0)
{
}

VideoEffects::~VideoEffects()
{
}

void VideoEffects::setBlurAmount(int amount)
{
    blurAmount = qBound(0, amount, VideoEffects::MAX_BLUR_AMOUNT);
}

void VideoEffects::setGrayscaleEnabled(bool enabled)
{
    grayscaleEnabled = enabled;
}

void VideoEffects::setBrightnessAmount(int amount)
{
    brightnessAmount = qBound(-100, amount, 100);
}

void VideoEffects::setContrastAmount(int amount)
{
    contrastAmount = qBound(-100, amount, 100);
}

void VideoEffects::setMotionDetectionEnabled(bool enabled)
{
    motionDetectionEnabled = enabled;
}

void VideoEffects::setMotionSensitivity(int sensitivity)
{
    motionSensitivity = qBound(1, sensitivity, 100);
}

QImage VideoEffects::applyEffects(const QImage &sourceImage)
{
    QImage result = sourceImage;
    
    // Apply brightness and contrast
    if (brightnessAmount != 0 || contrastAmount != 0) {
        result = applyBrightnessContrast(result, brightnessAmount, contrastAmount);
    }
    
    // Apply grayscale
    if (grayscaleEnabled) {
        result = result.convertToFormat(QImage::Format_Grayscale8);
    }
    
    return result;
}

QImage VideoEffects::applyBlur(const QImage &image, int kernelSize, double sigma)
{
    // Placeholder for blur implementation
    // In practice, this would use OpenCV or similar library
    return image;
}

QImage VideoEffects::applyBrightnessContrastToImage(const QImage &image)
{
    return applyBrightnessContrast(image, brightnessAmount, contrastAmount);
}

QImage VideoEffects::applyBrightnessContrast(const QImage &image, int brightness, int contrast)
{
    if (image.isNull() || (brightness == 0 && contrast == 0)) {
        return image;
    }

    // Create lookup table for faster pixel processing
    uchar lut[256];
    double factor = (contrast + 100.0) / 100.0;
    
    for (int i = 0; i < 256; ++i) {
        int value = i + brightness;
        if (contrast != 0) {
            value = static_cast<int>(128 + (value - 128) * factor);
        }
        lut[i] = static_cast<uchar>(qBound(0, value, 255));
    }
    
    QImage result = image;
    
    // Only convert format if needed
    if (result.format() != QImage::Format_RGB32 && result.format() != QImage::Format_ARGB32) {
        result = result.convertToFormat(QImage::Format_ARGB32);
    }
    
    // Apply brightness and contrast using lookup table
    for (int y = 0; y < result.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            QRgb pixel = line[x];
            
            uchar r = lut[qRed(pixel)];
            uchar g = lut[qGreen(pixel)];
            uchar b = lut[qBlue(pixel)];
            uchar a = qAlpha(pixel);
            
            line[x] = qRgba(r, g, b, a);
        }
    }
    
    return result;
}

