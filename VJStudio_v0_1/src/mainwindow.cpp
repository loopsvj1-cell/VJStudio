#include "mainwindow.h"
#include "clipwidget.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QGraphicsVideoItem>
#include <QGraphicsScene>
#include <QGraphicsView>
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
#include <QStatusBar>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QResizeEvent>
#include <QApplication>
#include <QFrame>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 1920, 1080);
    m_scene->setBackgroundBrush(Qt::black);

    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
        auto &layer = m_layers[static_cast<size_t>(i)];
        layer.player = new QMediaPlayer(this);
        layer.audio = new QAudioOutput(this);
        layer.audio->setDevice(QMediaDevices::defaultAudioOutput());
        layer.video = new QGraphicsVideoItem();
        layer.video->setPos(0, 0);
        layer.video->setSize(QSizeF(1920, 1080));
        layer.video->setAspectRatioMode(Qt::KeepAspectRatio);
        layer.video->setZValue(i);
        layer.video->setOpacity(i == 0 ? 1.0 : 0.0);
        m_scene->addItem(layer.video);
        layer.audio->setVolume(1.0);
        layer.audio->setMuted(false);
        layer.player->setAudioOutput(layer.audio);
        layer.player->setVideoOutput(layer.video);

        connect(layer.player, &QMediaPlayer::positionChanged, this,
                [this, i](qint64 position) {
            auto &currentLayer = m_layers[static_cast<size_t>(i)];
            if (currentLayer.activeClip)
                currentLayer.activeClip->setPlaybackProgress(position, currentLayer.player->duration());
        });
        connect(layer.player, &QMediaPlayer::durationChanged, this,
                [this, i](qint64 duration) {
            auto &currentLayer = m_layers[static_cast<size_t>(i)];
            if (currentLayer.activeClip)
                currentLayer.activeClip->setPlaybackProgress(currentLayer.player->position(), duration);
        });
        connect(layer.player, &QMediaPlayer::audioAvailableChanged, this,
                [this, i](bool available) {
            if (i == m_activeLayer && m_audioStatus)
                m_audioStatus->setText(available ? "Audio track detected" : "No audio track detected yet");
        });
        connect(layer.player, &QMediaPlayer::errorOccurred, this,
                [this, i](QMediaPlayer::Error, const QString &error) {
            setStatus(QString("Layer %1 playback error: %2").arg(i + 1).arg(error));
        });
        connect(layer.player, &QMediaPlayer::mediaStatusChanged, this,
                [this, i](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::EndOfMedia) {
                setStatus(QString("Layer %1 reached end of clip").arg(i + 1));
                auto &endedLayer = m_layers[static_cast<size_t>(i)];
                if (!endedLayer.loop && endedLayer.activeClip) {
                    endedLayer.activeClip->setPlaying(false);
                    endedLayer.activeClip->setPlaybackProgress(
                        endedLayer.player->duration(), endedLayer.player->duration());
                }
            }
        });
    }

    buildUi();
}

void MainWindow::buildUi()
{
    setWindowTitle("VJ Studio 0.5 — Multilayer Video");
    qApp->setStyleSheet(R"(
        QWidget { background: #171923; color: #e8eaf2; font-family: "Segoe UI"; font-size: 10pt; }
        QMainWindow, QDialog { background: #11131b; }
        QGroupBox { border: 1px solid #34384a; border-radius: 8px; margin-top: 12px; padding: 10px; font-weight: 600; color: #b9a6ff; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
        QPushButton { background: #292d3d; border: 1px solid #454b63; border-radius: 6px; padding: 8px 12px; }
        QPushButton:hover { background: #39345a; border-color: #9274ff; }
        QPushButton:pressed { background: #6246c7; }
        QListWidget, QComboBox, QSlider, QGraphicsView { background: #0c0e15; border: 1px solid #34384a; border-radius: 5px; }
        QComboBox { padding: 6px; }
        QSlider::groove:horizontal { height: 5px; background: #34384a; border-radius: 2px; }
        QSlider::handle:horizontal { background: #9b7bff; width: 14px; margin: -5px 0; border-radius: 7px; }
        QStatusBar { background: #11131b; color: #aeb4c8; }
        QWidget#clipWidget { background: #202436; border: 1px solid #414760; border-radius: 7px; }
    )");
    resize(1500, 920);

    auto *root = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(root);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto *toolbar = new QHBoxLayout;
    auto *importButton = new QPushButton("Import media");
    auto *loadButton = new QPushButton("Load to selected layer");
    auto *outputButton = new QPushButton("Open Output 1");
    auto *saveButton = new QPushButton("Save Project");
    auto *openProjectButton = new QPushButton("Open Project");
    toolbar->addWidget(importButton);
    toolbar->addWidget(openProjectButton);
    toolbar->addWidget(saveButton);
    toolbar->addWidget(loadButton);
    toolbar->addStretch();
    toolbar->addWidget(outputButton);
    mainLayout->addLayout(toolbar);

    auto *topSplitter = new QSplitter(Qt::Horizontal, root);

    auto *libraryBox = new QGroupBox("MEDIA LIBRARY");
    auto *libraryLayout = new QVBoxLayout(libraryBox);
    m_library = new QListWidget(libraryBox);
    libraryLayout->addWidget(m_library);
    libraryBox->setMinimumWidth(220);
    topSplitter->addWidget(libraryBox);

    auto *previewBox = new QGroupBox("COMPOSITION PREVIEW — 4 VIDEO LAYERS");
    auto *previewLayout = new QVBoxLayout(previewBox);
    m_previewView = new QGraphicsView(m_scene, previewBox);
    m_previewView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    m_previewView->setBackgroundBrush(Qt::black);
    m_previewView->setMinimumSize(480, 270);
    m_previewView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    previewLayout->addWidget(m_previewView, 1);
    topSplitter->addWidget(previewBox);

    auto *mixerBox = new QGroupBox("LAYER MIXER");
    auto *mixerLayout = new QVBoxLayout(mixerBox);
    m_layerSelector = new QComboBox(mixerBox);
    for (int i = 0; i < 4; ++i)
        m_layerSelector->addItem(QString("Layer %1").arg(i + 1), i);
    mixerLayout->addWidget(new QLabel("Selected layer"));
    mixerLayout->addWidget(m_layerSelector);

    m_layerPlayButton = new QPushButton("Play / Pause");
    auto *stopButton = new QPushButton("Stop layer");
    mixerLayout->addWidget(m_layerPlayButton);
    mixerLayout->addWidget(stopButton);

    mixerLayout->addWidget(new QLabel("Layer opacity"));
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(0, 100);
    m_opacitySlider->setValue(100);
    mixerLayout->addWidget(m_opacitySlider);
    mixerLayout->addSpacing(12);
    mixerLayout->addWidget(new QLabel("Audio output device"));
    m_audioDeviceSelector = new QComboBox(mixerBox);
    const QAudioDevice defaultAudioDevice = QMediaDevices::defaultAudioOutput();
    const auto audioDevices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : audioDevices) {
        m_audioDeviceSelector->addItem(device.description(), device.id());
        if (device.id() == defaultAudioDevice.id())
            m_audioDeviceSelector->setCurrentIndex(m_audioDeviceSelector->count() - 1);
    }
    m_audioDeviceSelector->setEnabled(!audioDevices.isEmpty());
    mixerLayout->addWidget(m_audioDeviceSelector);
    m_audioStatus = new QLabel(audioDevices.isEmpty() ? "No audio output device found" : "Audio output ready");
    m_audioStatus->setWordWrap(true);
    mixerLayout->addWidget(m_audioStatus);
    mixerLayout->addWidget(new QLabel("Master audio volume"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    mixerLayout->addWidget(m_volumeSlider);
    m_muteButton = new QPushButton("Mute audio");
    mixerLayout->addWidget(m_muteButton);
    mixerLayout->addWidget(new QLabel("Audio is routed through the selected layer player."));
    mixerLayout->addStretch();
    mixerBox->setMinimumWidth(210);
    topSplitter->addWidget(mixerBox);

    topSplitter->setStretchFactor(1, 4);
    mainLayout->addWidget(topSplitter, 3);

    auto *deckBox = new QGroupBox("CLIP DECK — 4 × 2");
    m_clipGrid = new QGridLayout(deckBox);
    for (int column = 0; column < 4; ++column)
        m_clipGrid->setColumnStretch(column, 1);
    for (int row = 0; row < 2; ++row)
        m_clipGrid->setRowStretch(row, 1);
    for (int i = 0; i < 8; ++i) {
        auto *clip = new ClipWidget(deckBox);
        clip->setSlotNumber(i + 1);
        clip->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        m_clipGrid->addWidget(clip, i / 4, i % 4);
        connect(clip, &ClipWidget::playRequested, this, [this, clip](const QUrl &url, bool loop) {
            m_layers[static_cast<size_t>(m_activeLayer)].activeClip = clip;
            loadMediaIntoLayer(m_activeLayer, url, true, loop);
        });
        connect(clip, &ClipWidget::mediaDropped, this, [this, clip](const QUrl &url) {
            clip->setMedia(url);
            setStatus(QString("Clip loaded: %1").arg(QFileInfo(url.toLocalFile()).fileName()));
        });
        connect(clip, &ClipWidget::selected, this, [this](ClipWidget *selectedClip) {
            m_activeClip = selectedClip;
            for (int slot = 0; slot < m_clipGrid->count(); ++slot) {
                auto *item = qobject_cast<ClipWidget *>(m_clipGrid->itemAt(slot)->widget());
                if (item) {
                    item->setStyleSheet(item == selectedClip
                        ? "QWidget#clipWidget { border: 2px solid #a78bfa; background: #292642; border-radius: 7px; }"
                        : "");
                    item->setPlaying(item == selectedClip);
                }
            }
        });
        auto *shortcut = new QShortcut(QKeySequence(QString::number(i + 1)), this);
        connect(shortcut, &QShortcut::activated, clip, &ClipWidget::triggerPlay);
    }
    mainLayout->addWidget(deckBox, 2);

    m_status = new QLabel("Ready — import media, select a layer and load a clip.");
    statusBar()->addWidget(m_status);
    setCentralWidget(root);

    connect(importButton, &QPushButton::clicked, this, &MainWindow::importVideos);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadSelectedMedia);
    connect(outputButton, &QPushButton::clicked, this, &MainWindow::openOutputWindow);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveProject);
    connect(openProjectButton, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(m_layerSelector, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &MainWindow::setActiveLayer);
    connect(m_layerPlayButton, &QPushButton::clicked,
            this, &MainWindow::toggleSelectedLayerPlayback);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopSelectedLayer);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        const qreal volume = value / 100.0;
        for (auto &layer : m_layers)
            layer.audio->setVolume(volume);
        setStatus(QString("Master audio volume: %1%").arg(value));
    });
    connect(m_audioDeviceSelector, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index < 0)
            return;
        const QByteArray selectedId = m_audioDeviceSelector->itemData(index).toByteArray();
        for (const QAudioDevice &device : QMediaDevices::audioOutputs()) {
            if (device.id() == selectedId) {
                for (auto &layer : m_layers)
                    layer.audio->setDevice(device);
                setStatus("Audio output: " + device.description());
                return;
            }
        }
    });
    connect(m_muteButton, &QPushButton::clicked, this, [this]() {
        m_audioMuted = !m_audioMuted;
        for (auto &layer : m_layers)
            layer.audio->setMuted(m_audioMuted);
        m_muteButton->setText(m_audioMuted ? "Unmute audio" : "Mute audio");
        setStatus(m_audioMuted ? "Audio muted." : "Audio enabled.");
    });
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_layers[static_cast<size_t>(m_activeLayer)].video->setOpacity(value / 100.0);
        if (m_outputView)
            m_outputView->viewport()->update();
    });
    connect(m_library, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) {
        loadMediaIntoLayer(m_activeLayer, QUrl(item->data(Qt::UserRole).toString()), true);
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
        setStatus(QString("%1 media file(s) imported. Double-click a file or load it to a layer.")
                  .arg(paths.size()));
}

void MainWindow::addClip(const QUrl &url)
{
    if (m_nextClip >= 8) {
        setStatus("Clip deck is full (8 clips). Media remains available in the library.");
        return;
    }
    auto *clip = qobject_cast<ClipWidget *>(m_clipGrid->itemAt(m_nextClip)->widget());
    if (clip)
        clip->setMedia(url);
    ++m_nextClip;
}

void MainWindow::loadSelectedMedia()
{
    const auto *item = m_library->currentItem();
    if (!item) {
        setStatus("Select a media file in the library first.");
        return;
    }
    loadMediaIntoLayer(m_activeLayer, QUrl(item->data(Qt::UserRole).toString()), true);
}

void MainWindow::loadMediaIntoLayer(int layerIndex, const QUrl &url, bool startPlayback, bool loop)
{
    if (layerIndex < 0 || layerIndex >= static_cast<int>(m_layers.size()) || !url.isValid())
        return;

    auto &layer = m_layers[static_cast<size_t>(layerIndex)];
    layer.player->stop();
    if (layer.activeClip && layer.activeClip->media() != url)
        layer.activeClip = nullptr;
    layer.source = url;
    layer.loop = loop;
    layer.player->setLoops(loop ? QMediaPlayer::Infinite : 1);
    layer.player->setSource(url);
    if (startPlayback)
        layer.player->play();

    setStatus(QString("Layer %1: %2").arg(layerIndex + 1)
              .arg(QFileInfo(url.toLocalFile()).fileName()));
    if (m_layerSelector->currentIndex() == layerIndex)
        m_layerPlayButton->setText(startPlayback ? "Pause layer" : "Play layer");
    if (m_audioStatus && layerIndex == m_activeLayer)
        m_audioStatus->setText("Checking audio track...");
}

void MainWindow::setActiveLayer(int index)
{
    if (index < 0 || index >= static_cast<int>(m_layers.size()))
        return;
    m_activeLayer = index;
    const auto &layer = m_layers[static_cast<size_t>(index)];
    m_opacitySlider->setValue(qRound(layer.video->opacity() * 100.0));
    m_layerPlayButton->setText(
        layer.player->playbackState() == QMediaPlayer::PlayingState ? "Pause layer" : "Play layer");
    setStatus(QString("Selected Layer %1").arg(index + 1));
}

void MainWindow::toggleSelectedLayerPlayback()
{
    auto &player = m_layers[static_cast<size_t>(m_activeLayer)].player;
    if (player->playbackState() == QMediaPlayer::PlayingState) {
        player->pause();
        m_layerPlayButton->setText("Play layer");
    } else {
        player->play();
        m_layerPlayButton->setText("Pause layer");
    }
}

void MainWindow::stopSelectedLayer()
{
    auto &player = m_layers[static_cast<size_t>(m_activeLayer)].player;
    player->stop();
    if (m_activeClip)
        m_activeClip->setPlaying(false);
    m_layerPlayButton->setText("Play layer");
    setStatus(QString("Layer %1 stopped.").arg(m_activeLayer + 1));
}

void MainWindow::openOutputWindow()
{
    if (!m_outputWindow) {
        m_outputWindow = new QWidget(nullptr, Qt::Window);
        m_outputWindow->setWindowTitle("VJ Studio — Output 1");
        m_outputWindow->resize(1280, 720);
        auto *layout = new QVBoxLayout(m_outputWindow);
        layout->setContentsMargins(0, 0, 0, 0);
        m_outputView = new QGraphicsView(m_scene, m_outputWindow);
        m_outputView->setFrameShape(QFrame::NoFrame);
        m_outputView->setBackgroundBrush(Qt::black);
        m_outputView->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        layout->addWidget(m_outputView);
    }
    m_outputWindow->show();
    m_outputWindow->raise();
    m_outputView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    setStatus("Output 1 opened — showing the live multilayer composition.");
}

void MainWindow::updatePreviewGeometry()
{
    if (m_previewView)
        m_previewView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updatePreviewGeometry();
}

void MainWindow::saveProject()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Save VJ Studio project", QString(), "VJ Studio Project (*.vjproj)");
    if (path.isEmpty())
        return;

    QJsonObject project;
    project["format"] = "VJStudioProject";
    project["version"] = 1;
    QJsonArray media;
    for (int i = 0; i < m_library->count(); ++i) {
        const auto *item = m_library->item(i);
        media.append(item->data(Qt::UserRole).toString());
    }
    project["mediaLibrary"] = media;

    QJsonArray layers;
    for (const auto &layer : m_layers) {
        QJsonObject entry;
        entry["source"] = layer.source.toString();
        entry["opacity"] = layer.video->opacity();
        entry["loop"] = layer.loop;
        layers.append(entry);
    }
    project["layers"] = layers;
    project["activeLayer"] = m_activeLayer;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save project", "Could not write the project file.");
        return;
    }
    file.write(QJsonDocument(project).toJson(QJsonDocument::Indented));
    setStatus("Project saved: " + QFileInfo(path).fileName());
}

void MainWindow::openProject()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open VJ Studio project", QString(), "VJ Studio Project (*.vjproj)");
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Open project", "Could not read the project file.");
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()
        || document.object().value("format").toString() != "VJStudioProject") {
        QMessageBox::warning(this, "Open project", "This is not a valid VJ Studio project.");
        return;
    }

    const QJsonObject project = document.object();
    m_library->clear();
    m_nextClip = 0;
    for (int i = 0; i < m_clipGrid->count(); ++i) {
        auto *clip = qobject_cast<ClipWidget *>(m_clipGrid->itemAt(i)->widget());
        if (clip)
            clip->setMedia(QUrl());
    }

    const QJsonArray media = project.value("mediaLibrary").toArray();
    for (const QJsonValue &value : media) {
        const QUrl url(value.toString());
        if (!url.isValid())
            continue;
        auto *item = new QListWidgetItem(QFileInfo(url.toLocalFile()).fileName(), m_library);
        item->setData(Qt::UserRole, url.toString());
        addClip(url);
    }

    const QJsonArray layers = project.value("layers").toArray();
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
        auto &layer = m_layers[static_cast<size_t>(i)];
        layer.player->stop();
        layer.source = QUrl();
        layer.player->setSource(QUrl());
        layer.video->setOpacity(0.0);
        if (i < layers.size()) {
            const QJsonObject entry = layers.at(i).toObject();
            const QUrl source(entry.value("source").toString());
            const qreal opacity = qBound(0.0, entry.value("opacity").toDouble(0.0), 1.0);
            layer.video->setOpacity(opacity);
            if (source.isValid() && !source.isEmpty())
                loadMediaIntoLayer(i, source, false, entry.value("loop").toBool(false));
        }
    }

    const int active = project.value("activeLayer").toInt(0);
    setActiveLayer(qBound(0, active, static_cast<int>(m_layers.size()) - 1));
    setStatus("Project opened: " + QFileInfo(path).fileName()
              + ". Media files must remain in their original locations.");
}

void MainWindow::setStatus(const QString &message)
{
    m_status->setText(message);
}

