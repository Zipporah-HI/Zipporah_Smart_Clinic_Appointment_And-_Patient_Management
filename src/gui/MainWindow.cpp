#include "gui/MainWindow.h"

#include "gui/ClinicTheme.h"
#include "utils/ClinicExceptions.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <filesystem>

namespace scapms::gui {

MainWindow::MainWindow(const std::string& dataDir, QWidget* parent)
    : QMainWindow(parent), dataDir_(dataDir) {
    setWindowTitle("SCAPMS — Smart Clinic Appointment & Patient Management System");
    resize(1340, 820);
    setStyleSheet(clinicStylesheet());

    initializeData();
    setupTabs();
    setupMenus();
    setupTimers();

    refreshAllPanels("SCAPMS ready. Loaded clinic records.");
    onReminderTick();
}

std::string MainWindow::patientsFilePath() const {
    return (std::filesystem::path(dataDir_) / "patients.csv").string();
}

std::string MainWindow::appointmentsFilePath() const {
    return (std::filesystem::path(dataDir_) / "appointments.csv").string();
}

void MainWindow::initializeData() {
    try {
        if (std::filesystem::exists(patientsFilePath())) {
            manager_.loadClinicData(patientsFilePath(), appointmentsFilePath());
            // Populate initial consultation queue on startup if clinic has patients
            const auto& patients = manager_.getDatabase().getPatients();
            for (std::size_t i = 0; i < patients.size() && i < 3; ++i) {
                if (patients[i]) {
                    manager_.addToWaitingQueue(patients[i]->getPatientId());
                }
            }
        } else {
            manager_.seedSampleClinicData();
            manager_.saveClinicData(patientsFilePath(), appointmentsFilePath());
        }
    } catch (...) {
        manager_.seedSampleClinicData();
    }
}

void MainWindow::setupTabs() {
    tabWidget_ = new QTabWidget(this);
    setCentralWidget(tabWidget_);

    dashboardPanel_ = new DashboardPanel(manager_, this);
    patientPanel_ = new PatientPanel(manager_, this);
    appointmentPanel_ = new AppointmentPanel(manager_, this);
    queuePanel_ = new QueuePanel(manager_, this);
    searchPanel_ = new SearchPanel(manager_, this);
    adminPanel_ = new AdminPanel(manager_, this);

    tabWidget_->addTab(dashboardPanel_, "Dashboard");
    tabWidget_->addTab(patientPanel_, "Patient Records");
    tabWidget_->addTab(appointmentPanel_, "Appointments");
    tabWidget_->addTab(queuePanel_, "Waiting Queue");
    tabWidget_->addTab(searchPanel_, "Search && Sort");
    tabWidget_->addTab(adminPanel_, "Reports && Admin");

    connect(dashboardPanel_, &DashboardPanel::navigateToTab, this, &MainWindow::selectTab);
    connect(patientPanel_, &PatientPanel::dataChanged, this, &MainWindow::refreshAllPanels);
    connect(appointmentPanel_, &AppointmentPanel::dataChanged, this, &MainWindow::refreshAllPanels);
    connect(queuePanel_, &QueuePanel::dataChanged, this, &MainWindow::refreshAllPanels);
    connect(searchPanel_, &SearchPanel::dataChanged, this, &MainWindow::refreshAllPanels);
    connect(adminPanel_, &AdminPanel::dataChanged, this, &MainWindow::refreshAllPanels);
    connect(adminPanel_, &AdminPanel::requestSaveDialog, this, &MainWindow::onFileSaveAs);
    connect(adminPanel_, &AdminPanel::requestLoadDialog, this, &MainWindow::onFileLoadFrom);

    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int) {
        refreshAllPanels();
    });
}

void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* newAct = fileMenu->addAction("&New Clinic Session");
    newAct->setShortcut(QKeySequence::New);
    newAct->setToolTip("Clear current records and start a fresh clinic database.");
    connect(newAct, &QAction::triggered, this, &MainWindow::onFileNew);

    auto* saveAct = fileMenu->addAction("&Save Data");
    saveAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    saveAct->setToolTip("Save patients and appointments to default CSV files (Ctrl+S).");
    connect(saveAct, &QAction::triggered, this, &MainWindow::onFileSave);

    auto* loadAct = fileMenu->addAction("&Load Saved Data");
    loadAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    loadAct->setToolTip("Reload patients and appointments from saved CSV files (Ctrl+O).");
    connect(loadAct, &QAction::triggered, this, &MainWindow::onFileLoad);

    fileMenu->addSeparator();

    auto* exportAct = fileMenu->addAction("&Export Patients CSV...");
    connect(exportAct, &QAction::triggered, this, &MainWindow::onFileSaveAs);

    auto* importAct = fileMenu->addAction("&Import Patients CSV...");
    connect(importAct, &QAction::triggered, this, &MainWindow::onFileLoadFrom);

    auto* seedAct = fileMenu->addAction("Restore &Sample University Clinic Data");
    connect(seedAct, &QAction::triggered, this, &MainWindow::onLoadSampleData);

    fileMenu->addSeparator();

    auto* exitAct = fileMenu->addAction("E&xit");
    exitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // Edit menu
    auto* editMenu = menuBar()->addMenu("&Edit");
    auto* undoAct = editMenu->addAction("&Undo Last Administrative Operation");
    undoAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    connect(undoAct, &QAction::triggered, this, &MainWindow::onUndoLastAction);

    auto* searchAct = editMenu->addAction("&Find Patient...");
    searchAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
    connect(searchAct, &QAction::triggered, this, &MainWindow::onFocusSearch);

    // View menu
    auto* viewMenu = menuBar()->addMenu("&View");
    const QStringList tabNames = {
        "Dashboard", "Patient Records", "Appointments", "Waiting Queue", "Search & Sort", "Reports & Admin"
    };
    for (int i = 0; i < tabNames.size(); ++i) {
        auto* viewAct = viewMenu->addAction(tabNames[i]);
        connect(viewAct, &QAction::triggered, this, [this, i]() { selectTab(i); });
    }

    // Help menu
    auto* helpMenu = menuBar()->addMenu("&Help");
    auto* aboutAct = helpMenu->addAction("&About SCAPMS");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::onShowAbout);

    statusBar()->showMessage("Ready");
}

void MainWindow::setupTimers() {
    secondTimer_ = new QTimer(this);
    connect(secondTimer_, &QTimer::timeout, this, &MainWindow::onSecondTick);
    secondTimer_->start(1000);

    reminderTimer_ = new QTimer(this);
    connect(reminderTimer_, &QTimer::timeout, this, &MainWindow::onReminderTick);
    reminderTimer_->start(10000);
}

void MainWindow::selectTab(int index) {
    if (tabWidget_ && index >= 0 && index < tabWidget_->count()) {
        tabWidget_->setCurrentIndex(index);
    }
}

void MainWindow::refreshAllPanels(const QString& statusMessage) {
    dashboardPanel_->refreshData();
    patientPanel_->refreshTable();
    appointmentPanel_->refreshData();
    queuePanel_->refreshData();
    searchPanel_->refreshAllRecords();
    adminPanel_->refreshData();

    if (!statusMessage.isEmpty()) {
        statusBar()->showMessage(statusMessage, 8000);
    }
}

void MainWindow::onFileNew() {
    const auto reply = QMessageBox::question(
        this,
        "New Clinic Session",
        "Clear all current in-memory patients, appointments, and queues?");
    if (reply != QMessageBox::Yes) {
        return;
    }
    manager_.startNewSession();
    refreshAllPanels("Started a new empty clinic session.");
}

void MainWindow::onFileSave() {
    try {
        manager_.saveClinicData(patientsFilePath(), appointmentsFilePath());
        statusBar()->showMessage(
            QString("Saved clinic data to %1").arg(QString::fromStdString(dataDir_)), 6000);
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Save Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to save clinic data.");
    }
}

void MainWindow::onFileLoad() {
    try {
        manager_.loadClinicData(patientsFilePath(), appointmentsFilePath());
        refreshAllPanels("Reloaded clinic data from CSV files.");
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Load Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to load clinic data.");
    }
}

void MainWindow::onFileSaveAs() {
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Export Patients CSV",
        "patients_export.csv",
        "CSV Files (*.csv);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }
    try {
        const std::filesystem::path pPath(path.toStdString());
        const std::string apptsPath =
            (pPath.parent_path() / (pPath.stem().string() + "_appointments.csv")).string();
        manager_.saveClinicData(path.toStdString(), apptsPath);
        statusBar()->showMessage(QString("Exported clinic CSV bundle to %1").arg(path), 6000);
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Export Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Export failed.");
    }
}

void MainWindow::onFileLoadFrom() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        "Import Patients CSV",
        QString::fromStdString(dataDir_),
        "CSV Files (*.csv);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }
    try {
        const std::filesystem::path pPath(path.toStdString());
        const std::string apptsPath = (pPath.parent_path() / "appointments.csv").string();
        manager_.loadClinicData(path.toStdString(), apptsPath);
        refreshAllPanels(QString("Imported clinic records from %1").arg(path));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Import Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Import failed.");
    }
}

void MainWindow::onLoadSampleData() {
    manager_.seedSampleClinicData();
    refreshAllPanels("Restored sample university clinic records.");
}

void MainWindow::onUndoLastAction() {
    try {
        const std::string msg = manager_.undoLastOperation();
        refreshAllPanels(QString::fromStdString(msg));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::information(this, "Undo Stack", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Undo operation failed.");
    }
}

void MainWindow::onFocusSearch() {
    selectTab(4);
    searchPanel_->focusSearchInput();
}

void MainWindow::onShowAbout() {
    QMessageBox::about(
        this,
        "About SCAPMS",
        "<h3>Smart Clinic Appointment & Patient Management System (SCAPMS)</h3>"
        "<p>COS202 Programming II Lab Assessment Project (C++17 / Qt 6).</p>"
        "<ul>"
        "<li><b>OOP:</b> Abstract <code>PatientRecord</code>, <code>Appointable</code> interface, "
        "3 subclasses (<code>GeneralPatient</code>, <code>EmergencyPatient</code>, <code>FollowUpPatient</code>).</li>"
        "<li><b>Data Structures:</b> <code>std::vector</code>, <code>std::queue</code>, "
        "<code>std::stack</code> (Undo), and fixed-size <code>PatientRecord*[10]</code> array cache.</li>"
        "<li><b>Manual Algorithms:</b> Linear Search, Binary Search, Recursive Name Search, "
        "Selection Sort, Insertion Sort, and Merge Sort.</li>"
        "</ul>"
        "<p>Shortcuts: Ctrl+S (Save), Ctrl+O (Load), Ctrl+F (Search), Ctrl+Z (Undo), Ctrl+Q (Exit).</p>");
}

void MainWindow::onSecondTick() {
    queuePanel_->updateWaitingTimers();
    const auto queueSnapshot = manager_.getDatabase().getQueueSnapshot();
    const auto pending = manager_.getAppointmentController().getUpcomingPendingAppointments();
    QString alertText;
    if (!queueSnapshot.empty()) {
        const auto* nextP = manager_.getDatabase().findPatientRaw(queueSnapshot.front());
        alertText = QString("Queue Alert: %1 patient(s) waiting — Next up: %2 (%3) | Pending Appointments: %4")
                        .arg(queueSnapshot.size())
                        .arg(nextP ? QString::fromStdString(nextP->getName()) : QString::fromStdString(queueSnapshot.front()))
                        .arg(nextP ? QString::fromStdString(nextP->getPriorityLabel()) : "Normal")
                        .arg(pending.size());
    } else {
        alertText = QString("Reminder: %1 pending clinic appointment(s) scheduled. Waiting room is currently clear.")
                        .arg(pending.size());
    }
    dashboardPanel_->updateAlertBanner(alertText);
}

void MainWindow::onReminderTick() {
    const auto pending = manager_.getAppointmentController().getUpcomingPendingAppointments();
    const auto queueSize = manager_.getDatabase().getQueueSize();
    statusBar()->showMessage(
        QString("Clinic Monitor: %1 waiting in queue | %2 pending appointment(s)")
            .arg(queueSize)
            .arg(pending.size()),
        5000);
}

} // namespace scapms::gui
