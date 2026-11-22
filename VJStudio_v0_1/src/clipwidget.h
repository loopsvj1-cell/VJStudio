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
    void setInPoint(qint64 position);
    void setOutPoint(qint64 position);
    qint64 inPoint() const { return m_inPoint; }
    qint64 outPoint() const { return m_outPoint; }
    void triggerPlay();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void playRequested(const QUrl &url, bool loop, qint64 inPoint, qint64 outPoint);
    void mediaDropped(const QUrl &url);
    void selected(ClipWidget *clip);
    void markInRequested(ClipWidget *clip);
    void markOutRequested(ClipWidget *clip);

private:
    QUrl m_url;
    QLabel *m_thumbnailLabel = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_playButton = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLabel *m_marksLabel = nullptr;
    QPushButton *m_inButton = nullptr;
    QPushButton *m_outButton = nullptr;
    QCheckBox *m_loopCheck = nullptr;
    QMediaPlayer *m_previewPlayer = nullptr;
    QVideoSink *m_videoSink = nullptr;
    bool m_thumbnailCaptured = false;
    int m_slotNumber = 0;
    qint64 m_inPoint = 0;
    qint64 m_outPoint = -1;
    qint64 m_duration = 0;
};
