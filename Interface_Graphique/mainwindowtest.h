#ifndef MAINWINDOWTEST_H
#define MAINWINDOWTEST_H

#include <QMainWindow>

#include "Solver/src/geomap.h"
#include "Solver/src/geoserializer/xlsserializer.h"
#include "Solver/src/geoserializer/csvserializer.h"
#include "stationmodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowTest; }
QT_END_NAMESPACE

class MainWindowTest : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindowTest(QWidget *parent = nullptr);
    ~MainWindowTest();

private:
    Ui::MainWindowTest *ui;

    QString filePath;
    GeoMap geoMap;

    StationModel m_model;

public slots:
    void openFileDialog();
    void saveFileDialog();
    void updateGeoMapFromFile();
    void updateExcelTableFromGeoMap();
};

#endif // MAINWINDOWTEST_H
