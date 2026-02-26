#ifndef EFFECTSSIDEBAR_H
#define EFFECTSSIDEBAR_H

#include "videoeffects.h"
#include <QCheckBox>
#include <QLabel>
#include <QSlider>
#include <QWidget>

class EffectsSidebar : public QWidget
{
    Q_OBJECT

public:
    EffectsSidebar(VideoEffects *effects, QWidget *parent = nullptr);
    ~EffectsSidebar();

    VideoEffects *getEffects() const
    {
        return effects;
    }
    const int getBlurValue();
    
    void setOverlayEnabled(bool enabled);
    bool isOverlayEnabled() const;

signals:
    void effectsChanged();
    void overlayToggled(bool enabled);

private slots:
    void onBlurChanged(int value);
    void onGrayscaleToggled(bool checked);
    void onBrightnessChanged(int value);
    void onContrastChanged(int value);
    void onResetEffects();
    void onMotionDetectionChanged();
    void onMotionVectorsChanged();
    void onOverlayToggled(bool checked);

private:
    void setupUI();

    VideoEffects *effects;

    QSlider *blurSlider;
    QLabel *blurValueLabel;
    QCheckBox *grayscaleCheckBox;
    QSlider *brightnessSlider;
    QLabel *brightnessValueLabel;
    QSlider *contrastSlider;
    QLabel *contrastValueLabel;

    // Motion detection UI controls
    QCheckBox *motionDetectionCheckBox;
    QSlider *motionSensitivitySlider;
    QLabel *motionSensitivityLabel;

    // Motion vectors UI control
    QCheckBox *motionVectorsCheckBox;

    // Overlay UI control
    QCheckBox *overlayCheckBox;
};

#endif // EFFECTSSIDEBAR_H
