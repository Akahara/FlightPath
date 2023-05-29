#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "excelhelper.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //ui->tableWidget->setContentsMargins(10,10,0,0);

    // Call the function in the ExcelHelper class to load the contents of the Excel file into the QTableWidget
    //ExcelHelper::loadExcelToTableWidget(ui->tableWidget, "E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/aerodromes.xlsx");

    //ExcelHelper::displayColumnContentsInTableWidget(ui->tableWidget_3, "E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/aerodromes.xlsx","VFR nuit");

    //ExcelHelper::displayColumnContentsInTableWidget(ui->tableWidget_2, "E:/001_FA/001_Tours/Cours/S8/Projet Collectif/interface graphique/Interface_Graphique/aerodromes.xlsx","Essence");


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comboBox_choisir_algo_currentIndexChanged(int index)
{
    if (index == 0)
    {
        ui->label_11->hide();
        ui->label_12->hide();
        ui->label_13->hide();
        ui->label_14->hide();
        ui->label_15->hide();
        ui->lineEdit_heurDepart->hide();
        ui->lineEdit_levelDuJour->hide();
        ui->lineEdit_tombeDeLaNiut->hide();
        ui->doubleSpinBox_recharge->hide();
        ui->checkBox_boucle->show();
    }
    else
    {
        ui->label_11->show();
        ui->label_12->show();
        ui->label_13->show();
        ui->label_14->show();
        ui->label_15->show();
        ui->lineEdit_heurDepart->show();
        ui->lineEdit_levelDuJour->show();
        ui->lineEdit_tombeDeLaNiut->show();
        ui->doubleSpinBox_recharge->show();
        ui->checkBox_boucle->hide();
    }
}


void MainWindow::on_checkBox_boucle_toggled(bool checked)
{
    if (checked) {
            ui->label_arrivee->hide();
            ui->label_5->hide();
            ui->label_6->hide();
            ui->label_7->hide();
            ui->label_8->hide();
            ui->comboBox_arrivee->hide();
            ui->label_arrivee_OACI->hide();
            ui->label_arrivee_statut->hide();
            ui->label_arrivee_VFRnuit->hide();
            ui->label_arrivee_essence->hide();

        } else {
            ui->label_arrivee->show();
            ui->label_5->show();
            ui->label_6->show();
            ui->label_7->show();
            ui->label_8->show();
            ui->comboBox_arrivee->show();
            ui->label_arrivee_OACI->show();
            ui->label_arrivee_statut->show();
            ui->label_arrivee_VFRnuit->show();
            ui->label_arrivee_essence->show();
        }
}

