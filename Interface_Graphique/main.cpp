#include "mainwindow.h"
using namespace std;
#include <QApplication>
#include <QFile>
#include <QDir>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //Set the app style sheet
    QString relativePath = "Adaptic.qss";   //Combinear.qss
    QString absolutePath = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(relativePath);

    QFile styleSheetFile(absolutePath);
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(styleSheetFile.readAll());
    a.setStyleSheet(styleSheet);

    MainWindow w;
    w.show();
    return a.exec();
}
