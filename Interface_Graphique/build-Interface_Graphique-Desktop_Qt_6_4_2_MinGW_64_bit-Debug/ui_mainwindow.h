/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.4.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QSplitter *splitter;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_18;
    QLabel *label_choisir_algo;
    QComboBox *comboBox_choisir_algo;
    QGridLayout *gridLayout_2;
    QLabel *label_11;
    QLineEdit *lineEdit_heurDepart;
    QLabel *label_12;
    QLineEdit *lineEdit_levelDuJour;
    QLabel *label_13;
    QLineEdit *lineEdit_tombeDeLaNiut;
    QLabel *label_14;
    QDoubleSpinBox *doubleSpinBox_recharge;
    QLabel *label_15;
    QHBoxLayout *horizontalLayout_5;
    QFormLayout *formLayout_2;
    QLabel *label__depart_OACI;
    QLabel *label_depart_statut;
    QLabel *label_depart_VFRnuit;
    QLabel *label_depart_essence;
    QLabel *label_1;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QComboBox *comboBox_depart;
    QLabel *label_depart;
    QFormLayout *formLayout;
    QLabel *label_arrivee;
    QComboBox *comboBox_arrivee;
    QLabel *label_5;
    QLabel *label_arrivee_OACI;
    QLabel *label_6;
    QLabel *label_arrivee_statut;
    QLabel *label_7;
    QLabel *label_arrivee_VFRnuit;
    QLabel *label_8;
    QLabel *label_arrivee_essence;
    QCheckBox *checkBox_boucle;
    QGridLayout *gridLayout;
    QLabel *label_vitesse;
    QDoubleSpinBox *doubleSpinBox_vitesse;
    QLabel *label;
    QLabel *label_capacite;
    QDoubleSpinBox *doubleSpinBox_capacite;
    QLabel *label_9;
    QLabel *label_consommation;
    QDoubleSpinBox *doubleSpinBox_consommation;
    QLabel *label_10;
    QListWidget *listWidget_2;
    QListWidget *listWidget_3;
    QPushButton *pushButton_calculer;
    QListWidget *listWidget_1;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(922, 528);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName("splitter");
        splitter->setOrientation(Qt::Horizontal);
        splitter->setOpaqueResize(true);
        splitter->setHandleWidth(3);
        splitter->setChildrenCollapsible(true);
        groupBox = new QGroupBox(splitter);
        groupBox->setObjectName("groupBox");
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy1);
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        horizontalLayout_18 = new QHBoxLayout();
        horizontalLayout_18->setObjectName("horizontalLayout_18");
        horizontalLayout_18->setContentsMargins(10, -1, 10, -1);
        label_choisir_algo = new QLabel(groupBox);
        label_choisir_algo->setObjectName("label_choisir_algo");

        horizontalLayout_18->addWidget(label_choisir_algo);

        comboBox_choisir_algo = new QComboBox(groupBox);
        comboBox_choisir_algo->addItem(QString());
        comboBox_choisir_algo->addItem(QString::fromUtf8("TSP"));
        comboBox_choisir_algo->addItem(QString());
        comboBox_choisir_algo->setObjectName("comboBox_choisir_algo");
        comboBox_choisir_algo->setEditable(true);

        horizontalLayout_18->addWidget(comboBox_choisir_algo);


        verticalLayout->addLayout(horizontalLayout_18);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(5, -1, 5, -1);
        label_11 = new QLabel(groupBox);
        label_11->setObjectName("label_11");

        gridLayout_2->addWidget(label_11, 0, 0, 1, 1);

        lineEdit_heurDepart = new QLineEdit(groupBox);
        lineEdit_heurDepart->setObjectName("lineEdit_heurDepart");

        gridLayout_2->addWidget(lineEdit_heurDepart, 0, 1, 1, 2);

        label_12 = new QLabel(groupBox);
        label_12->setObjectName("label_12");

        gridLayout_2->addWidget(label_12, 1, 0, 1, 1);

        lineEdit_levelDuJour = new QLineEdit(groupBox);
        lineEdit_levelDuJour->setObjectName("lineEdit_levelDuJour");

        gridLayout_2->addWidget(lineEdit_levelDuJour, 1, 1, 1, 2);

        label_13 = new QLabel(groupBox);
        label_13->setObjectName("label_13");

        gridLayout_2->addWidget(label_13, 2, 0, 1, 1);

        lineEdit_tombeDeLaNiut = new QLineEdit(groupBox);
        lineEdit_tombeDeLaNiut->setObjectName("lineEdit_tombeDeLaNiut");

        gridLayout_2->addWidget(lineEdit_tombeDeLaNiut, 2, 1, 1, 2);

        label_14 = new QLabel(groupBox);
        label_14->setObjectName("label_14");

        gridLayout_2->addWidget(label_14, 3, 0, 1, 1);

        doubleSpinBox_recharge = new QDoubleSpinBox(groupBox);
        doubleSpinBox_recharge->setObjectName("doubleSpinBox_recharge");
        doubleSpinBox_recharge->setMaximum(9999999.990000000223517);

        gridLayout_2->addWidget(doubleSpinBox_recharge, 3, 1, 1, 1);

        label_15 = new QLabel(groupBox);
        label_15->setObjectName("label_15");

        gridLayout_2->addWidget(label_15, 3, 2, 1, 1);


        verticalLayout->addLayout(gridLayout_2);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName("formLayout_2");
        formLayout_2->setContentsMargins(10, 5, 10, 5);
        label__depart_OACI = new QLabel(groupBox);
        label__depart_OACI->setObjectName("label__depart_OACI");

        formLayout_2->setWidget(2, QFormLayout::FieldRole, label__depart_OACI);

        label_depart_statut = new QLabel(groupBox);
        label_depart_statut->setObjectName("label_depart_statut");

        formLayout_2->setWidget(3, QFormLayout::FieldRole, label_depart_statut);

        label_depart_VFRnuit = new QLabel(groupBox);
        label_depart_VFRnuit->setObjectName("label_depart_VFRnuit");

        formLayout_2->setWidget(4, QFormLayout::FieldRole, label_depart_VFRnuit);

        label_depart_essence = new QLabel(groupBox);
        label_depart_essence->setObjectName("label_depart_essence");

        formLayout_2->setWidget(5, QFormLayout::FieldRole, label_depart_essence);

        label_1 = new QLabel(groupBox);
        label_1->setObjectName("label_1");

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");

        formLayout_2->setWidget(3, QFormLayout::LabelRole, label_2);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName("label_3");

        formLayout_2->setWidget(4, QFormLayout::LabelRole, label_3);

        label_4 = new QLabel(groupBox);
        label_4->setObjectName("label_4");

        formLayout_2->setWidget(5, QFormLayout::LabelRole, label_4);

        comboBox_depart = new QComboBox(groupBox);
        comboBox_depart->setObjectName("comboBox_depart");
        comboBox_depart->setEditable(true);

        formLayout_2->setWidget(1, QFormLayout::SpanningRole, comboBox_depart);

        label_depart = new QLabel(groupBox);
        label_depart->setObjectName("label_depart");

        formLayout_2->setWidget(0, QFormLayout::SpanningRole, label_depart);


        horizontalLayout_5->addLayout(formLayout_2);

        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");
        formLayout->setContentsMargins(10, 5, 10, 5);
        label_arrivee = new QLabel(groupBox);
        label_arrivee->setObjectName("label_arrivee");

        formLayout->setWidget(0, QFormLayout::SpanningRole, label_arrivee);

        comboBox_arrivee = new QComboBox(groupBox);
        comboBox_arrivee->setObjectName("comboBox_arrivee");
        comboBox_arrivee->setEditable(true);

        formLayout->setWidget(1, QFormLayout::SpanningRole, comboBox_arrivee);

        label_5 = new QLabel(groupBox);
        label_5->setObjectName("label_5");

        formLayout->setWidget(2, QFormLayout::LabelRole, label_5);

        label_arrivee_OACI = new QLabel(groupBox);
        label_arrivee_OACI->setObjectName("label_arrivee_OACI");

        formLayout->setWidget(2, QFormLayout::FieldRole, label_arrivee_OACI);

        label_6 = new QLabel(groupBox);
        label_6->setObjectName("label_6");

        formLayout->setWidget(3, QFormLayout::LabelRole, label_6);

        label_arrivee_statut = new QLabel(groupBox);
        label_arrivee_statut->setObjectName("label_arrivee_statut");

        formLayout->setWidget(3, QFormLayout::FieldRole, label_arrivee_statut);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName("label_7");

        formLayout->setWidget(4, QFormLayout::LabelRole, label_7);

        label_arrivee_VFRnuit = new QLabel(groupBox);
        label_arrivee_VFRnuit->setObjectName("label_arrivee_VFRnuit");

        formLayout->setWidget(4, QFormLayout::FieldRole, label_arrivee_VFRnuit);

        label_8 = new QLabel(groupBox);
        label_8->setObjectName("label_8");

        formLayout->setWidget(5, QFormLayout::LabelRole, label_8);

        label_arrivee_essence = new QLabel(groupBox);
        label_arrivee_essence->setObjectName("label_arrivee_essence");

        formLayout->setWidget(5, QFormLayout::FieldRole, label_arrivee_essence);


        horizontalLayout_5->addLayout(formLayout);

        checkBox_boucle = new QCheckBox(groupBox);
        checkBox_boucle->setObjectName("checkBox_boucle");
        checkBox_boucle->setEnabled(true);

        horizontalLayout_5->addWidget(checkBox_boucle);


        verticalLayout->addLayout(horizontalLayout_5);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");
        label_vitesse = new QLabel(groupBox);
        label_vitesse->setObjectName("label_vitesse");
        sizePolicy.setHeightForWidth(label_vitesse->sizePolicy().hasHeightForWidth());
        label_vitesse->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_vitesse, 0, 0, 1, 1);

        doubleSpinBox_vitesse = new QDoubleSpinBox(groupBox);
        doubleSpinBox_vitesse->setObjectName("doubleSpinBox_vitesse");
        doubleSpinBox_vitesse->setMaximum(9999999.990000000223517);

        gridLayout->addWidget(doubleSpinBox_vitesse, 0, 1, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 2, 1, 1);

        label_capacite = new QLabel(groupBox);
        label_capacite->setObjectName("label_capacite");
        sizePolicy.setHeightForWidth(label_capacite->sizePolicy().hasHeightForWidth());
        label_capacite->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_capacite, 1, 0, 1, 1);

        doubleSpinBox_capacite = new QDoubleSpinBox(groupBox);
        doubleSpinBox_capacite->setObjectName("doubleSpinBox_capacite");
        doubleSpinBox_capacite->setMaximum(9999999.990000000223517);

        gridLayout->addWidget(doubleSpinBox_capacite, 1, 1, 1, 1);

        label_9 = new QLabel(groupBox);
        label_9->setObjectName("label_9");
        sizePolicy.setHeightForWidth(label_9->sizePolicy().hasHeightForWidth());
        label_9->setSizePolicy(sizePolicy);

        gridLayout->addWidget(label_9, 1, 2, 1, 1);

        label_consommation = new QLabel(groupBox);
        label_consommation->setObjectName("label_consommation");

        gridLayout->addWidget(label_consommation, 2, 0, 1, 1);

        doubleSpinBox_consommation = new QDoubleSpinBox(groupBox);
        doubleSpinBox_consommation->setObjectName("doubleSpinBox_consommation");
        doubleSpinBox_consommation->setMaximum(9999999.990000000223517);

        gridLayout->addWidget(doubleSpinBox_consommation, 2, 1, 1, 1);

        label_10 = new QLabel(groupBox);
        label_10->setObjectName("label_10");

        gridLayout->addWidget(label_10, 2, 2, 1, 1);


        verticalLayout->addLayout(gridLayout);

        listWidget_2 = new QListWidget(groupBox);
        listWidget_2->setObjectName("listWidget_2");

        verticalLayout->addWidget(listWidget_2);

        listWidget_3 = new QListWidget(groupBox);
        listWidget_3->setObjectName("listWidget_3");

        verticalLayout->addWidget(listWidget_3);

        pushButton_calculer = new QPushButton(groupBox);
        pushButton_calculer->setObjectName("pushButton_calculer");
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(pushButton_calculer->sizePolicy().hasHeightForWidth());
        pushButton_calculer->setSizePolicy(sizePolicy2);

        verticalLayout->addWidget(pushButton_calculer);

        splitter->addWidget(groupBox);

        horizontalLayout->addWidget(splitter);

        listWidget_1 = new QListWidget(centralwidget);
        listWidget_1->setObjectName("listWidget_1");

        horizontalLayout->addWidget(listWidget_1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 922, 17));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        groupBox->setTitle(QString());
        label_choisir_algo->setText(QCoreApplication::translate("MainWindow", "choisir l'algorithme", nullptr));
        comboBox_choisir_algo->setItemText(0, QCoreApplication::translate("MainWindow", "Choisissez", nullptr));
        comboBox_choisir_algo->setItemText(2, QCoreApplication::translate("MainWindow", "Breitling", nullptr));

        comboBox_choisir_algo->setCurrentText(QCoreApplication::translate("MainWindow", "Choisissez", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "heure d\303\251part", nullptr));
        label_12->setText(QCoreApplication::translate("MainWindow", "level du jour", nullptr));
        label_13->setText(QCoreApplication::translate("MainWindow", "tomb\303\251 de la nuit", nullptr));
        label_14->setText(QCoreApplication::translate("MainWindow", "tps de recharge", nullptr));
        label_15->setText(QCoreApplication::translate("MainWindow", "min", nullptr));
        label__depart_OACI->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_depart_statut->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_depart_VFRnuit->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_depart_essence->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_1->setText(QCoreApplication::translate("MainWindow", "Code OACI", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Statut", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "VFR nuit", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Essence", nullptr));
        label_depart->setText(QCoreApplication::translate("MainWindow", "D\303\251part: ", nullptr));
        label_arrivee->setText(QCoreApplication::translate("MainWindow", "Arriv\303\251e: ", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "Code OACI", nullptr));
        label_arrivee_OACI->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Statut", nullptr));
        label_arrivee_statut->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "VFR nuit", nullptr));
        label_arrivee_VFRnuit->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Essence", nullptr));
        label_arrivee_essence->setText(QCoreApplication::translate("MainWindow", "-----", nullptr));
        checkBox_boucle->setText(QCoreApplication::translate("MainWindow", "boucle", nullptr));
        label_vitesse->setText(QCoreApplication::translate("MainWindow", "vitesse: ", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "mm/h", nullptr));
        label_capacite->setText(QCoreApplication::translate("MainWindow", "capacit\303\251: ", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "L", nullptr));
        label_consommation->setText(QCoreApplication::translate("MainWindow", "consommation: ", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "L/h", nullptr));
        pushButton_calculer->setText(QCoreApplication::translate("MainWindow", "Calculer", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
