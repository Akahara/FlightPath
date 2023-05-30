#ifndef MAINWINDOWTEST_H
#define MAINWINDOWTEST_H

#include <QMainWindow>

#include "Solver/src/geomap.h"
#include "Solver/src/geoserializer/xlsserializer.h"
#include "Solver/src/geoserializer/csvserializer.h"
#include "fuelmodel.h"
#include "stationmodel.h"
#include "nightflightmodel.h"
#include "statusmodel.h"

#define TSP_INDEX 0
#define BREITLING_INDEX 1

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
    StationModel m_excelModel;
    FuelModel m_fuelModel;
    StationStatusModel m_statusModel;
    NightFlightModel m_nightFlightModel;

    template<typename AttributeGetter>
    void repopulateFilterMap(QMap<std::string, bool> &filterMap, AttributeGetter getter);

public slots:
    void openFileDialog();
    void saveFileDialog();
    void updateGeoMapFromFile();
    void updateExcelTableFromGeoMap();
    void clickOnBoucle(int checkState);
    void clickOnAlgoComboBox(int index);
    void updateComboBoxDepArr();
    void updateDepArrInfos();
    void checkDepArrBoucleValidity();
    void updateFilterViews();
    void excelTableViewChanged();
};

#endif // MAINWINDOWTEST_H
