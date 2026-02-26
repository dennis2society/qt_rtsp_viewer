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

    const static int MAX_BLUR_AMOUNT = 50;

    // Effect parameters
    void setBlurAmount(int amount);
    void setGrayscaleEnabled(bool enabled);
    void setBrightnessAmount(int amount);
    void setContrastAmount(int amount);

    // Apply effects to an image
    QImage applyEffects(const QImage &sourceImage);
    QImage applyBlur(const QImage &image, int kernelSize = 10, double sigma = 1.5);

    // Get current parameters
    int getBlurAmount() const { return blurAmount; }
    bool isGrayscaleEnabled() const { return grayscaleEnabled; }
    int getBrightnessAmount() const { return brightnessAmount; }
    int getContrastAmount() const { return contrastAmount; }

private:
    int blurAmount;
    bool grayscaleEnabled;
    int brightnessAmount;
    int contrastAmount;
};

#endif // VIDEOEFFECTS_H
