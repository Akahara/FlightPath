#include "mainwindowtest.h"
#include "dialogwindow.h"
#include "ui_mainwindowtest.h"

#include <algorithm>
#include <thread>
#include <memory>
#include <QFileDialog>

#include "Solver/src/pathsolver.h"
#include "Solver/src/tsp/tsp_nearest_multistart_opt.h"
#include "Solver/src/breitling/breitlingnatural.h"
#include "Solver/src/breitling/label_setting_breitling.h"

MainWindowTest::MainWindowTest(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowTest)
{
    ui->setupUi(this);

    // Set LabelNbThreads with max hardware threads
    ui->labelNbThreads->setText("(Le maximum semble être " + QString::number(std::thread::hardware_concurrency()) + " sur cette machine)");

    // Hide "heures widget" by default
    ui->heures->setVisible(false);

    // Set up excel table view
    ui->excelTable->setModel(&m_excelModel);
    ui->excelTable->verticalHeader()->hide();

    // Set up filter tables views
    ui->essenceViewTable->setModel(&m_fuelModel);
    ui->essenceViewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->StatutViewTable->setModel(&m_statusModel);
    ui->StatutViewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->VFRViewTable->setModel(&m_nightFlightModel);
    ui->VFRViewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);



    // Connect signals and slots
    connect(ui->actionOuvrir, SIGNAL(triggered()), this, SLOT(openFileDialog()));
    connect(ui->actionEnregistrer, SIGNAL(triggered()), this, SLOT(saveFileDialog()));
    connect(ui->boucle, SIGNAL(stateChanged(int)), this, SLOT(clickOnBoucle(int)));
    connect(ui->algoCombobox, SIGNAL(activated(int)), this, SLOT(clickOnAlgoComboBox(int)));
    connect(ui->depComboBox, SIGNAL(activated(int)), this, SLOT(updateDepArrInfos()));
    connect(ui->arrComboBox, SIGNAL(activated(int)), this, SLOT(updateDepArrInfos()));
    connect(ui->excelTable->model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(excelTableViewChanged()));
    connect(ui->computeButton, SIGNAL(clicked(bool)), this, SLOT(runSolver()));
}

MainWindowTest::~MainWindowTest()
{
    delete ui;
}


void MainWindowTest::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                    tr("(*.xls *.xlsx *.csv)"));
    if (fileName.isEmpty())
        return;

    this->filePath = fileName;
    updateGeoMapFromFile();
    updateComboBoxDepArr();
    updateDepArrInfos();
    updateFilterViews();
}

void MainWindowTest::saveFileDialog()
{
  if (filePath.isEmpty())
      return;

  GeoMap geoMap;
  geoMap.setStations(std::vector<Station>(m_excelModel.getStations().begin(), m_excelModel.getStations().end()));

  if (filePath.endsWith(".xls") || filePath.endsWith(".xlsx")) {
      XLSSerializer serializer;
      serializer.writeMap(geoMap, filePath.toStdString());
  } else if (filePath.endsWith(".csv")) {
      CSVSerializer serializer;
      serializer.writeMap(geoMap, filePath.toStdString());
  }
}

void MainWindowTest::updateGeoMapFromFile()
{
    if (filePath.isEmpty())
        return;

    GeoMap geoMap;

    if (filePath.endsWith(".xls") || filePath.endsWith(".xlsx")) {
        XLSSerializer serializer;
        geoMap = serializer.parseMap(filePath.toStdString());
    } else if (filePath.endsWith(".csv")) {
        CSVSerializer serializer;
        geoMap = serializer.parseMap(filePath.toStdString());
    } else {
      // unreachable
      assert(false);
    }

    QList<Station> stations;
    stations.reserve(geoMap.getStations().size());
    for(const Station &s : geoMap.getStations())
      stations.append(s);
    m_excelModel.setStations(stations);
}

void MainWindowTest::clickOnBoucle(int checkState) {
    checkDepArrBoucleValidity();
}

void MainWindowTest::clickOnAlgoComboBox(int index) {
    ui->heures->setVisible(index != TSP_INDEX);
    ui->boucle->setVisible(index == TSP_INDEX);
    ui->optWidget->setVisible(index == TSP_INDEX);
    ui->threadWidget->setVisible(index == TSP_INDEX);
    if(index == BREITLING_INDEX)
        ui->arriveeBoxWidget->setEnabled(true);
    checkDepArrBoucleValidity();
}

void MainWindowTest::updateComboBoxDepArr() {
    ui->depComboBox->clear();
    ui->arrComboBox->clear();

    ui->depComboBox->addItem("<aucun>");
    ui->arrComboBox->addItem("<aucun>");

    for (const Station &station : m_excelModel.getStations()) {
        if (!station.isExcluded()) {
            ui->depComboBox->addItem(QString::fromStdString(station.getName()));
            ui->arrComboBox->addItem(QString::fromStdString(station.getName()));
        }
    }
}

void MainWindowTest::updateDepArrInfos() {
    checkDepArrBoucleValidity();

    // Get the selected stations
    const Station *depStation = nullptr;
    const Station *arrStation = nullptr;

    for (const Station &station : m_excelModel.getStations()) {
        if (station.getName() == ui->depComboBox->currentText().toStdString()) {
            depStation = &station;
        }
        if (station.getName() == ui->arrComboBox->currentText().toStdString()) {
            arrStation = &station;
        }
    }

    // Update the infos
    if (depStation != nullptr) {
        ui->depOACILabel->setText(QString::fromStdString(depStation->getOACI()));
        ui->depStatutLabel->setText(QString::fromStdString(depStation->getStatus()));
        ui->depVFRLabel->setText(QString::fromStdString(depStation->getNightVFR()));
        ui->depEssenceLabel->setText(QString::fromStdString(depStation->getFuel()));
    } else {
        ui->depOACILabel->setText("----");
        ui->depStatutLabel->setText("----");
        ui->depVFRLabel->setText("----");
        ui->depEssenceLabel->setText("----");
    }

    if (arrStation != nullptr) {
        ui->arrOACILabel->setText(QString::fromStdString(arrStation->getOACI()));
        ui->arrStatutLabel->setText(QString::fromStdString(arrStation->getStatus()));
        ui->arrVFRLabel->setText(QString::fromStdString(arrStation->getNightVFR()));
        ui->arrEssenceLabel->setText(QString::fromStdString(arrStation->getFuel()));
    } else {
        ui->arrOACILabel->setText("----");
        ui->arrStatutLabel->setText("----");
        ui->arrVFRLabel->setText("----");
        ui->arrEssenceLabel->setText("----");
    }
}

void MainWindowTest::checkDepArrBoucleValidity() {
    if (ui->algoCombobox->currentIndex() == TSP_INDEX) {
        if (ui->depComboBox->currentText() == "<aucun>" || ui->boucle->checkState() == Qt::Checked)
            ui->arriveeBoxWidget->setEnabled(false);
        else
            ui->arriveeBoxWidget->setEnabled(true);
    }
}

template<typename AttributeGetter>
void MainWindowTest::repopulateFilterMap(QMap<std::string, bool> &filterMap, AttributeGetter getter) {
    // remove entries which are no longer used
    filterMap.removeIf([this,&getter](const std::pair<std::string,bool> &p) {
      return std::find_if(m_excelModel.getStations().begin(), m_excelModel.getStations().end(),
            [&p,&getter](const Station &s) { return getter(s) == p.first; }) == m_excelModel.getStations().end();
    });

    // add entries for new values (default to true, ie. do not remove when filtering)
    for (const Station &s : m_excelModel.getStations()) {
        if(!filterMap.contains(getter(s)))
            filterMap[getter(s)] = true;
    }
}

void MainWindowTest::updateFilterViews() {
    QMap<std::string, bool> fuelMap = m_fuelModel.getFuels();
    QMap<std::string, bool> statusMap = m_statusModel.getStatuses();
    QMap<std::string, bool> nightFlightMap = m_nightFlightModel.getStatuses();

    // delete unused entries and add new entries for new statuses
    repopulateFilterMap(fuelMap, [](const Station &s) { return s.getFuel(); });
    repopulateFilterMap(statusMap, [](const Station &s) { return s.getStatus(); });
    repopulateFilterMap(nightFlightMap, [](const Station &s) { return s.getNightVFR(); });

    // Update the models
    m_fuelModel.setFuels(fuelMap);
    m_statusModel.setStatuses(statusMap);
    m_nightFlightModel.setStatuses(nightFlightMap);
}

void MainWindowTest::excelTableViewChanged() {
    updateComboBoxDepArr();
    updateFilterViews();
    updateDepArrInfos();
}

static inline daytime_t controlsToDaytime(const QSpinBox *hoursSpinbox, const QSpinBox *minutesSpinBox) {
    return
        (hoursSpinbox==nullptr ? 0 : hoursSpinbox->value()) +
        (minutesSpinBox==nullptr ? 0 : minutesSpinBox->value()/60.f);
}

void MainWindowTest::runSolver() {
    static int progressPercentage = 0;
  std::thread runnerThread{[this]() {
      std::unique_ptr<PathSolver> solver;
      ProblemMap problemMap;

      for(const Station &station : m_excelModel.getStations()) {
          if(station.isExcluded() || m_statusModel.isExcluded(station))
              continue;
          bool isAccessibleAtNight = m_nightFlightModel.isAccessibleAtNight(station);
          bool canBeUsedToFuel = m_fuelModel.canBeUsedToFuel(station);
          problemMap.emplace_back(&station, isAccessibleAtNight, canBeUsedToFuel);
      }

      size_t departureStation = ui->depComboBox->currentIndex()-1; // will be -1 if no departure station is selected
      size_t targetStation = ui->arrComboBox->currentIndex()-1; // will be -1 if no target station is selected

      // generate the solver instance
      if(ui->algoCombobox->currentIndex() == TSP_INDEX) {
          unsigned int nbThread = ui->threadSpinBox->value();
          unsigned int optAlgo = ui->optComboBox->currentIndex() == 0 ? 0 : ui->optComboBox->currentIndex() + 1; // 0, 2, 3
          bool loop = ui->boucle->checkState() == Qt::Checked;
          const ProblemStation *startStation = departureStation == -1 ? nullptr : &problemMap[departureStation];
          const ProblemStation *endStation = targetStation == -1 ? nullptr : &problemMap[targetStation];
          solver = std::make_unique<TspNearestMultistartOptSolver>(nbThread, optAlgo, loop, startStation, endStation);
      } else {
          BreitlingData dataset;
          dataset.departureTime = controlsToDaytime(ui->depH, ui->depM);
          dataset.nauticalDaytime = controlsToDaytime(ui->couH, ui->couM);
          dataset.nauticalNighttime = controlsToDaytime(ui->levH, ui->levM);
          dataset.planeFuelCapacity = ui->capacite->value();
          dataset.planeFuelUsage = ui->consommation->value();
          dataset.planeSpeed = ui->vitesse->value();
          dataset.timeToRefuel = controlsToDaytime(nullptr, ui->tmpRav);
          dataset.departureStation = departureStation;
          dataset.targetStation = targetStation;
          solver = std::make_unique<NaturalBreitlingSolver>(dataset);
      }

      static bool stopFlag = false;

      solver->solveForPath(problemMap, &stopFlag, &progressPercentage);
  }};

  runnerThread.detach();

  static DialogWindow dialogWindow(this);
  dialogWindow.startProgress(&progressPercentage);

  dialogWindow.exec();
}

