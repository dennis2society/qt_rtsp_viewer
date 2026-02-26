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

signals:
    void effectsChanged();

private slots:
    void onBlurChanged(int value);
    void onGrayscaleToggled(bool checked);
    void onBrightnessChanged(int value);
    void onContrastChanged(int value);
    void onResetEffects();

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
};

#endif // EFFECTSSIDEBAR_H
