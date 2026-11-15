#ifndef SCAPMS_GUI_MAIN_WINDOW_H
#define SCAPMS_GUI_MAIN_WINDOW_H

#include "controller/ClinicManager.h"
#include "gui/AdminPanel.h"
#include "gui/AppointmentPanel.h"
#include "gui/DashboardPanel.h"
#include "gui/PatientPanel.h"
#include "gui/QueuePanel.h"
#include "gui/SearchPanel.h"

#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>

namespace scapms::gui {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const std::string& dataDir = "data", QWidget* parent = nullptr);

    void selectTab(int index);
    void refreshAllPanels(const QString& statusMessage = "");

public slots:
    void onFileNew();
    void onFileSave();
    void onFileLoad();
    void onFileSaveAs();
    void onFileLoadFrom();
    void onLoadSampleData();
    void onUndoLastAction();
    void onFocusSearch();
    void onShowAbout();
    void onSecondTick();
    void onReminderTick();

private:
    void setupMenus();
    void setupTabs();
    void setupTimers();
    void initializeData();

    std::string patientsFilePath() const;
    std::string appointmentsFilePath() const;

    std::string dataDir_;
    controller::ClinicManager manager_;

    QTabWidget* tabWidget_ = nullptr;
    DashboardPanel* dashboardPanel_ = nullptr;
    PatientPanel* patientPanel_ = nullptr;
    AppointmentPanel* appointmentPanel_ = nullptr;
    QueuePanel* queuePanel_ = nullptr;
    SearchPanel* searchPanel_ = nullptr;
    AdminPanel* adminPanel_ = nullptr;

    QTimer* secondTimer_ = nullptr;
    QTimer* reminderTimer_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_MAIN_WINDOW_H
