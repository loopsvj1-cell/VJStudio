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
#include <QProgressBar>

ClipWidget::ClipWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("clipWidget");
    setAcceptDrops(true);
    setMinimumSize(130, 112);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_thumbnailLabel = new QLabel("VIDEO PREVIEW", this);
    m_thumbnailLabel->setAlignment(Qt::AlignCenter);
    m_thumbnailLabel->setFixedSize(160, 42);
    m_thumbnailLabel->setScaledContents(true);
    m_thumbnailLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_thumbnailLabel->setStyleSheet(
        "QLabel { background: #080a11; color: #777f9b; border: 1px solid #34384a; border-radius: 4px; }");

    m_nameLabel = new QLabel("Empty clip", this);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setMaximumHeight(34);

    m_playButton = new QPushButton("PLAY", this);
    m_playButton->setEnabled(false);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 1000);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(7);
    m_progressBar->setStyleSheet("QProgressBar { background: #0c0e15; border: 0; border-radius: 3px; } QProgressBar::chunk { background: #9b7bff; border-radius: 3px; }");
    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet("QLabel { color: #aeb4c8; font-size: 8pt; }");

    m_marksLabel = new QLabel("IN 00:00  |  OUT END", this);
    m_marksLabel->setAlignment(Qt::AlignCenter);
    m_marksLabel->setStyleSheet("QLabel { color: #b9a6ff; font-size: 8pt; }");
    m_inButton = new QPushButton("SET IN", this);
    m_outButton = new QPushButton("SET OUT", this);
    m_clearMarksButton = new QPushButton("CLEAR", this);
    m_inButton->setEnabled(false);
    m_outButton->setEnabled(false);
    m_inButton->setToolTip("Set the start point at the current playback position");
    m_outButton->setToolTip("Set the end point at the current playback position");
    auto *markControls = new QHBoxLayout;
    markControls->setContentsMargins(0, 0, 0, 0);
    markControls->addWidget(m_inButton);
    markControls->addWidget(m_outButton);
    markControls->addWidget(m_clearMarksButton);

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
    auto *thumbnailRow = new QHBoxLayout;
    thumbnailRow->setContentsMargins(0, 0, 0, 0);
    thumbnailRow->addStretch();
    thumbnailRow->addWidget(m_thumbnailLabel);
    thumbnailRow->addStretch();
    layout->addLayout(thumbnailRow);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_progressBar);
    layout->addWidget(m_timeLabel);
    layout->addWidget(m_marksLabel);
    layout->addLayout(markControls);
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
                160, 42, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            m_thumbnailLabel->setPixmap(thumbnail);
            m_previewPlayer->stop();
            m_previewPlayer->setSource(QUrl());
        }
    });

    connect(m_playButton, &QPushButton::clicked, this, [this] { triggerPlay(); });
    connect(m_inButton, &QPushButton::clicked, this, [this] { emit markInRequested(this); });
    connect(m_outButton, &QPushButton::clicked, this, [this] { emit markOutRequested(this); });
    connect(m_clearMarksButton, &QPushButton::clicked, this, [this] { emit clearMarksRequested(this); });
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
        m_inButton->setEnabled(false);
        m_outButton->setEnabled(false);
        m_clearMarksButton->setEnabled(false);
        m_inPoint = 0;
        m_outPoint = -1;
        m_duration = 0;
        m_marksLabel->setText("IN 00:00  |  OUT END");
        setPlaybackProgress(0, 0);
        return;
    }

    m_thumbnailLabel->setText("Loading preview...");
    m_nameLabel->setText(QFileInfo(url.toLocalFile()).fileName());
    m_playButton->setEnabled(true);
    m_inButton->setEnabled(true);
    m_outButton->setEnabled(true);
    m_clearMarksButton->setEnabled(true);
    m_inPoint = 0;
    m_outPoint = -1;
    m_duration = 0;
    m_marksLabel->setText("IN 00:00  |  OUT END");
    setPlaybackProgress(0, 0);
    m_previewPlayer->setSource(url);
    m_previewPlayer->play();
}

QString ClipWidget::displayName() const
{
    return m_url.isValid() ? QFileInfo(m_url.toLocalFile()).fileName() : "Empty clip";
}

void ClipWidget::setSlotNumber(int number)
{
    m_slotNumber = number;
    setPlaying(false);
}

void ClipWidget::setPlaying(bool playing)
{
    m_playButton->setText(QString("%1  %2").arg(m_slotNumber).arg(playing ? "PLAYING" : "PLAY"));
}

void ClipWidget::setPlaybackProgress(qint64 position, qint64 duration)
{
    m_duration = duration;
    if (duration <= 0) {
        m_progressBar->setValue(0);
        m_timeLabel->setText("00:00 / --:--");
        return;
    }
    const auto formatTime = [](qint64 milliseconds) {
        const qint64 seconds = qMax<qint64>(0, milliseconds) / 1000;
        return QString("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0'))
                               .arg(seconds % 60, 2, 10, QLatin1Char('0'));
    };
    const qint64 boundedPosition = qBound<qint64>(0, position, duration);
    m_progressBar->setValue(static_cast<int>((boundedPosition * 1000) / duration));
    m_timeLabel->setText(formatTime(boundedPosition) + " / " + formatTime(duration));
}

void ClipWidget::setInPoint(qint64 position)
{
    m_inPoint = qBound<qint64>(0, position, m_duration > 0 ? m_duration : position);
    if (m_outPoint >= 0 && m_outPoint <= m_inPoint)
        m_outPoint = -1;
    const auto fmt = [](qint64 ms) { const qint64 s = ms / 1000; return QString("%1:%2").arg(s / 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0')); };
    m_marksLabel->setText("IN " + fmt(m_inPoint) + "  |  OUT " + (m_outPoint < 0 ? QString("END") : fmt(m_outPoint)));
}

void ClipWidget::setOutPoint(qint64 position)
{
    const qint64 limit = m_duration > 0 ? m_duration : position;
    if (position <= m_inPoint) {
        m_marksLabel->setText("OUT must be after IN");
        return;
    }
    m_outPoint = qBound<qint64>(m_inPoint + 1, position, limit);
    const auto fmt = [](qint64 ms) { const qint64 s = ms / 1000; return QString("%1:%2").arg(s / 60, 2, 10, QLatin1Char('0')).arg(s % 60, 2, 10, QLatin1Char('0')); };
    m_marksLabel->setText("IN " + fmt(m_inPoint) + "  |  OUT " + fmt(m_outPoint));
}

void ClipWidget::clearMarks()
{
    m_inPoint = 0;
    m_outPoint = -1;
    m_marksLabel->setText("IN 00:00  |  OUT END");
}

void ClipWidget::triggerPlay()
{
    if (m_url.isValid() && !m_url.isEmpty()) {
        emit selected(this);
        setPlaying(true);
        emit playRequested(m_url, m_loopCheck->isChecked(), m_inPoint, m_outPoint);
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
