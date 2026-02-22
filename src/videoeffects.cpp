#include "videoeffects.h"
#include <QImage>
#include <algorithm>

VideoEffects::VideoEffects(QObject *parent)
    : QObject(parent)
    , blurAmount(0)
    , grayscaleEnabled(false)
    , sepiaEnabled(false)
    , brightnessAmount(0)
    , contrastAmount(0)
{
}

VideoEffects::~VideoEffects()
{
}

void VideoEffects::setBlurAmount(int amount)
{
    blurAmount = qBound(0, amount, 20);
}

void VideoEffects::setGrayscaleEnabled(bool enabled)
{
    grayscaleEnabled = enabled;
}

void VideoEffects::setSepiaEnabled(bool enabled)
{
    sepiaEnabled = enabled;
}

void VideoEffects::setBrightnessAmount(int amount)
{
    brightnessAmount = qBound(-100, amount, 100);
}

void VideoEffects::setContrastAmount(int amount)
{
    contrastAmount = qBound(-100, amount, 100);
}

QImage VideoEffects::applyEffects(const QImage &sourceImage)
{
    // If no effects are enabled, return the original image
    if (blurAmount == 0 && !grayscaleEnabled && !sepiaEnabled && brightnessAmount == 0 && contrastAmount == 0) {
        return sourceImage;
    }

    QImage result = sourceImage;

    // Apply brightness and contrast
    if (brightnessAmount != 0 || contrastAmount != 0) {
        result = applyBrightnessContrast(result, brightnessAmount, contrastAmount);
    }

    // Apply blur
    if (blurAmount > 0) {
        result = applyBlur(result, blurAmount);
    }

    // Apply grayscale
    if (grayscaleEnabled) {
        result = applyGrayscale(result);
    }

    // Apply sepia (after grayscale if both enabled)
    if (sepiaEnabled && !grayscaleEnabled) {
        result = applySepia(result);
    }

    return result;
}

QImage VideoEffects::applyBlur(const QImage &image, int amount)
{
    if (amount <= 0) {
        return image;
    }

    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();

    // Simple box blur
    int radius = amount;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int r = 0, g = 0, b = 0, count = 0;

            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        QColor color(result.pixel(nx, ny));
                        r += color.red();
                        g += color.green();
                        b += color.blue();
                        count++;
                    }
                }
            }

            if (count > 0) {
                result.setPixel(x, y, qRgb(r / count, g / count, b / count));
            }
        }
    }

    return result;
}

QImage VideoEffects::applyGrayscale(const QImage &image)
{
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color(result.pixel(x, y));
            int gray = (color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114);
            result.setPixel(x, y, qRgb(gray, gray, gray));
        }
    }

    return result;
}

QImage VideoEffects::applySepia(const QImage &image)
{
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color(result.pixel(x, y));
            int gray = (color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114);

            int r = qBound(0, gray + 30, 255);
            int g = qBound(0, gray + 10, 255);
            int b = qBound(0, gray - 20, 255);

            result.setPixel(x, y, qRgb(r, g, b));
        }
    }

    return result;
}

QImage VideoEffects::applyBrightnessContrast(const QImage &image, int brightness, int contrast)
{
    QImage result = image.convertToFormat(QImage::Format_RGB32);
    int width = result.width();
    int height = result.height();

    float contrastFactor = (259.0f * (contrast + 255.0f)) / (255.0f * (259.0f - contrast));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            QColor color(result.pixel(x, y));

            int r = qBound(0, (int)(contrastFactor * (color.red() - 128) + 128 + brightness), 255);
            int g = qBound(0, (int)(contrastFactor * (color.green() - 128) + 128 + brightness), 255);
            int b = qBound(0, (int)(contrastFactor * (color.blue() - 128) + 128 + brightness), 255);

            result.setPixel(x, y, qRgb(r, g, b));
        }
    }

    return result;
}
