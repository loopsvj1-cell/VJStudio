#pragma once

#include <QWidget>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>

class QLabel;
class QPushButton;
class QCheckBox;
class QMediaPlayer;
class QVideoSink;
class QProgressBar;

class ClipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClipWidget(QWidget *parent = nullptr);

    void setMedia(const QUrl &url);
    QUrl media() const { return m_url; }
    QString displayName() const;
    void setSlotNumber(int number);
    void setPlaying(bool playing);
    void setPlaybackProgress(qint64 position, qint64 duration);
    void triggerPlay();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void playRequested(const QUrl &url, bool loop);
    void mediaDropped(const QUrl &url);
    void selected(ClipWidget *clip);

private:
    QUrl m_url;
    QLabel *m_thumbnailLabel = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_playButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_timeLabel = nullptr;
    QCheckBox *m_loopCheck = nullptr;
    QMediaPlayer *m_previewPlayer = nullptr;
    QVideoSink *m_videoSink = nullptr;
    bool m_thumbnailCaptured = false;
    int m_slotNumber = 0;
};
