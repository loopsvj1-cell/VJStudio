#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("VJ Studio");
    QApplication::setOrganizationName("VJStudio");

    MainWindow window;
    window.show();
    return app.exec();
}
