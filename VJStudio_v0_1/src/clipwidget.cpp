#include "clipwidget.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileInfo>

ClipWidget::ClipWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("clipWidget");
    setMinimumSize(130, 86);

    m_nameLabel = new QLabel("Empty clip", this);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setAlignment(Qt::AlignCenter);

    m_playButton = new QPushButton("PLAY", this);
    m_playButton->setEnabled(false);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_nameLabel, 1);
    layout->addWidget(m_playButton);

    connect(m_playButton, &QPushButton::clicked, this, [this] {
        if (m_url.isValid())
            emit playRequested(m_url);
    });
}

void ClipWidget::setMedia(const QUrl &url)
{
    m_url = url;
    m_nameLabel->setText(QFileInfo(url.toLocalFile()).fileName());
    m_playButton->setEnabled(true);
}

QString ClipWidget::displayName() const
{
    return m_url.isValid() ? QFileInfo(m_url.toLocalFile()).fileName() : "Empty clip";
}
