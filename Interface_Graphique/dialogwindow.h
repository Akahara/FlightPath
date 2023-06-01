#ifndef DIALOGWINDOW_H
#define DIALOGWINDOW_H

#include <QDialog>

#include "Solver/src/pathsolver.h"

QT_BEGIN_NAMESPACE
    namespace Ui { class DialogWindow; }
QT_END_NAMESPACE

class DialogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit DialogWindow(SolverRuntime *runtime, ProblemPath *finalPath, ProblemMap *originalMap, QWidget *parent = nullptr);
    ~DialogWindow();

    void startProgress();

private slots:
    void updateProgress();
    void cancelSolver();
    void onSolverFinished();
    void saveFlightMapToFile();
    void saveMapToFile();

private:
    Ui::DialogWindow *ui;
    ProblemMap       *m_originalMap;
    SolverRuntime    *m_solverRuntime;
    ProblemPath      *m_finalPath; // must not be read until the solver finished running
    bool              m_userForcedTermination = false;

signals:
    void progressUpdated();
    void solverFinishedRunning();
};

#endif // DIALOGWINDOW_H
