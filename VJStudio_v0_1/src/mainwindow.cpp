#include "mainwindow.h"
#include "clipwidget.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QFileDialog>
#include <QListWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDockWidget>
#include <QStatusBar>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_player(new QMediaPlayer(this)),
      m_audioOutput(new QAudioOutput(this)),
      m_videoWidget(new QVideoWidget(this))
{
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoOutput(m_videoWidget);
    buildUi();

    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &error) {
        setStatus("Playback error: " + error);
    });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            setStatus("End of clip");
    });
}

void MainWindow::buildUi()
{
    setWindowTitle("VJ Studio 0.1 — Live Visual Performance");
    resize(1440, 900);

    auto *root = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(root);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto *toolbar = new QHBoxLayout;
    auto *importButton = new QPushButton("Import media");
    auto *playButton = new QPushButton("Play / Pause");
    auto *stopButton = new QPushButton("Stop");
    auto *outputButton = new QPushButton("Open Output 1");
    auto *output2Button = new QPushButton("Open Output 2");

    toolbar->addWidget(importButton);
    toolbar->addWidget(playButton);
    toolbar->addWidget(stopButton);
    toolbar->addStretch();
    toolbar->addWidget(outputButton);
    toolbar->addWidget(output2Button);
    mainLayout->addLayout(toolbar);

    auto *topSplitter = new QSplitter(Qt::Horizontal, root);

    auto *libraryBox = new QGroupBox("MEDIA LIBRARY");
    auto *libraryLayout = new QVBoxLayout(libraryBox);
    m_library = new QListWidget(libraryBox);
    libraryLayout->addWidget(m_library);
    libraryBox->setMinimumWidth(220);
    topSplitter->addWidget(libraryBox);

    auto *previewBox = new QGroupBox("COMPOSITION PREVIEW");
    auto *previewLayout = new QVBoxLayout(previewBox);
    m_videoWidget->setMinimumSize(480, 270);
    previewLayout->addWidget(m_videoWidget, 1);
    auto *transport = new QHBoxLayout;
    auto *playLabel = new QLabel("Preview / master playback");
    transport->addWidget(playLabel);
    transport->addStretch();
    previewLayout->addLayout(transport);
    topSplitter->addWidget(previewBox);

    auto *inspector = new QGroupBox("OUTPUT / STATUS");
    auto *inspectorLayout = new QVBoxLayout(inspector);
    inspectorLayout->addWidget(new QLabel("Output configuration"));
    inspectorLayout->addWidget(new QLabel("Output 1: Full HD (planned)"));
    inspectorLayout->addWidget(new QLabel("Output 2: Full HD (planned)"));
    inspectorLayout->addSpacing(12);
    inspectorLayout->addWidget(new QLabel("Layer mixer"));
    for (int i = 1; i <= 4; ++i) {
        auto *layer = new QHBoxLayout;
        layer->addWidget(new QLabel(QString("Layer %1").arg(i)));
        auto *enable = new QPushButton("ON");
        enable->setCheckable(true);
        enable->setChecked(true);
        enable->setMaximumWidth(55);
        layer->addStretch();
        layer->addWidget(enable);
        inspectorLayout->addLayout(layer);
    }
    inspectorLayout->addStretch();
    inspector->setMinimumWidth(190);
    topSplitter->addWidget(inspector);
    topSplitter->setStretchFactor(1, 3);
    mainLayout->addWidget(topSplitter, 3);

    auto *deckBox = new QGroupBox("CLIP DECK — 4 × 2 STARTER GRID");
    auto *deckLayout = new QGridLayout(deckBox);
    m_clipGrid = deckLayout;
    for (int i = 0; i < 8; ++i) {
        auto *clip = new ClipWidget(deckBox);
        deckLayout->addWidget(clip, i / 4, i % 4);
        connect(clip, &ClipWidget::playRequested, this, &MainWindow::playMedia);
    }
    mainLayout->addWidget(deckBox, 2);

    m_status = new QLabel("Ready — import video files to begin.");
    statusBar()->addWidget(m_status);
    setCentralWidget(root);

    connect(importButton, &QPushButton::clicked, this, &MainWindow::importVideos);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopPlayback);
    connect(outputButton, &QPushButton::clicked, this, &MainWindow::openOutputWindow);
    connect(output2Button, &QPushButton::clicked, this, [this] {
        setStatus("Output 2 is reserved for the independent-output milestone.");
    });
    connect(m_library, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        const QUrl url(item->data(Qt::UserRole).toString());
        playMedia(url);
    });
}

void MainWindow::importVideos()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, "Import video media", QString(),
        "Video files (*.mp4 *.mov *.mkv *.avi *.webm *.m4v);;All files (*.*)");

    for (const QString &path : paths) {
        const QUrl url = QUrl::fromLocalFile(path);
        auto *item = new QListWidgetItem(QFileInfo(path).fileName(), m_library);
        item->setData(Qt::UserRole, url.toString());
        addClip(url);
    }
    if (!paths.isEmpty())
        setStatus(QString("%1 media file(s) imported.").arg(paths.size()));
}

void MainWindow::addClip(const QUrl &url)
{
    const int capacity = 8;
    if (m_nextClip >= capacity) {
        setStatus("Starter deck is full (8 clips). More pages/decks are planned.");
        return;
    }
    auto *clip = qobject_cast<ClipWidget *>(m_clipGrid->itemAt(m_nextClip)->widget());
    if (clip)
        clip->setMedia(url);
    ++m_nextClip;
}

void MainWindow::playMedia(const QUrl &url)
{
    if (!url.isValid())
        return;
    m_player->setSource(url);
    m_player->play();
    setStatus("Playing: " + QFileInfo(url.toLocalFile()).fileName());
}

void MainWindow::togglePlayback()
{
    if (m_player->playbackState() == QMediaPlayer::PlayingState)
        m_player->pause();
    else
        m_player->play();
}

void MainWindow::stopPlayback()
{
    m_player->stop();
    setStatus("Playback stopped.");
}

void MainWindow::openOutputWindow()
{
    if (!m_outputWindow) {
        m_outputWindow = new QWidget(nullptr, Qt::Window);
        m_outputWindow->setWindowTitle("VJ Studio — Output 1");
        m_outputWindow->resize(1920, 1080);
        auto *layout = new QVBoxLayout(m_outputWindow);
        auto *note = new QLabel("OUTPUT 1\n\nIndependent fullscreen output is the next implementation milestone.");
        note->setAlignment(Qt::AlignCenter);
        layout->addWidget(note);
    }
    m_outputWindow->show();
    m_outputWindow->raise();
    setStatus("Output 1 window opened. Fullscreen routing is planned next.");
}

void MainWindow::setStatus(const QString &message)
{
    m_status->setText(message);
}
