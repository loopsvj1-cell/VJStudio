#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    // Use Windows Media Foundation instead of Qt 6.8's default FFmpeg backend.
    // This works around the SWR resampler initialization issue seen on some Windows systems.
    qputenv("QT_MEDIA_BACKEND", "windows");
#endif
    QApplication app(argc, argv);
    QApplication::setApplicationName("VJ Studio");
    QApplication::setOrganizationName("VJStudio");

    MainWindow window;
    window.show();
    return app.exec();
}
