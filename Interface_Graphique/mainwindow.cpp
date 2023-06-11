#include "mainwindow.h"
#include "dialogwindow.h"
#include "ui_mainwindow.h"

#include <algorithm>
#include <thread>
#include <memory>
#include <QFileDialog>
#include <QMessageBox>
#include "Solver/src/pathsolver.h"
#include "Solver/src/tsp/tsp_nearest_multistart_opt.h"
#include "Solver/src/breitling/breitlingnatural.h"
#include "Solver/src/breitling/label_setting_breitling.h"
#include "Solver/src/optimisation/optimisationSolver.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set LabelNbThreads with max hardware threads
    ui->labelNbThreads->setText("(Le maximum semble être " + QString::number(std::thread::hardware_concurrency()) + " sur cette machine)");

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
    connect(ui->algoCombobox, SIGNAL(activated(int)), this, SLOT(updateFieldsVisibility()));
    connect(ui->depComboBox, SIGNAL(activated(int)), this, SLOT(updateDepArrInfos()));
    connect(ui->arrComboBox, SIGNAL(activated(int)), this, SLOT(updateDepArrInfos()));
    connect(ui->excelTable->model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(excelTableViewChanged()));
    connect(ui->computeButton, SIGNAL(clicked(bool)), this, SLOT(runSolver()));
    connect(ui->algoCombobox, SIGNAL(activated(int)), this, SLOT(checkSolverCanBeRun()));
    connect(ui->depComboBox, SIGNAL(activated(int)), this, SLOT(checkSolverCanBeRun()));
    connect(ui->arrComboBox, SIGNAL(activated(int)), this, SLOT(checkSolverCanBeRun()));
    connect(ui->excelTable->model(), SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(checkSolverCanBeRun()));

    updateFieldsVisibility();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Ouvrir un fichier", QString(),
                                                    "(*.xls *.xlsx *.csv)");
    if (fileName.isEmpty())
        return;

    this->filePath = fileName;
    updateGeoMapFromFile();
    updateComboBoxDepArr();
    updateDepArrInfos();
    updateFilterViews();
}

void MainWindow::saveFileDialog()
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

void MainWindow::updateGeoMapFromFile()
{
    if (filePath.isEmpty())
        return;

    GeoMap geoMap;

    try {
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
    } catch (const std::runtime_error &e) {
        QMessageBox::warning(this, "Impossible de lire le fichier", e.what());
        return;
    }

    QList<Station> stations;
    stations.reserve(geoMap.getStations().size());
    for(const Station &s : geoMap.getStations())
      stations.append(s);
    m_excelModel.setStations(stations);
    checkSolverCanBeRun();
}

void MainWindow::clickOnBoucle(int checkState) {
    checkDepArrBoucleValidity();
}

void MainWindow::updateFieldsVisibility() {
    int index = ui->algoCombobox->currentIndex();
    ui->heures->setVisible(index == BREITLING_INDEX);
    ui->boucle->setVisible(index == TSP_INDEX);
    ui->optWidget->setVisible(index == TSP_INDEX);
    ui->threadWidget->setVisible(index == TSP_INDEX);
    ui->breitlingSolverSelection->setVisible(index == BREITLING_INDEX);
    ui->EssenceViewWidget->setEnabled(index == BREITLING_INDEX);
    ui->VFRViewWidget->setEnabled(index == BREITLING_INDEX);
    if(index == BREITLING_INDEX)
        ui->arriveeBoxWidget->setEnabled(true);
    checkDepArrBoucleValidity();
}

void MainWindow::updateComboBoxDepArr() {
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

void MainWindow::updateDepArrInfos() {
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

void MainWindow::checkDepArrBoucleValidity() {
    if (ui->algoCombobox->currentIndex() == TSP_INDEX) {
        if (ui->depComboBox->currentText() == "<aucun>" || ui->boucle->checkState() == Qt::Checked)
            ui->arriveeBoxWidget->setEnabled(false);
        else
            ui->arriveeBoxWidget->setEnabled(true);
    }
}

template<typename AttributeGetter>
void MainWindow::repopulateFilterMap(QMap<std::string, bool> &filterMap, AttributeGetter getter) {
    // remove entries which are no longer used
    filterMap.removeIf([this,&getter](const std::pair<std::string,bool> &p) {
      return std::find_if(m_excelModel.getStations().begin(), m_excelModel.getStations().end(),
            [&p,&getter](const Station &s) { return getter(s) == p.first; }) == m_excelModel.getStations().end();
    });

    // add entries for new values (default to true, ie. do not remove when filtering)
    for (const Station &s : m_excelModel.getStations()) {
        std::string clazz = getter(s);
        if(!filterMap.contains(clazz)) {
            bool defaultValue = clazz != "non";
            filterMap[clazz] = defaultValue;
        }
    }
}

void MainWindow::updateFilterViews() {
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

void MainWindow::excelTableViewChanged() {
    updateComboBoxDepArr();
    updateFilterViews();
    updateDepArrInfos();
}

void MainWindow::checkSolverCanBeRun()
{
    ui->computeButton->setEnabled(true);
    if(m_excelModel.getStations().isEmpty()) {
        ui->computeButton->setEnabled(false);
    } else if(ui->algoCombobox->currentIndex() == BREITLING_INDEX) {
        int departureStation = ui->depComboBox->currentIndex() - 1;
        int targetStation = ui->arrComboBox->currentIndex() - 1;
        switch(ui->breitlingSolverCombo->currentIndex()) {
        case 0 /* natural solver      */: ui->computeButton->setEnabled(departureStation >= 0 && targetStation >= 0 && targetStation != departureStation); return;
        case 1 /* label setting       */: ui->computeButton->setEnabled(departureStation >= 0 && targetStation != departureStation); return;
        case 2 /* optimization solver */: ui->computeButton->setEnabled(departureStation >= 0 && targetStation >= 0 && targetStation != departureStation); return;
        }
    }
}

static inline daytime_t controlsToDaytime(const QSpinBox *hoursSpinbox, const QSpinBox *minutesSpinBox) {
    return
        (hoursSpinbox==nullptr ? 0 : hoursSpinbox->value()) +
        (minutesSpinBox==nullptr ? 0 : minutesSpinBox->value()/60.f);
}

void MainWindow::runSolver() {
  SolverRuntime *runtime = new SolverRuntime{};
  ProblemPath *finalPath = new ProblemPath{};
  ProblemMap *problemMap = new ProblemMap{};
  SolverExecutionState state;
  state.finalPath = finalPath;
  state.originalMap = problemMap;
  state.solverRuntime = runtime;

  std::thread runnerThread{[this,&state,runtime,finalPath,problemMap]() {
      std::unique_ptr<PathSolver> solver;

      for(const Station &station : m_excelModel.getStations()) {
          if(station.isExcluded() || m_statusModel.isExcluded(station))
              continue;
          bool isAccessibleAtNight = m_nightFlightModel.isAccessibleAtNight(station);
          bool canBeUsedToFuel = m_fuelModel.canBeUsedToFuel(station);
          problemMap->emplace_back(&station, isAccessibleAtNight, canBeUsedToFuel);
      }

      int departureStation = ui->depComboBox->currentIndex()-1; // will be -1 if no departure station is selected
      int targetStation = ui->arrComboBox->currentIndex()-1; // will be -1 if no target station is selected

      // generate the solver instance
      if(ui->algoCombobox->currentIndex() == TSP_INDEX) {
          unsigned int nbThread = ui->threadSpinBox->value();
          unsigned int optAlgo = ui->optComboBox->currentIndex() == 0 ? 0 : ui->optComboBox->currentIndex() + 1; // 0, 2, 3
          bool loop = ui->boucle->checkState() == Qt::Checked;
          const ProblemStation *startStation = departureStation == -1 ? nullptr : &(*problemMap)[departureStation];
          const ProblemStation *endStation = targetStation == -1 ? nullptr : &(*problemMap)[targetStation];
          solver = std::make_unique<TspNearestMultistartOptSolver>(nbThread, optAlgo, loop, startStation, endStation);
          state.isTspInstance = true;
      } else {
          BreitlingData dataset;
          dataset.departureTime = controlsToDaytime(ui->depH, ui->depM);
          dataset.nauticalDaytime = controlsToDaytime(ui->levH, ui->levM);
          dataset.nauticalNighttime = controlsToDaytime(ui->couH, ui->couM);
          dataset.planeFuelCapacity = ui->capacite->value();
          dataset.planeFuelUsage = ui->consommation->value();
          dataset.planeSpeed = ui->vitesse->value();
          dataset.timeToRefuel = controlsToDaytime(nullptr, ui->tmpRav);
          dataset.departureStation = departureStation;
          dataset.targetStation = targetStation;
          switch(ui->breitlingSolverCombo->currentIndex()) {
          default:
          case 0: solver = std::make_unique<NaturalBreitlingSolver>(dataset); break;
          case 1: solver = std::make_unique<LabelSettingBreitlingSolver>(dataset); break;
          case 2: solver = std::make_unique<OptimisationSolver>(dataset); break;
          }
      }

      *finalPath = solver->solveForPath(*problemMap, runtime);
      state.finishedExecution = true; // tells the UI that the solver finished solving
  }};

  DialogWindow dialogWindow(&state, this);
  dialogWindow.startProgress();
  dialogWindow.exec();

  // suboptimal shared memory management
  delete runtime;
  delete finalPath;
  delete problemMap;
  runnerThread.join(); // the thread stopped already
}

