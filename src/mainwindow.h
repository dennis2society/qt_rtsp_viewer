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
    void onUrlChanged(const QString &url);
    void onUrlSelected();
    void onPlayerError(const QString &errorMessage);
    void onPlayerStatusChanged(const QString &status);
    void onRemoveUrlClicked();

private:
    void setupUI();
    void connectSignals();
    void loadUrlHistory();
    void saveUrlHistory();
    void addUrlToHistory(const QString &url);
    void autoplayLastStream();
    void loadSavedPassword();
    void saveCredentialsForUrl(const QString &url, const QString &username, const QString &password);
    void loadCredentialsForUrl(const QString &url);
    void removeCredentialsForUrl(const QString &url);

    QLabel *titleLabel;
    QComboBox *urlInput;
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QCheckBox *savePasswordCheckbox;
    QPushButton *playButton;
    QPushButton *stopButton;
    QPushButton *removeButton;
    VideoPlayer *videoPlayer;
    VideoEffects *videoEffects;
    EffectsSidebar *effectsSidebar;
    QSettings *settings;
};

#endif // MAINWINDOW_H
