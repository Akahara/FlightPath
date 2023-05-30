#ifndef DIALOGWINDOW_H
#define DIALOGWINDOW_H

#include <QDialog>

QT_BEGIN_NAMESPACE
    namespace Ui { class DialogWindow; }
QT_END_NAMESPACE

class DialogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit DialogWindow(QWidget *parent = nullptr);
    ~DialogWindow();

private:
    Ui::DialogWindow *ui;
};

#endif // DIALOGWINDOW_H
