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
    void setColorTemperature(int temp); // -100 (warm/reddish) to 100 (cool/bluish)

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
    int getColorTemperature() const { return colorTemperature; }

    // Motion detection getters
    bool isMotionDetectionEnabled() const { return motionDetectionEnabled; }
    int getMotionSensitivity() const { return motionSensitivity; }

    // Motion vectors
    void setMotionVectorsEnabled(bool enabled) { motionVectorsEnabled = enabled; }
    bool isMotionVectorsEnabled() const { return motionVectorsEnabled; }

    // Motion graph overlay
    void setMotionGraphEnabled(bool enabled) { motionGraphEnabled = enabled; }
    bool isMotionGraphEnabled() const { return motionGraphEnabled; }
    void setMotionGraphSensitivity(int s) { motionGraphSensitivity = qBound(1, s, 100); }
    int  getMotionGraphSensitivity() const { return motionGraphSensitivity; }

    // Face detection
    void setFaceDetectionEnabled(bool enabled) { faceDetectionEnabled = enabled; }
    bool isFaceDetectionEnabled() const { return faceDetectionEnabled; }

private:
    int blurAmount;
    bool grayscaleEnabled;
    int brightnessAmount;
    int contrastAmount;
    int colorTemperature = 0; // -100 (warm) to 100 (cool)
    bool motionDetectionEnabled = false;
    int motionSensitivity = 50;
    bool motionVectorsEnabled = false;
    bool motionGraphEnabled = false;
    int  motionGraphSensitivity = 15; // mean abs-diff that saturates the chart
    bool faceDetectionEnabled = false;
    
    QImage applyBrightnessContrast(const QImage &image, int brightness, int contrast);
    QImage applyColorTemperature(const QImage &image, int temperature);
};

#endif // VIDEOEFFECTS_H
