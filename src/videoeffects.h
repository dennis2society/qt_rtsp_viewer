#ifndef VIDEOEFFECTS_H
#define VIDEOEFFECTS_H

#include <QObject>
#include <QImage>

class VideoEffects : public QObject{
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

    // Motion detection parameters
    void setMotionDetectionEnabled(bool enabled);
    void setMotionSensitivity(int sensitivity);

    // Apply effects to an image
    QImage applyEffects(const QImage &sourceImage);
    QImage applyBlur(const QImage &image, int kernelSize = 10, double sigma = 1.5);
    QImage applyBrightnessContrastToImage(const QImage &image);

    // Get current parameters
    int getBlurAmount() const { return blurAmount; }
    bool isGrayscaleEnabled() const { return grayscaleEnabled; }
    int getBrightnessAmount() const { return brightnessAmount; }
    int getContrastAmount() const { return contrastAmount; }

    // Motion detection getters
    bool isMotionDetectionEnabled() const { return motionDetectionEnabled; }
    int getMotionSensitivity() const { return motionSensitivity; }

    // Motion vectors
    void setMotionVectorsEnabled(bool enabled) { motionVectorsEnabled = enabled; }
    bool isMotionVectorsEnabled() const { return motionVectorsEnabled; }

    // Face detection
    void setFaceDetectionEnabled(bool enabled) { faceDetectionEnabled = enabled; }
    bool isFaceDetectionEnabled() const { return faceDetectionEnabled; }

private:
    int blurAmount;
    bool grayscaleEnabled;
    int brightnessAmount;
    int contrastAmount;
    bool motionDetectionEnabled = false;
    int motionSensitivity = 50;
    bool motionVectorsEnabled = false;
    bool faceDetectionEnabled = false;
    
    QImage applyBrightnessContrast(const QImage &image, int brightness, int contrast);
};

#endif // VIDEOEFFECTS_H
