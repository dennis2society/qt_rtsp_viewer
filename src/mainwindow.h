#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "effectssidebar.h"
#include "videoeffects.h"
#include "videoplayer.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onPauseButtonToggled(bool checked);
    void onUrlChanged(const QString &url);
    // void onUrlSelected(); // removed, not implemented
    void onPlayerError(const QString &errorMessage);
    void onPlayerStatusChanged(const QString &status);
    void onRemoveUrlClicked();
    void onRecordButtonToggled(bool checked);
    void onRecordingFinished(const QString &path);
    void onRecordingError(const QString &message);

private:
    void setupUI();
    void connectSignals();
    void loadUrlHistory();
    void saveUrlHistory();
    void addUrlToHistory(const QString &url);
    void autoplayLastStream();

    QLabel *titleLabel;
    QComboBox *urlInput;
    QPushButton *playButton;
    QPushButton *stopButton;
    QPushButton *pauseButton;
    QPushButton *removeButton;
    QPushButton *recordButton;
    VideoPlayer *videoPlayer;
    VideoEffects *videoEffects;
    EffectsSidebar *effectsSidebar;
    QSettings *settings;
    QString autoRecordDir;   // mirrors the sidebar auto-record output directory
};

#endif // MAINWINDOW_H
