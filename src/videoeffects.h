#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <QObject>
#include <QImage>

class VideoEffects : public QObject
{
    Q_OBJECT

public:
    VideoEffects(QObject *parent = nullptr);
    ~VideoEffects();

    // Effect parameters
    void setBlurAmount(int amount);
    void setGrayscaleEnabled(bool enabled);
    void setSepiaEnabled(bool enabled);
    void setBrightnessAmount(int amount);
    void setContrastAmount(int amount);

    // Apply effects to an image
    QImage applyEffects(const QImage &sourceImage);

    // Get current parameters
    int getBlurAmount() const { return blurAmount; }
    bool isGrayscaleEnabled() const { return grayscaleEnabled; }
    bool isSepiaEnabled() const { return sepiaEnabled; }
    int getBrightnessAmount() const { return brightnessAmount; }
    int getContrastAmount() const { return contrastAmount; }

private:
    QImage applyBlur(const QImage &image, int amount);
    QImage applyGrayscale(const QImage &image);
    QImage applySepia(const QImage &image);
    QImage applyBrightnessContrast(const QImage &image, int brightness, int contrast);

    int blurAmount;
    bool grayscaleEnabled;
    bool sepiaEnabled;
    int brightnessAmount;
    int contrastAmount;
};

#endif // VIDEOEFFECTS_H
