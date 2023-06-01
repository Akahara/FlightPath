#include "dialogwindow.h"
#include "ui_dialogwindow.h"
#include <thread>
#include <QThread>

#define SLEEP_TIME 500

DialogWindow::DialogWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogWindow)
{
    ui->setupUi(this);

    ui->finishedWidget->setVisible(false);
    connect(this, &DialogWindow::progressUpdated, this, &DialogWindow::updateProgress);
}

DialogWindow::~DialogWindow()
{
    delete ui;
}

void DialogWindow::startProgress(int *progressPercentage) {

    std::thread progressThread([this, progressPercentage]() {
        while (*progressPercentage < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME));
            emit progressUpdated(*progressPercentage);
        }
    });

    progressThread.detach();
}

void DialogWindow::updateProgress(int progress) {
    ui->progressBar->setValue(progress);
    ui->progressBar->setFormat(QString::number(progress) + "%");
    ui->progressBar->update();
}
