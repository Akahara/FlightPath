#include "dialogwindow.h"
#include "ui_dialogwindow.h"

#include <thread>
#include <fstream>

#include <QThread>
#include <QFileDialog>

#include "Solver/src/geoserializer/csvserializer.h"
#include "Solver/src/geoserializer/xlsserializer.h"
#include "Solver/src/userinterface.h"

#define SLEEP_TIME 500

DialogWindow::DialogWindow(SolverExecutionState *solverState, QWidget *parent) :
    QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint),
    ui(new Ui::DialogWindow),
    m_solverState(solverState)
{
    ui->setupUi(this);

    ui->finishedWidget->setVisible(false);
    connect(this, &DialogWindow::progressUpdated, this, &DialogWindow::updateProgress);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &DialogWindow::cancelSolver);
    connect(this, &DialogWindow::solverFinishedRunning, this, &DialogWindow::onSolverFinished);
    connect(ui->closeBtn, &QPushButton::clicked, this, &DialogWindow::close);
    connect(ui->saveFlightMapBtn, &QPushButton::clicked, this, &DialogWindow::saveFlightMapToFile);
    connect(ui->saveMapBtn, &QPushButton::clicked, this, &DialogWindow::saveMapToFile);
}

DialogWindow::~DialogWindow()
{
    delete ui;
}

void DialogWindow::startProgress() {

    std::thread progressThread([this]() {
        while (!m_solverState->finishedExecution) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
            emit progressUpdated();
        }
        emit solverFinishedRunning();
    });

    progressThread.detach();
}

void DialogWindow::cancelSolver() {
    m_solverState->solverRuntime->userInterupted = true;
    ui->cancelBtn->setEnabled(false);
}

void DialogWindow::updateProgress() {
    int progressPercentage = (int)(m_solverState->solverRuntime->currentProgress*100);
    ui->progressBar->setValue(progressPercentage);
    if(m_solverState->solverRuntime->discoveredSolutionCount == 0)
        ui->progressBar->setFormat(QString::number(progressPercentage) + "%");
    else
        ui->progressBar->setFormat(QString::number(m_solverState->solverRuntime->foundSolutionCount) + " solutions découvertes, " + QString::number(m_solverState->solverRuntime->discoveredSolutionCount) + " restent à explorer");
    ui->progressBar->update();
}

void DialogWindow::onSolverFinished() {
  if(m_solverState->solverRuntime->userInterupted && m_solverState->solverRuntime->foundSolutionCount == 0) {
      close();
  } else {
      ui->finishedWidget->setVisible(true);
      ui->computingWidget->setVisible(false);
  }
}

void DialogWindow::saveFlightMapToFile() {
    QString filePath = QFileDialog::getSaveFileName(this, "Choisir une destination", QString(), "( *.xlsx *.xls *.csv)");
    if (filePath.isEmpty())
        return;

    std::unique_ptr<GeoSerializer> serializer;
    if(filePath.endsWith(".xls") || filePath.endsWith(".xlsx"))
        serializer = std::make_unique<XLSSerializer>();
    else
        serializer = std::make_unique<CSVSerializer>();

    Path path{};
    for(ProblemStation &s : *m_solverState->finalPath)
        path.getStations().push_back(s.getOriginalStation());
    serializer->writePath(filePath.toStdString(), path);
}

void DialogWindow::saveMapToFile() {
    QString filePath = QFileDialog::getSaveFileName(this, "Choisir une destination", QString(), "(*.kml)");
    if (filePath.isEmpty())
        return;

    std::ofstream outFile{ filePath.toStdString() };

    kml_export::writeHeader(outFile);
    kml_export::writeAllStationsLayer(outFile, *m_solverState->originalMap);
    kml_export::writeProblemStationsLayer(outFile, *m_solverState->originalMap); // TODO do not write problem stations for TSP
    kml_export::writePathLayer(outFile, *m_solverState->finalPath, "Chemin");
    kml_export::writeFooter(outFile);
}
