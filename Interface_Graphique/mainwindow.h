#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QString filePath;
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
    void clickOnBoucle(int checkState);
    void updateFieldsVisibility();
    void updateComboBoxDepArr();
    void updateDepArrInfos();
    void checkDepArrBoucleValidity();
    void updateFilterViews();
    void excelTableViewChanged();
    void checkSolverCanBeRun();
    void runSolver();
};

#endif // MAINWINDOW_H
