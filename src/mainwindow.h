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
#include <QMap>

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
    void onCameraNameEdited(const QString &newName);

private:
    void setupUI();
    void connectSignals();
    void loadUrlHistory();
    void saveUrlHistory();
    void addUrlToHistory(const QString &url);
    void autoplayLastStream();
    QString generateCameraName(int index) const;
    QString getCameraNameForUrl(const QString &url) const;
    void setCameraNameForUrl(const QString &url, const QString &cameraName);
    void updateCurrentCameraName();

    QLabel *titleLabel;
    QComboBox *urlInput;
    QLineEdit *cameraNameEdit;
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
    QMap<QString, QString> cameraNames;  // maps URL to camera name
    QString currentCameraName;  // current camera name for active URL
};

#endif // MAINWINDOW_H
