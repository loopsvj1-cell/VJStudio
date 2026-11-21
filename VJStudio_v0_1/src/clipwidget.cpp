#include "clipwidget.h"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QPixmap>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

ClipWidget::ClipWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("clipWidget");
    setAcceptDrops(true);
    setMinimumSize(130, 112);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_thumbnailLabel = new QLabel("VIDEO PREVIEW", this);
    m_thumbnailLabel->setAlignment(Qt::AlignCenter);
    m_thumbnailLabel->setMinimumHeight(64);
    m_thumbnailLabel->setFixedHeight(84);
    m_thumbnailLabel->setScaledContents(true);
    m_thumbnailLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    m_thumbnailLabel->setMinimumWidth(0);
    m_thumbnailLabel->setStyleSheet(
        "QLabel { background: #080a11; color: #777f9b; border: 1px solid #34384a; border-radius: 4px; }");

    m_nameLabel = new QLabel("Empty clip", this);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setMaximumHeight(34);

    m_playButton = new QPushButton("PLAY", this);
    m_playButton->setEnabled(false);

    m_loopCheck = new QCheckBox("LOOP", this);
    m_loopCheck->setChecked(true);
    m_loopCheck->setToolTip("Repeat this clip when it reaches the end");

    auto *controls = new QHBoxLayout;
    controls->setContentsMargins(0, 0, 0, 0);
    controls->addWidget(m_loopCheck);
    controls->addWidget(m_playButton, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    layout->addWidget(m_thumbnailLabel, 1);
    layout->addWidget(m_nameLabel);
    layout->addLayout(controls);

    m_previewPlayer = new QMediaPlayer(this);
    m_videoSink = new QVideoSink(this);
    m_previewPlayer->setVideoSink(m_videoSink);

    connect(m_videoSink, &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame &frame) {
        const QImage image = frame.toImage();
        if (!image.isNull() && !m_thumbnailCaptured) {
            // Capture exactly one frame for the static clip-deck thumbnail.
            // Stop and detach the source immediately so idle clips do not keep
            // decoding video or consuming GPU/CPU resources in the background.
            m_thumbnailCaptured = true;
            const QPixmap thumbnail = QPixmap::fromImage(image).scaled(
                320, 84, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            m_thumbnailLabel->setPixmap(thumbnail);
            m_previewPlayer->stop();
            m_previewPlayer->setSource(QUrl());
        }
    });

    connect(m_playButton, &QPushButton::clicked, this, [this] { triggerPlay(); });
}

void ClipWidget::setMedia(const QUrl &url)
{
    m_previewPlayer->stop();
    m_previewPlayer->setSource(QUrl());
    m_thumbnailCaptured = false;
    m_url = url;
    m_thumbnailLabel->setPixmap(QPixmap());
    if (!m_url.isValid() || m_url.isEmpty()) {
        m_thumbnailLabel->setText("VIDEO PREVIEW");
        m_nameLabel->setText("Empty clip");
        m_playButton->setEnabled(false);
        return;
    }

    m_thumbnailLabel->setText("Loading preview...");
    m_nameLabel->setText(QFileInfo(url.toLocalFile()).fileName());
    m_playButton->setEnabled(true);
    m_previewPlayer->setSource(url);
    m_previewPlayer->play();
}

QString ClipWidget::displayName() const
{
    return m_url.isValid() ? QFileInfo(m_url.toLocalFile()).fileName() : "Empty clip";
}

void ClipWidget::setSlotNumber(int number)
{
    m_playButton->setText(QString("%1  PLAY").arg(number));
}

void ClipWidget::triggerPlay()
{
    if (m_url.isValid() && !m_url.isEmpty()) {
        emit selected(this);
        emit playRequested(m_url, m_loopCheck->isChecked());
    }
}

void ClipWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
    else
        event->ignore();
}

void ClipWidget::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty() && urls.first().isLocalFile()) {
        emit mediaDropped(urls.first());
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}
