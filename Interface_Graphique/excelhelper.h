#ifndef EXCELHELPER_H
#define EXCELHELPER_H

#include <QtWidgets>

class ExcelHelper
{
public:
    static void loadExcelToTableWidget(QTableWidget* tableWidget, QString filePath);

    static void addSortingAndSearchingToTableWidget(QTableWidget* tableWidget);

    static void displayColumnContentsInTableWidget(QTableWidget* tableWidget, QString filePath, QString columnName);
};

#endif // EXCELHELPER_H
