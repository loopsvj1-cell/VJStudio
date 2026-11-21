#pragma once

#include <QWidget>
#include <QUrl>

class QLabel;
class QPushButton;

class ClipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClipWidget(QWidget *parent = nullptr);

    void setMedia(const QUrl &url);
    QUrl media() const { return m_url; }
    QString displayName() const;

signals:
    void playRequested(const QUrl &url);

private:
    QUrl m_url;
    QLabel *m_nameLabel;
    QPushButton *m_playButton;
};
