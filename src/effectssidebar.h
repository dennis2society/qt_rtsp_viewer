#ifndef EFFECTSSIDEBAR_H
#define EFFECTSSIDEBAR_H

#include "videoeffects.h"
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
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
    
    void setAutoRecordDir(const QString &dir);

signals:
    void effectsChanged();
    void overlayToggled(bool enabled);
    void autoRecordToggled(bool enabled);
    void autoRecordDirChanged(const QString &dir);
    void autoRecordTimeoutChanged(int seconds);

private slots:
    void onBlurChanged(int value);
    void onGrayscaleToggled(bool checked);
    void onBrightnessChanged(int value);
    void onColorTemperatureChanged(int value);
    void onContrastChanged(int value);
    void onResetEffects();
    void onMotionDetectionChanged();
    void onMotionVectorsChanged();
    void onMotionGraphChanged();
    void onMotionGraphSensitivityChanged(int value);
    void onFaceDetectionChanged();
    void onOverlayToggled(bool checked);
    void onAutoRecordToggled(bool checked);
    void onAutoRecordDirClicked();
    void onAutoRecordTimeoutChanged(int value);

private:
    void setupUI();

    VideoEffects *effects;

    QSlider *blurSlider;
    QLabel *blurValueLabel;
    QCheckBox *grayscaleCheckBox;
    QSlider *brightnessSlider;
    QLabel *brightnessValueLabel;
    QSlider *colorTemperatureSlider;
    QLabel *colorTemperatureValueLabel;
    QSlider *contrastSlider;
    QLabel *contrastValueLabel;

    // Motion detection UI controls
    QCheckBox *motionDetectionCheckBox;
    QSlider *motionSensitivitySlider;
    QLabel *motionSensitivityLabel;

    // Motion vectors UI control
    QCheckBox *motionVectorsCheckBox;

    // Motion graph UI controls
    QCheckBox *motionGraphCheckBox;
    QSlider   *motionGraphSensitivitySlider;
    QLabel    *motionGraphSensitivityLabel;

    // Face detection UI control
    QCheckBox *faceDetectionCheckBox;

    // Auto-record on motion UI controls
    QCheckBox   *autoRecordCheckBox;
    QPushButton *autoRecordDirButton;
    QLabel      *autoRecordDirLabel;
    QSlider     *autoRecordTimeoutSlider;
    QLabel      *autoRecordTimeoutLabel;

    // Overlay UI control
    QCheckBox *overlayCheckBox;
};

#endif // EFFECTSSIDEBAR_H
