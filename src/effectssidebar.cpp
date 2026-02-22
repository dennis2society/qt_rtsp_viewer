#include "effectssidebar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>

EffectsSidebar::EffectsSidebar(VideoEffects *effects, QWidget *parent)
    : QWidget(parent), effects(effects)
{
    setMaximumWidth(250);
    setMinimumWidth(200);
    setupUI();
}

EffectsSidebar::~EffectsSidebar()
{
}

void EffectsSidebar::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // Title
    QLabel *titleLabel = new QLabel("Video Effects", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Blur control
    QGroupBox *blurGroup = new QGroupBox("Blur", this);
    QVBoxLayout *blurLayout = new QVBoxLayout();
    blurSlider = new QSlider(Qt::Horizontal, this);
    blurSlider->setMinimum(0);
    blurSlider->setMaximum(20);
    blurSlider->setValue(0);
    blurValueLabel = new QLabel("0", this);
    blurValueLabel->setMaximumWidth(30);
    QHBoxLayout *blurHLayout = new QHBoxLayout();
    blurHLayout->addWidget(blurSlider);
    blurHLayout->addWidget(blurValueLabel);
    blurLayout->addLayout(blurHLayout);
    blurGroup->setLayout(blurLayout);
    mainLayout->addWidget(blurGroup);
    connect(blurSlider, QOverload<int>::of(&QSlider::valueChanged), this, &EffectsSidebar::onBlurChanged);
    
    // Grayscale control
    QGroupBox *grayscaleGroup = new QGroupBox("Color", this);
    QVBoxLayout *grayscaleLayout = new QVBoxLayout();
    grayscaleCheckBox = new QCheckBox("Grayscale", this);
    grayscaleLayout->addWidget(grayscaleCheckBox);
    sepiaCheckBox = new QCheckBox("Sepia", this);
    grayscaleLayout->addWidget(sepiaCheckBox);
    grayscaleGroup->setLayout(grayscaleLayout);
    mainLayout->addWidget(grayscaleGroup);
    connect(grayscaleCheckBox, &QCheckBox::toggled, this, &EffectsSidebar::onGrayscaleToggled);
    connect(sepiaCheckBox, &QCheckBox::toggled, this, &EffectsSidebar::onSepiaToggled);
    
    // Brightness control
    QGroupBox *brightnessGroup = new QGroupBox("Brightness", this);
    QVBoxLayout *brightnessLayout = new QVBoxLayout();
    brightnessSlider = new QSlider(Qt::Horizontal, this);
    brightnessSlider->setMinimum(-100);
    brightnessSlider->setMaximum(100);
    brightnessSlider->setValue(0);
    brightnessValueLabel = new QLabel("0", this);
    brightnessValueLabel->setMaximumWidth(40);
    QHBoxLayout *brightnessHLayout = new QHBoxLayout();
    brightnessHLayout->addWidget(brightnessSlider);
    brightnessHLayout->addWidget(brightnessValueLabel);
    brightnessLayout->addLayout(brightnessHLayout);
    brightnessGroup->setLayout(brightnessLayout);
    mainLayout->addWidget(brightnessGroup);
    connect(brightnessSlider, QOverload<int>::of(&QSlider::valueChanged), this, &EffectsSidebar::onBrightnessChanged);
    
    // Contrast control
    QGroupBox *contrastGroup = new QGroupBox("Contrast", this);
    QVBoxLayout *contrastLayout = new QVBoxLayout();
    contrastSlider = new QSlider(Qt::Horizontal, this);
    contrastSlider->setMinimum(-100);
    contrastSlider->setMaximum(100);
    contrastSlider->setValue(0);
    contrastValueLabel = new QLabel("0", this);
    contrastValueLabel->setMaximumWidth(40);
    QHBoxLayout *contrastHLayout = new QHBoxLayout();
    contrastHLayout->addWidget(contrastSlider);
    contrastHLayout->addWidget(contrastValueLabel);
    contrastLayout->addLayout(contrastHLayout);
    contrastGroup->setLayout(contrastLayout);
    mainLayout->addWidget(contrastGroup);
    connect(contrastSlider, QOverload<int>::of(&QSlider::valueChanged), this, &EffectsSidebar::onContrastChanged);
    
    // Reset button
    QPushButton *resetButton = new QPushButton("Reset All", this);
    mainLayout->addWidget(resetButton);
    connect(resetButton, &QPushButton::clicked, this, &EffectsSidebar::onResetEffects);
    
    mainLayout->addStretch();
    setLayout(mainLayout);
}

void EffectsSidebar::onBlurChanged(int value)
{
    effects->setBlurAmount(value);
    blurValueLabel->setText(QString::number(value));
    emit effectsChanged();
}

void EffectsSidebar::onGrayscaleToggled(bool checked)
{
    effects->setGrayscaleEnabled(checked);
    sepiaCheckBox->setEnabled(!checked);
    emit effectsChanged();
}

void EffectsSidebar::onSepiaToggled(bool checked)
{
    effects->setSepiaEnabled(checked);
    grayscaleCheckBox->setEnabled(!checked);
    emit effectsChanged();
}

void EffectsSidebar::onBrightnessChanged(int value)
{
    effects->setBrightnessAmount(value);
    brightnessValueLabel->setText(QString::number(value));
    emit effectsChanged();
}

void EffectsSidebar::onContrastChanged(int value)
{
    effects->setContrastAmount(value);
    contrastValueLabel->setText(QString::number(value));
    emit effectsChanged();
}

void EffectsSidebar::onResetEffects()
{
    blurSlider->setValue(0);
    grayscaleCheckBox->setChecked(false);
    sepiaCheckBox->setChecked(false);
    brightnessSlider->setValue(0);
    contrastSlider->setValue(0);
}
