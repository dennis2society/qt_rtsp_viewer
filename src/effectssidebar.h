#ifndef EFFECTSSIDEBAR_H
#define EFFECTSSIDEBAR_H

#include <QWidget>
#include <QSlider>
#include <QCheckBox>
#include <QLabel>
#include "videoeffects.h"

class EffectsSidebar : public QWidget
{
    Q_OBJECT

public:
    EffectsSidebar(VideoEffects *effects, QWidget *parent = nullptr);
    ~EffectsSidebar();

    VideoEffects *getEffects() const { return effects; }

signals:
    void effectsChanged();

private slots:
    void onBlurChanged(int value);
    void onGrayscaleToggled(bool checked);
    void onSepiaToggled(bool checked);
    void onBrightnessChanged(int value);
    void onContrastChanged(int value);
    void onResetEffects();

private:
    void setupUI();

    VideoEffects *effects;
    
    QSlider *blurSlider;
    QLabel *blurValueLabel;
    QCheckBox *grayscaleCheckBox;
    QCheckBox *sepiaCheckBox;
    QSlider *brightnessSlider;
    QLabel *brightnessValueLabel;
    QSlider *contrastSlider;
    QLabel *contrastValueLabel;
};

#endif // EFFECTSSIDEBAR_H
