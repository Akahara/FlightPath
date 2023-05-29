#include "mainwindowtest.h"
#include "ui_mainwindowtest.h"

#include <QFileDialog>

MainWindowTest::MainWindowTest(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowTest)
{
    ui->setupUi(this);

    // Set up table view
    ui->excelTable->setModel(&m_model);
    ui->excelTable->verticalHeader()->hide();

    // Set max value for spin boxes
    ui->depH->setMaximum(23);
    ui->depM->setMaximum(59);
    ui->levH->setMaximum(23);
    ui->levM->setMaximum(59);
    ui->couH->setMaximum(23);
    ui->couM->setMaximum(59);

    // Connect signals and slots
    connect(ui->actionOuvrir, SIGNAL(triggered()), this, SLOT(openFileDialog()));
    connect(ui->actionEnregistrer, SIGNAL(triggered()), this, SLOT(saveFileDialog()));
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
        m_model.append(station);
    }
}

