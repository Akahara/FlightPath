#include "excelhelper.h"
#include <QAxObject>

void ExcelHelper::loadExcelToTableWidget(QTableWidget* tableWidget, QString filePath)
{
    QAxObject excel("Excel.Application");
    excel.setProperty("Visible", false); // Set the Excel application to be invisible

    QAxObject* workbooks = excel.querySubObject("Workbooks");
    QAxObject* workbook = workbooks->querySubObject("Open(const QString&)", filePath);

    QAxObject* sheets = workbook->querySubObject("Sheets");
    //int sheetCount = sheets->property("Count").toInt();

    // Load the data of the first worksheet into QTableWidget
    QAxObject* worksheet = sheets->querySubObject("Item(int)", 1);
    QAxObject* usedRange = worksheet->querySubObject("UsedRange");
    QAxObject* rows = usedRange->querySubObject("Rows");
    QAxObject* columns = usedRange->querySubObject("Columns");


    int rowCount = rows->property("Count").toInt();
    //int columnCount = columns->property("Count").toInt();
    int columnCount = 8;

    // Set the column count and header labels
    tableWidget->setColumnCount(columnCount);
    for (int column = 1; column <= columnCount; column++) {
        QAxObject* cell = worksheet->querySubObject("Cells(int,int)", 1, column);
        QVariant value = cell->property("Value");
        QString columnName = value.toString();
        tableWidget->setHorizontalHeaderItem(column - 1, new QTableWidgetItem(columnName));
        cell->clear();
        delete cell;
    }

    // Set the row count and cell values
    tableWidget->setRowCount(rowCount - 1);
    for (int row = 2; row <= rowCount; row++) {
        for (int column = 1; column <= columnCount; column++) {
            if(column==1)
            {
                QCheckBox* checkBox = new QCheckBox();
                checkBox->setChecked(false);
                checkBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                checkBox->setStyleSheet("QCheckBox { margin-left: 20px; margin-right: 20px; }"); // Adjust horizontal spacing of checkboxes

                QWidget* checkBoxContainer = new QWidget();
                QHBoxLayout* layout = new QHBoxLayout(checkBoxContainer);
                layout->setContentsMargins(0, 0, 0, 0);
                layout->addWidget(checkBox);
                layout->setAlignment(Qt::AlignCenter);

                tableWidget->setCellWidget(row - 2, column - 1, checkBoxContainer);
            }
            else
            {
                QAxObject* cell = worksheet->querySubObject("Cells(int,int)", row, column);
                QVariant value = cell->property("Value");
                tableWidget->setItem(row - 2, column - 1, new QTableWidgetItem(value.toString()));
                cell->clear();
                delete cell;
            }

        }
    }

    // Set header sort
    tableWidget->horizontalHeader()->setSortIndicatorShown(true);
    tableWidget->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

    // sort slot function
    QObject::connect(tableWidget->horizontalHeader(), &QHeaderView::sortIndicatorChanged,[=](int column, Qt::SortOrder order) {
        tableWidget->sortItems(column, order);
    });

    // Add search functionality for each column
    //addSortingAndSearchingToTableWidget(tableWidget);

    // autofit column width
    for (int column = 0; column < columnCount; column++) {
        int maxWidth = 0;
        for (int row = 0; row < rowCount; row++) {
            QTableWidgetItem* item = tableWidget->item(row, column);
            if (item != nullptr) {
                maxWidth = qMax(maxWidth, item->text().length());
            }
        }
        tableWidget->setColumnWidth(column, maxWidth * 7);
    }
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // remove blank lines
    for (int row = tableWidget->rowCount() - 1; row >= 0; row--) {
        bool isEmpty = true;
        for (int column = 0; column < tableWidget->columnCount(); column++) {
            QTableWidgetItem* item = tableWidget->item(row, column);
            if (item != nullptr && !item->text().isEmpty()) {
                isEmpty = false;
                break;
            }
        }
        if (isEmpty) {
            tableWidget->removeRow(row);
        }
    }

    // release resources
    delete columns;
    delete rows;
    delete usedRange;
    delete worksheet;
    delete sheets;
    workbook->dynamicCall("Close()");
    delete workbook;
    delete workbooks;
    excel.dynamicCall("Quit()");
}

void ExcelHelper::addSortingAndSearchingToTableWidget(QTableWidget* tableWidget)
{
//    // Get the horizontal title bar
//    QHeaderView* horizontalHeader = tableWidget->horizontalHeader();

//    // Create a dropdown box for each column header
//    for (int i = 0; i < horizontalHeader->count(); ++i) {
//        QComboBox* comboBox = new QComboBox();
//        comboBox->addItem("No Sorting");
//        comboBox->addItem("Ascending");
//        comboBox->addItem("Descending");
//        comboBox->addItem("Search");

//        // Add dropdowns next to column headers
//        QModelIndex index = tableWidget->model()->index(0, i);
//        horizontalHeader->setIndexWidget(index, comboBox);

//        // Associate the drop-down box with the column number corresponding to the column header
//        comboBox->setProperty("column", i);

//        // When the selection of the drop-down box changes, update the sorting or searching method of the table
//        QObject::connect(comboBox, QOverload<int>::of(&QComboBox::activated), [=](int index) {
//            int column = comboBox->property("column").toInt();
//            if (index == 0) {
//                // No Sorting
//                tableWidget->sortByColumn(-1, Qt::AscendingOrder);
//            } else if (index == 1) {
//                // Ascending
//                tableWidget->sortByColumn(column, Qt::AscendingOrder);
//            } else if (index == 2) {
//                // Descending
//                tableWidget->sortByColumn(column, Qt::DescendingOrder);
//            } else if (index == 3) {
//                // Search
//                QString searchText = QInputDialog::getText(tableWidget, "Search", "Enter search text:");
//                if (!searchText.isEmpty()) {
//                    QList<QTableWidgetItem*> items = tableWidget->findItems(searchText, Qt::MatchContains);
//                    if (!items.isEmpty()) {
//                        tableWidget->setCurrentItem(items.first());
//                    }
//                }
//            }
//        });
//    }
}

void ExcelHelper::displayColumnContentsInTableWidget(QTableWidget* tableWidget, QString filePath, QString columnName)
{
    QAxObject excel("Excel.Application");
    excel.setProperty("Visible", false);

    QAxObject* workbooks = excel.querySubObject("Workbooks");
    QAxObject* workbook = workbooks->querySubObject("Open(const QString&)", filePath);

    QAxObject* sheets = workbook->querySubObject("Sheets");
    QAxObject* worksheet = sheets->querySubObject("Item(int)", 1);
    QAxObject* usedRange = worksheet->querySubObject("UsedRange");
    QAxObject* columns = usedRange->querySubObject("Columns");
    QAxObject* rows = usedRange->querySubObject("Rows");

    int rowCount = rows->property("Count").toInt();
    int columnCount = columns->property("Count").toInt();
    int columnIndex = -1;

    // Find the index of the specified column
    for (int column = 1; column <= columnCount; ++column) {
        QAxObject* cell = worksheet->querySubObject("Cells(int,int)", 1, column);
        QVariant value = cell->property("Value");
        if (value.toString() == columnName) {
            columnIndex = column;
            break;
        }
    }

    QStringList data;
    for(int i = 2; i < rowCount; i++)
    {
        // Get the cell object
        QAxObject* cell = worksheet->querySubObject("Cells(int, int)", i, columnIndex);
        // Get the value of the cell
        QVariant value = cell->property("Value");
        // Checks if the value is valid and converts it to a string
        QString cellData;
        if (value.isValid() && !value.isNull()) {
            cellData = value.toString().trimmed(); // Use the trimmed() function to remove whitespace characters from both ends of a string
            if (!cellData.isEmpty()) { // Add a condition to skip empty strings
                bool containsCaseInsensitive = false;
                for (const QString& existingData : data) {
                    if (cellData.compare(existingData, Qt::CaseInsensitive) == 0) {
                        containsCaseInsensitive = true;
                        break;
                    }
                }
                if (!containsCaseInsensitive) {
                    data.append(cellData);
                }
            }
        }
        delete cell;
    }

    // Set the number of rows and columns of the table
    tableWidget->setRowCount(data.size());
    tableWidget->setColumnCount(2);

    for (int row = 0; row < data.size(); ++row)
    {
        QCheckBox* checkBox = new QCheckBox();
        checkBox->setChecked(false);
        checkBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        checkBox->setStyleSheet("QCheckBox { margin-left: 20px; margin-right: 20px; }"); // Adjust horizontal spacing of checkboxes

        QWidget* checkBoxContainer = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(checkBoxContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(checkBox);
        layout->setAlignment(Qt::AlignCenter);

        tableWidget->setCellWidget(row, 0, checkBoxContainer);

        const QString& value = data.at(row);
        // Create a QTableWidgetItem and set the value into the cell
        QTableWidgetItem* item = new QTableWidgetItem(value);
        tableWidget->setItem(row, 1, item);

        // shrink row height
        tableWidget->setRowHeight(row, 25);
    }

    tableWidget->setColumnWidth(0, 60); // Set the width of the first column to 60
    tableWidget->setColumnWidth(1, 150); // Set the width of the first column to 150

    //set column name
    QStringList headerLabels;
    headerLabels << "choice" << "tpyes";

    tableWidget->setColumnCount(headerLabels.size());
    tableWidget->setHorizontalHeaderLabels(headerLabels);

    // release resources
    delete columns;
    delete rows;
    delete usedRange;
    delete worksheet;
    delete sheets;
    workbook->dynamicCall("Close()");
    delete workbook;
    delete workbooks;
    excel.dynamicCall("Quit()");

}
