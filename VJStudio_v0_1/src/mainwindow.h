#pragma once

#include <QMainWindow>
#include <QUrl>

class QMediaPlayer;
class QAudioOutput;
class QVideoWidget;
class QListWidget;
class QGridLayout;
class QLabel;
class ClipWidget;
class QSlider;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void importVideos();
    void playMedia(const QUrl &url);
    void togglePlayback();
    void stopPlayback();
    void openOutputWindow();

private:
    void buildUi();
    void addClip(const QUrl &url);
    void setStatus(const QString &message);

    QMediaPlayer *m_player;
    QAudioOutput *m_audioOutput;
    QVideoWidget *m_videoWidget;
    QListWidget *m_library;
    QGridLayout *m_clipGrid;
    QLabel *m_status;
    QWidget *m_outputWindow = nullptr;
    int m_nextClip = 0;
};
