#include "mainwindowtest.h"
#include "ui_mainwindowtest.h"

#include <QFileDialog>
#include <algorithm>

MainWindowTest::MainWindowTest(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowTest)
{
    ui->setupUi(this);

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
    updateExcelTableFromGeoMap();
    updateComboBoxDepArr();
    updateDepArrInfos();
    updateFilterViews();
}

void MainWindowTest::saveFileDialog()
{
  if (filePath.isEmpty())
      return;
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
    if (filePath.isEmpty()) {
        return;
    } else if (filePath.endsWith(".xls") || filePath.endsWith(".xlsx")) {
        XLSSerializer serializer;
        geoMap = serializer.parseMap(filePath.toStdString());
    } else if (filePath.endsWith(".csv")) {
        CSVSerializer serializer;
        geoMap = serializer.parseMap(filePath.toStdString());
    }
}

void MainWindowTest::updateExcelTableFromGeoMap() {
    for (const Station &station : geoMap.getStations()) {
        m_excelModel.append(station);
    }
}

void MainWindowTest::clickOnBoucle(int checkState) {
    checkDepArrBoucleValidity();
}

void MainWindowTest::clickOnAlgoComboBox(int index) {
    ui->heures->setVisible(index != TSP_INDEX);
    ui->boucle->setVisible(index == TSP_INDEX);
    if(index == BREITLING_INDEX)
        ui->arriveeBoxWidget->setEnabled(true);
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

