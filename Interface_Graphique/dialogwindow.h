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

    void startProgress(int *progressPercentage);

private slots:
    void updateProgress(int progress);

private:
    Ui::DialogWindow *ui;

signals:
    void progressUpdated(int progress);
};

#endif // DIALOGWINDOW_H
