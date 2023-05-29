#include "mainwindow.h"
#include "mainwindowtest.h"

#include <QApplication>
#include <QFile>
#include <QScreen>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //Set the app style sheet
    //QFile styleSheetFile("E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/Adaptic.qss");
    //QFile styleSheetFile("E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/Combinear.qss");
    //QFile styleSheetFile("/Users/thomasraymond/Documents/Etudes/DI4/S8/Projet collectif/ProjetS8/Interface_Graphique/Adaptic.qss");
    //QFile styleSheetFile("/Users/thomasraymond/Documents/Etudes/DI4/S8/Projet collectif/ProjetS8/Interface_Graphique/Combinear.qss");
    //styleSheetFile.open(QFile::ReadOnly);
    //QString styleSheet = QLatin1String(styleSheetFile.readAll());
    //a.setStyleSheet(styleSheet);

    MainWindowTest mainWindow;
    mainWindow.resize(1400,800);

    // 获取主屏幕信息
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    // 移动窗口到屏幕中心
    int x = (screenGeometry.width() - mainWindow.width()) / 2;
    int y = (screenGeometry.height() - mainWindow.height()) / 2;
    mainWindow.move(x, y);
    mainWindow.show();

    return a.exec();
}
