#include "mainwindow.h"

#include <QApplication>
#include <QFile>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //Set the app style sheet
    QFile styleSheetFile("E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/Adaptic.qss");
    //QFile styleSheetFile("E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/Combinear.qss");
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(styleSheetFile.readAll());
    a.setStyleSheet(styleSheet);

    MainWindow w;
    w.show();
    return a.exec();
}
