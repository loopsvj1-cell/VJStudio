#pragma once

#include <QWidget>
#include <QUrl>

class QLabel;
class QPushButton;
class QCheckBox;
class QMediaPlayer;
class QVideoSink;

class ClipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClipWidget(QWidget *parent = nullptr);

    void setMedia(const QUrl &url);
    QUrl media() const { return m_url; }
    QString displayName() const;

signals:
    void playRequested(const QUrl &url, bool loop);

private:
    QUrl m_url;
    QLabel *m_thumbnailLabel = nullptr;
    QLabel *m_nameLabel = nullptr;
    QPushButton *m_playButton = nullptr;
    QCheckBox *m_loopCheck = nullptr;
    QMediaPlayer *m_previewPlayer = nullptr;
    QVideoSink *m_videoSink = nullptr;
    bool m_thumbnailCaptured = false;
};
