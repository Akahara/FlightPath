#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    MainWindow mainWindow;
    mainWindow.resize(400,250);

    // center the window
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - mainWindow.width()) / 2;
    int y = (screenGeometry.height() - mainWindow.height()) / 2;
    mainWindow.move(x, y);
    mainWindow.show();

    return application.exec();
}
