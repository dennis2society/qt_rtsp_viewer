#include "mainwindow.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , videoPlayer(nullptr)
    , settings(nullptr)
{
    setWindowTitle("Qt RTSP Viewer");
    setGeometry(100, 100, 1000, 700);

    // Initialize settings
    settings = new QSettings("QtRtspViewer", "QtRtspViewer", this);

    setupUI();
    loadUrlHistory();
    loadSavedPassword();
    connectSignals();
    autoplayLastStream();
}

MainWindow::~MainWindow()
{
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
    urlInput->setToolTip("Enter the RTSP stream URL. Select from history or enter a new URL.");
    removeButton = new QPushButton("Remove", this);
    removeButton->setMaximumWidth(80);
    removeButton->setToolTip("Remove the selected URL from history.");
    inputLayout->addWidget(urlLabel);
    inputLayout->addWidget(urlInput);
    inputLayout->addWidget(removeButton);

    // Username layout
    QHBoxLayout *usernameLayout = new QHBoxLayout();
    usernameLayout->setContentsMargins(0, 0, 0, 0);
    usernameLayout->setSpacing(2);
    QLabel *usernameLabel = new QLabel("Username:", this);
    usernameLabel->setMaximumWidth(70);
    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Optional");
    usernameInput->setMaximumWidth(120);
    usernameInput->setToolTip("Enter the RTSP username (optional). Used for authenticated streams.");
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(usernameInput);
    inputLayout->addLayout(usernameLayout);

    // Password layout
    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(2);
    QLabel *passwordLabel = new QLabel("Password:", this);
    passwordLabel->setMaximumWidth(70);
    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Optional");
    passwordInput->setMaximumWidth(120);
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setToolTip("Enter the RTSP password (optional). Used for authenticated streams.");
    savePasswordCheckbox = new QCheckBox("Save PW", this);
    savePasswordCheckbox->setToolTip("Save password for next time.");
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordInput);
    passwordLayout->addWidget(savePasswordCheckbox);
    inputLayout->addLayout(passwordLayout);

    mainLayout->addLayout(inputLayout);

    // Video display with sidebar
    QHBoxLayout *videoLayout = new QHBoxLayout();

    // Video display area
    videoPlayer = new VideoPlayer(this);
    videoLayout->addWidget(videoPlayer, 1);

    // Initialize video effects and effects sidebar
    videoEffects = new VideoEffects(this);
    effectsSidebar = new EffectsSidebar(videoEffects, this);
    videoLayout->addWidget(effectsSidebar);

    mainLayout->addLayout(videoLayout, 1);

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    playButton = new QPushButton("Play", this);
    stopButton = new QPushButton("Stop", this);
    stopButton->setEnabled(false);
    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    centralWidget->setLayout(mainLayout);
    videoPlayer->setEffectsSidebar(effectsSidebar);
}

void MainWindow::connectSignals()
{
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveUrlClicked);
    connect(videoPlayer, &VideoPlayer::errorOccurred, this, &MainWindow::onPlayerError);
    connect(videoPlayer, &VideoPlayer::statusChanged, this, &MainWindow::onPlayerStatusChanged);
    connect(urlInput->lineEdit(), &QLineEdit::textChanged, this, &MainWindow::onUrlChanged);
}

void MainWindow::onPlayButtonClicked()
{
    QString url = urlInput->currentText();
    if (url.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter an RTSP URL");
        return;
    }

    // Inject credentials if provided
    QString username = usernameInput->text().trimmed();
    QString password = passwordInput->text().trimmed();

    if (!username.isEmpty()) {
        // Parse URL to inject credentials
        if (url.startsWith("rtsp://")) {
            QString baseUrl = url.mid(7); // Remove "rtsp://"
            url = QString("rtsp://%1:%2@%3").arg(username, password, baseUrl);
        }
    }

    // Add URL to history
    addUrlToHistory(url);

    // Save as last played URL
    settings->setValue("LastPlayedUrl", url);

    // Save password if checkbox is checked
    if (savePasswordCheckbox->isChecked()) {
        settings->setValue("SavePassword", true);
        settings->setValue("SavedPassword", password);
    } else {
        settings->setValue("SavePassword", false);
        settings->remove("SavedPassword");
    }

    // Start streaming
    videoPlayer->playStream(url);

    playButton->setEnabled(false);
    stopButton->setEnabled(true);
    urlInput->setEnabled(false);
}

void MainWindow::onStopButtonClicked()
{
    // Stop the media player
    videoPlayer->stopStream();

    playButton->setEnabled(true);
    stopButton->setEnabled(false);
    urlInput->setEnabled(true);
}

void MainWindow::onPlayerError(const QString &errorMessage)
{
    QMessageBox::critical(this, "Playback Error", errorMessage);
    playButton->setEnabled(true);
    stopButton->setEnabled(false);
    urlInput->setEnabled(true);
}

void MainWindow::onPlayerStatusChanged(const QString &status)
{
    titleLabel->setText(status);

    if (status.contains("ended")) {
        playButton->setEnabled(true);
        stopButton->setEnabled(false);
        urlInput->setEnabled(true);
    }
}

void MainWindow::onUrlChanged(const QString &url)
{
    // This slot can be used to validate URLs or show additional info
    playButton->setEnabled(!url.isEmpty());
}

void MainWindow::loadUrlHistory()
{
    settings->beginGroup("UrlHistory");
    int size = settings->beginReadArray("urls");
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        QString url = settings->value("url", "").toString();
        if (!url.isEmpty()) {
            urlInput->addItem(url);
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
        settings->setValue("url", urlInput->itemText(i));
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
    }

    // Add URL at the beginning
    urlInput->insertItem(0, url);
    urlInput->setCurrentIndex(0);

    // Keep only the last 20 URLs
    while (urlInput->count() > 20) {
        urlInput->removeItem(urlInput->count() - 1);
    }

    // Save to persistent storage
    saveUrlHistory();
}

void MainWindow::autoplayLastStream()
{
    QString lastUrl = settings->value("LastPlayedUrl", "").toString();
    if (!lastUrl.isEmpty()) {
        urlInput->lineEdit()->setText(lastUrl);
        QTimer::singleShot(500, this, &MainWindow::onPlayButtonClicked);
    }
}

void MainWindow::loadSavedPassword()
{
    bool savePassword = settings->value("SavePassword", false).toBool();
    savePasswordCheckbox->setChecked(savePassword);

    if (savePassword) {
        QString savedPassword = settings->value("SavedPassword", "").toString();
        passwordInput->setText(savedPassword);
    }
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

    QMessageBox::information(this, "Removed", "URL removed from history.");
}

void MainWindow::onUrlSelected()
{
    // Load credentials for the selected URL if they exist
    QString selectedUrl = urlInput->currentText();
    if (!selectedUrl.isEmpty()) {
        loadCredentialsForUrl(selectedUrl);
    }
}

void MainWindow::saveCredentialsForUrl(const QString &url, const QString &username, const QString &password)
{
    settings->beginGroup("Credentials");
    settings->beginGroup(url);
    settings->setValue("username", username);
    settings->setValue("password", password);
    settings->endGroup();
    settings->endGroup();
    settings->sync();
}

void MainWindow::loadCredentialsForUrl(const QString &url)
{
    settings->beginGroup("Credentials");
    settings->beginGroup(url);
    QString username = settings->value("username", "").toString();
    QString password = settings->value("password", "").toString();
    settings->endGroup();
    settings->endGroup();

    usernameInput->setText(username);
    passwordInput->setText(password);
}

void MainWindow::removeCredentialsForUrl(const QString &url)
{
    settings->beginGroup("Credentials");
    settings->remove(url);
    settings->endGroup();
    settings->sync();
}
