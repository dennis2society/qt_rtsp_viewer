#include "mainwindow.h"
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

// ---------------------------------------------------------------------------
// RecordDialog – lets the user pick output file, codec preset and frame rate
// ---------------------------------------------------------------------------
class RecordDialog : public QDialog
{
public:
    struct Preset { QString label; QString ext; QString codec; };

    explicit RecordDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Start Recording");
        setMinimumWidth(500);

        auto *layout = new QVBoxLayout(this);
        auto *form   = new QFormLayout();

        // Format / codec preset
        formatCombo = new QComboBox(this);
        for (const auto &p : presets())
            formatCombo->addItem(p.label);
        form->addRow("Format:", formatCombo);

        // Output file path
        auto *pathRow = new QHBoxLayout();
        pathEdit      = new QLineEdit(this);
        pathEdit->setPlaceholderText("Select output file…");
        auto *browseBtn = new QPushButton("Browse…", this);
        pathRow->addWidget(pathEdit);
        pathRow->addWidget(browseBtn);
        form->addRow("Output file:", pathRow);

        // Frame rate
        fpsSpinBox = new QDoubleSpinBox(this);
        fpsSpinBox->setRange(1.0, 120.0);
        fpsSpinBox->setValue(25.0);
        fpsSpinBox->setDecimals(2);
        fpsSpinBox->setSuffix(" fps");
        form->addRow("Frame rate:", fpsSpinBox);

        layout->addLayout(form);

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);

        // Update file extension when preset changes
        connect(formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) { syncExtension(idx); });

        connect(browseBtn, &QPushButton::clicked, this, [this]() {
            const auto &ps = presets();
            int idx  = formatCombo->currentIndex();
            QString ext = (idx >= 0 && idx < ps.size()) ? ps[idx].ext : "mp4";
            QString path = QFileDialog::getSaveFileName(
                this, "Save Recording",
                QDir::homePath() + "/recording." + ext,
                QString("Video (*.%1);;All files (*)").arg(ext));
            if (!path.isEmpty())
                pathEdit->setText(path);
        });

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (pathEdit->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "No file selected",
                                     "Please choose an output file.");
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QString filePath() const { return pathEdit->text().trimmed(); }
    QString codec()    const {
        int idx = formatCombo->currentIndex();
        const auto &ps = presets();
        return (idx >= 0 && idx < ps.size()) ? ps[idx].codec : "libx264";
    }
    double  fps()      const { return fpsSpinBox->value(); }

private:
    void syncExtension(int idx)
    {
        const auto &ps = presets();
        if (idx < 0 || idx >= ps.size()) return;
        QString cur = pathEdit->text().trimmed();
        if (cur.isEmpty()) return;
        QFileInfo fi(cur);
        pathEdit->setText(fi.absolutePath() + "/" +
                          fi.completeBaseName() + "." + ps[idx].ext);
    }

    static const QVector<Preset> &presets()
    {
        static const QVector<Preset> ps = {
            { "MP4  –  H.264  (libx264)",       "mp4", "libx264" },
            { "MP4  –  H.265/HEVC  (libx265)",  "mp4", "libx265" },
            { "MKV  –  H.264  (libx264)",       "mkv", "libx264" },
            { "MKV  –  H.265/HEVC  (libx265)",  "mkv", "libx265" },
            { "AVI  –  H.264  (libx264)",       "avi", "libx264" },
        };
        return ps;
    }

    QComboBox      *formatCombo;
    QLineEdit      *pathEdit;
    QDoubleSpinBox *fpsSpinBox;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , videoPlayer(nullptr)
    , settings(nullptr)
{
    setWindowTitle("Qt RTSP Viewer");
    setGeometry(100, 100, 1200, 900);

    // Initialize settings
    settings = new QSettings("QtRtspViewer", "QtRtspViewer", this);

    setupUI();
    loadUrlHistory();
    // loadSavedPassword() removed
    
    // Load overlay and auto-record settings (now stored in Effects group by EffectsSidebar)
    // Create a temporary QSettings to check for old settings at root level for migration
    bool overlayEnabled = settings->value("OverlayEnabled", true).toBool();
    videoPlayer->setOverlayEnabled(overlayEnabled);
    
    QString autoRecDir = settings->value("AutoRecordDir", "").toString();
    if (!autoRecDir.isEmpty()) {
        effectsSidebar->setAutoRecordDir(autoRecDir);
        autoRecordDir = autoRecDir;
    } else {
        // Try to load from Effects group if it exists
        settings->beginGroup("Effects");
        autoRecDir = settings->value("AutoRecordDir", "").toString();
        settings->endGroup();
        if (!autoRecDir.isEmpty()) {
            effectsSidebar->setAutoRecordDir(autoRecDir);
            autoRecordDir = autoRecDir;
        }
    }
    
    connectSignals();
    updateCurrentCameraName();
    autoplayLastStream();
}

MainWindow::~MainWindow()
{
    if (effectsSidebar) {
        effectsSidebar->saveEffectsSettings();
    }
}

void MainWindow::setupUI()
{
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Title
    titleLabel = new QLabel("RTSP Stream Viewer", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // URL input layout
    QHBoxLayout *inputLayout = new QHBoxLayout();
    QLabel *urlLabel = new QLabel("RTSP URL:", this);
    urlInput = new QComboBox(this);
    urlInput->setEditable(true);
    urlInput->setInsertPolicy(QComboBox::NoInsert);
    urlInput->lineEdit()->setPlaceholderText("Enter RTSP URL (e.g., rtsp://example.com/stream)");
    urlInput->lineEdit()->setMinimumWidth(300);
    urlInput->setToolTip("Enter the RTSP stream URL. Select from history or enter a new URL.");
    removeButton = new QPushButton("Remove", this);
    removeButton->setMaximumWidth(80);
    removeButton->setToolTip("Remove the selected URL from history.");
    cameraNameEdit = new QLineEdit(this);
    cameraNameEdit->setPlaceholderText("Camera name");
    cameraNameEdit->setMaximumWidth(120);
    cameraNameEdit->setToolTip("Edit the camera name for the current URL.");
    playButton = new QPushButton(this);
    playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    playButton->setToolTip("Play");
    stopButton = new QPushButton(this);
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton->setToolTip("Stop");
    pauseButton = new QPushButton(this);
    pauseButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    pauseButton->setToolTip("Pause");
    pauseButton->setCheckable(true);
    stopButton->setEnabled(false);
    pauseButton->setEnabled(false);
    recordButton = new QPushButton("\u23fa Record", this);
    recordButton->setCheckable(true);
    recordButton->setEnabled(false);
    recordButton->setToolTip("Record the stream with all effects and overlays applied");
    inputLayout->addWidget(urlLabel);
    inputLayout->addWidget(urlInput);
    inputLayout->addWidget(removeButton);
    inputLayout->addWidget(cameraNameEdit);
    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    inputLayout->addWidget(separator);
    inputLayout->addWidget(playButton);
    inputLayout->addWidget(pauseButton);
    inputLayout->addWidget(stopButton);
    QFrame *recSep = new QFrame(this);
    recSep->setFrameShape(QFrame::VLine);
    recSep->setFrameShadow(QFrame::Sunken);
    inputLayout->addWidget(recSep);
    inputLayout->addWidget(recordButton);

    // Username and password fields removed

    mainLayout->addLayout(inputLayout);

    // Video display with sidebar
    QHBoxLayout *videoLayout = new QHBoxLayout();

    // Video display area
    videoPlayer = new VideoPlayer(this);
    videoLayout->addWidget(videoPlayer, 1);

    // Initialize video effects and effects sidebar
    videoEffects = new VideoEffects(this);
    effectsSidebar = new EffectsSidebar(videoEffects, settings, this);
    
    // Wrap sidebar in a QScrollArea for scrolling if it exceeds available height
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(effectsSidebar);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    videoLayout->addWidget(scrollArea);

    mainLayout->addLayout(videoLayout, 1);

    centralWidget->setLayout(mainLayout);
    videoPlayer->setVideoEffects(videoEffects);
}

void MainWindow::connectSignals()
{
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(pauseButton, &QPushButton::toggled, this, &MainWindow::onPauseButtonToggled);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveUrlClicked);
    connect(cameraNameEdit, &QLineEdit::textChanged, this, &MainWindow::onCameraNameEdited);
    connect(videoPlayer, &VideoPlayer::errorOccurred, this, &MainWindow::onPlayerError);
    connect(videoPlayer, &VideoPlayer::statusChanged, this, &MainWindow::onPlayerStatusChanged);
    connect(urlInput->lineEdit(), &QLineEdit::textChanged, this, &MainWindow::onUrlChanged);
    connect(effectsSidebar, &EffectsSidebar::overlayToggled, videoPlayer, &VideoPlayer::setOverlayEnabled);
    connect(effectsSidebar, &EffectsSidebar::overlayToggled, this, [this](bool enabled) {
        effectsSidebar->saveEffectsSettings();
    });
    connect(recordButton,  &QPushButton::toggled,
            this,          &MainWindow::onRecordButtonToggled);
    connect(videoPlayer,   &VideoPlayer::recordingFinished,
            this,          &MainWindow::onRecordingFinished);
    connect(videoPlayer,   &VideoPlayer::recordingError,
            this,          &MainWindow::onRecordingError);
    // Auto-record on motion: wire sidebar ↔ player
    connect(effectsSidebar, &EffectsSidebar::autoRecordToggled,
            videoPlayer,    &VideoPlayer::setAutoRecordEnabled);
    connect(effectsSidebar, &EffectsSidebar::autoRecordDirChanged,
            videoPlayer,    &VideoPlayer::setAutoRecordDir);
    connect(effectsSidebar, &EffectsSidebar::autoRecordDirChanged, this, [this](const QString &dir) {
        autoRecordDir = dir;
        effectsSidebar->saveEffectsSettings();
    });
    connect(effectsSidebar, &EffectsSidebar::autoRecordTimeoutChanged,
            videoPlayer,    &VideoPlayer::setAutoRecordTimeout);
    connect(videoPlayer, &VideoPlayer::autoRecordingStarted, this, [this]() {
        // Show record button as active while an auto-recording file is being written
        recordButton->blockSignals(true);
        recordButton->setChecked(true);
        recordButton->blockSignals(false);
        recordButton->setText("\u23fa Auto-Rec");
        recordButton->setStyleSheet("color: red; font-weight: bold;");
        statusBar()->showMessage("\u23fa Auto-recording started (motion detected)", 3000);
    });
    connect(videoPlayer, &VideoPlayer::autoRecordingStopped, this, [this](const QString &path) {
        statusBar()->showMessage("\u23f9 Auto-recording saved: " + path, 5000);
    });
    // Auto-stop recording when the stream stops for any reason
    connect(videoPlayer, &VideoPlayer::streamStopped, this, [this]() {
        if (recordButton->isChecked())
            videoPlayer->stopRecording();
        recordButton->setEnabled(false);
    });
}

void MainWindow::onPlayButtonClicked()
{
    QString url = urlInput->currentText();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter an RTSP URL");
        return;
    }

    // Add URL to history
    addUrlToHistory(url);

    // Save as last played URL
    settings->setValue("LastPlayedUrl", url);

    // Start streaming
    videoPlayer->playStream(url);

    playButton->setEnabled(false);
    stopButton->setEnabled(true);
    pauseButton->setEnabled(true);
    pauseButton->setChecked(false);
    recordButton->setEnabled(true);
    urlInput->setEnabled(false);
}

void MainWindow::onStopButtonClicked()
{
    // If recording, stop it first (worker will emit recordingFinished)
    videoPlayer->stopRecording();

    videoPlayer->stopStream();

    playButton->setEnabled(true);
    stopButton->setEnabled(false);
    pauseButton->setEnabled(false);
    pauseButton->setChecked(false);
    urlInput->setEnabled(true);
}

void MainWindow::onPauseButtonToggled(bool checked)
{
    if (checked) {
        videoPlayer->pauseVideo();
    } else {
        videoPlayer->resumeVideo();
    }
}

void MainWindow::onPlayerError(const QString &errorMessage)
{
    QMessageBox::critical(this, "Playback Error", errorMessage);
    playButton->setEnabled(true);
    stopButton->setEnabled(false);
    pauseButton->setEnabled(false);
    pauseButton->setChecked(false);
    recordButton->setEnabled(false);
    urlInput->setEnabled(true);
}

void MainWindow::onPlayerStatusChanged(const QString &status)
{
    titleLabel->setText(status);

    if (status.contains("ended")) {
        playButton->setEnabled(true);
        stopButton->setEnabled(false);
        pauseButton->setEnabled(false);
        pauseButton->setChecked(false);
        recordButton->setEnabled(false);
        urlInput->setEnabled(true);
    }
}

void MainWindow::onUrlChanged(const QString &url)
{
    // This slot can be used to validate URLs or show additional info
    playButton->setEnabled(!url.isEmpty());
    updateCurrentCameraName();
}

void MainWindow::loadUrlHistory()
{
    settings->beginGroup("UrlHistory");
    int size = settings->beginReadArray("urls");
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        QString url = settings->value("url", "").toString();
        QString cameraName = settings->value("cameraName", "").toString();
        if (!url.isEmpty()) {
            urlInput->addItem(url);
            // If no camera name was saved, generate one
            if (cameraName.isEmpty()) {
                cameraName = generateCameraName(i);
            }
            cameraNames[url] = cameraName;
        }
    }
    settings->endArray();
    settings->endGroup();
}

void MainWindow::saveUrlHistory()
{
    settings->beginGroup("UrlHistory");
    settings->beginWriteArray("urls");
    for (int i = 0; i < urlInput->count(); ++i) {
        settings->setArrayIndex(i);
        QString url = urlInput->itemText(i);
        settings->setValue("url", url);
        settings->setValue("cameraName", cameraNames.value(url, generateCameraName(i)));
    }
    settings->endArray();
    settings->endGroup();
    settings->sync();
}

void MainWindow::addUrlToHistory(const QString &url)
{
    // Check if URL already exists
    int index = urlInput->findText(url);
    if (index >= 0) {
        // Move existing URL to top
        urlInput->removeItem(index);
    } else {
        // New URL – auto-generate a camera name if not already present
        if (!cameraNames.contains(url)) {
            cameraNames[url] = generateCameraName(0);
        }
    }

    // Add URL at the beginning
    urlInput->insertItem(0, url);
    urlInput->setCurrentIndex(0);

    // Keep only the last 20 URLs
    while (urlInput->count() > 20) {
        QString removedUrl = urlInput->itemText(urlInput->count() - 1);
        urlInput->removeItem(urlInput->count() - 1);
        cameraNames.remove(removedUrl);
    }

    // Save to persistent storage
    saveUrlHistory();
    effectsSidebar->saveEffectsSettings();
    updateCurrentCameraName();
}

void MainWindow::autoplayLastStream()
{
    QString lastUrl = settings->value("LastPlayedUrl", "").toString();
    if (!lastUrl.isEmpty()) {
        urlInput->lineEdit()->setText(lastUrl);
        QTimer::singleShot(500, this, &MainWindow::onPlayButtonClicked);
    }
}



void MainWindow::onRecordButtonToggled(bool checked)
{
    if (checked) {
        if (!autoRecordDir.isEmpty()) {
            // Use the same folder and filename pattern as auto-recording, with camera name
            const QString ts       = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
            const QString filename = ts + "_" + currentCameraName + "_recording.mp4";
            const QString path     = autoRecordDir + "/" + filename;
            recordButton->setText("\u23f9 Stop Rec");
            recordButton->setStyleSheet("color: red; font-weight: bold;");
            videoPlayer->startRecording(path, "libx265", 25.0);
        } else {
            RecordDialog dlg(this);
            if (dlg.exec() != QDialog::Accepted) {
                // User cancelled – revert button without re-triggering the slot
                recordButton->blockSignals(true);
                recordButton->setChecked(false);
                recordButton->blockSignals(false);
                return;
            }
            recordButton->setText("\u23f9 Stop Rec");
            recordButton->setStyleSheet("color: red; font-weight: bold;");
            videoPlayer->startRecording(dlg.filePath(), dlg.codec(), dlg.fps());
        }
    } else {
        // User clicked stop, don't start another recording
        videoPlayer->stopRecording();
    }
}

void MainWindow::onRecordingFinished(const QString &path)
{
    // Only update the button if it's currently checked (manual recording)
    // This prevents double-stopping the recorder
    if (recordButton->isChecked()) {
        recordButton->blockSignals(true);
        recordButton->setChecked(false);
        recordButton->blockSignals(false);
    }
    recordButton->setText("\u23fa Record");
    recordButton->setStyleSheet("");
    if (path.contains("_motion_recording")) {
        statusBar()->showMessage("\u23f9 Motion recording saved: " + path, 6000);
    } else {
        QMessageBox::information(this, "Recording Saved",
                                 "Recording saved to:\n" + path);
    }
}

void MainWindow::onRecordingError(const QString &message)
{
    recordButton->blockSignals(true);
    recordButton->setChecked(false);
    recordButton->blockSignals(false);
    recordButton->setText("\u23fa Record");
    recordButton->setStyleSheet("");
    QMessageBox::critical(this, "Recording Error", message);
}

void MainWindow::onRemoveUrlClicked()
{
    int currentIndex = urlInput->currentIndex();
    if (currentIndex < 0) {
        QMessageBox::warning(this, "Warning", "No URL selected to remove.");
        return;
    }

    QString urlToRemove = urlInput->itemText(currentIndex);

    // Show confirmation dialog
    QMessageBox::StandardButton reply =
        QMessageBox::warning(this, "Remove URL", "Are you sure you want to remove this URL?\n" + urlToRemove, QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        return;
    }

    urlInput->removeItem(currentIndex);

    // Save updated history
    saveUrlHistory();
    effectsSidebar->saveEffectsSettings();

    QMessageBox::information(this, "Removed", "URL removed from history.");
}

QString MainWindow::generateCameraName(int index) const
{
    return QString("cam_%1").arg(index + 1, 2, 10, QChar('0'));
}

QString MainWindow::getCameraNameForUrl(const QString &url) const
{
    return cameraNames.value(url, "cam_01");
}

void MainWindow::setCameraNameForUrl(const QString &url, const QString &cameraName)
{
    if (!cameraName.isEmpty()) {
        cameraNames[url] = cameraName;
        saveUrlHistory();
    }
}

void MainWindow::updateCurrentCameraName()
{
    QString currentUrl = urlInput->currentText();
    currentCameraName = getCameraNameForUrl(currentUrl);
    cameraNameEdit->blockSignals(true);
    cameraNameEdit->setText(currentCameraName);
    cameraNameEdit->blockSignals(false);
    if (videoPlayer) {
        videoPlayer->setCameraName(currentCameraName);
    }
}

void MainWindow::onCameraNameEdited(const QString &newName)
{
    QString currentUrl = urlInput->currentText();
    if (currentUrl.isEmpty() || newName.isEmpty()) {
        return;
    }

    setCameraNameForUrl(currentUrl, newName);
    currentCameraName = newName;
    if (videoPlayer) {
        videoPlayer->setCameraName(currentCameraName);
    }
}




