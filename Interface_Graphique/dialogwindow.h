#ifndef DIALOGWINDOW_H
#define DIALOGWINDOW_H

#include <QDialog>

#include "Solver/src/pathsolver.h"

QT_BEGIN_NAMESPACE
    namespace Ui { class DialogWindow; }
QT_END_NAMESPACE

struct SolverExecutionState {
    ProblemMap    *originalMap;
    SolverRuntime *solverRuntime;
    ProblemPath   *finalPath; // must not be read until the solver finished running
    bool           finishedExecution = false;
    bool           isTspInstance = false;
};

class DialogWindow : public QDialog
{
    Q_OBJECT

public:
    explicit DialogWindow(SolverExecutionState *sovlerState, QWidget *parent = nullptr);
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
    SolverExecutionState *m_solverState;

signals:
    void progressUpdated();
    void solverFinishedRunning();
};

#endif // DIALOGWINDOW_H
