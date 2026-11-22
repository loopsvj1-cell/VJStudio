#pragma once

#include <QMainWindow>
#include <QUrl>
#include <array>

class QMediaPlayer;
class QAudioOutput;
class QGraphicsScene;
class QGraphicsView;
class QGraphicsVideoItem;
class QListWidget;
class QGridLayout;
class QLabel;
class ClipWidget;
class QSlider;
class QComboBox;
class QPushButton;
class QResizeEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void importVideos();
    void loadSelectedMedia();
    void toggleSelectedLayerPlayback();
    void stopSelectedLayer();
    void openOutputWindow();
    void saveProject();
    void openProject();

private:
    struct Layer {
        QMediaPlayer *player = nullptr;
        QAudioOutput *audio = nullptr;
        QGraphicsVideoItem *video = nullptr;
        QUrl source;
        bool loop = false;
    };

    void buildUi();
    void addClip(const QUrl &url);
    void loadMediaIntoLayer(int layerIndex, const QUrl &url, bool startPlayback, bool loop = false);
    void setActiveLayer(int index);
    void setStatus(const QString &message);
    void updatePreviewGeometry();

    std::array<Layer, 4> m_layers;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_previewView = nullptr;
    QListWidget *m_library = nullptr;
    QGridLayout *m_clipGrid = nullptr;
    QLabel *m_status = nullptr;
    QComboBox *m_layerSelector = nullptr;
    QSlider *m_opacitySlider = nullptr;
    QPushButton *m_layerPlayButton = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QPushButton *m_muteButton = nullptr;
    bool m_audioMuted = false;
    QWidget *m_outputWindow = nullptr;
    QGraphicsView *m_outputView = nullptr;
    int m_activeLayer = 0;
    int m_nextClip = 0;
};
