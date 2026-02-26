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

