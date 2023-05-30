#include "mainwindowtest.h"
#include "ui_mainwindowtest.h"

#include <QFileDialog>

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

    // Set up fuel table view
    ui->essenceViewTable->setModel(&m_fuelModel);
    ui->essenceViewTable->verticalHeader()->hide();
    ui->essenceViewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // Set max value for spin boxes
    ui->depH->setMaximum(23);
    ui->depM->setMaximum(59);
    ui->levH->setMaximum(23);
    ui->levM->setMaximum(59);
    ui->couH->setMaximum(23);
    ui->couM->setMaximum(59);
    ui->tmpRav->setMaximum(999);

    // Connect signals and slots
    connect(ui->actionOuvrir, SIGNAL(triggered()), this, SLOT(openFileDialog()));
    connect(ui->actionEnregistrer, SIGNAL(triggered()), this, SLOT(saveFileDialog()));
    connect(ui->boucle, SIGNAL(stateChanged(int)), this, SLOT(clickOnBoucle(int)));
    connect(ui->algoCombobox, SIGNAL(activated(int)), this, SLOT(clickOnAlgoComboBox(int)));
    connect(ui->depComboBox, SIGNAL(activated(int)), this, SLOT(updateDepArrInfos()));
    connect(ui->arrComboBox, SIGNAL(activated(int)), this, SLOT(updateDepArrInfos()));
    connect(ui->excelTable->model(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(excelTableViewChanged()));
}

MainWindowTest::~MainWindowTest()
{
    delete ui;
}


void MainWindowTest::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                    tr("(*.xls *.xlsx *.csv)"));
    if (!fileName.isEmpty()) {
        this->filePath = fileName;
    }

    updateGeoMapFromFile();
    updateExcelTableFromGeoMap();
    updateComboBoxDepArr();
    updateDepArrInfos();
    updateEssenceView();
}

void MainWindowTest::saveFileDialog()
{
    // TODO
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
    if (index == TSP_INDEX) {
        ui->heures->setVisible(false);
    } else if (index == BREITLING_INDEX) {
        ui->heures->setVisible(true);
    }
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
        if (ui->depComboBox->currentText() == "<aucun>") {
            ui->arriveeBoxWidget->setEnabled(false);
        }
        else if (ui->boucle->checkState() == Qt::Checked) {
            ui->arriveeBoxWidget->setEnabled(false);
        }
        else {
            ui->arriveeBoxWidget->setEnabled(true);
        }
    }
}

void MainWindowTest::updateEssenceView() {
    QMap<std::string, bool> fuelPresenceMap;
    QMap<std::string, bool> fuelMap = m_fuelModel.getFuels();
    
    // Fill with fuels from the map
    for (const Station &station : m_excelModel.getStations()) {
        fuelPresenceMap[station.getFuel()] = false;
    }

    // For all keys in the fuelPresenceMap
    for (const std::string &key : fuelPresenceMap.keys()) {
        // If not in the fuelMap, add it
        if (fuelMap.find(key) == fuelMap.end()) {
            fuelMap[key] = true;
        }
    }

    // For all keys in the fuelMap
    for (const std::string &key : fuelMap.keys()) {
        // If not in the fuelPresenceMap, remove it
        if (fuelPresenceMap.find(key) == fuelPresenceMap.end()) {
            fuelMap.remove(key);
        }
    }

    // Update the model
    m_fuelModel.setFuels(fuelMap);
}

void MainWindowTest::excelTableViewChanged() {
    updateComboBoxDepArr();
    updateEssenceView();
    updateDepArrInfos();
}

