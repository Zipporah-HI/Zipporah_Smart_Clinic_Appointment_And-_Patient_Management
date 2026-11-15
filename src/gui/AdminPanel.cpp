#include "gui/AdminPanel.h"

#include "gui/ClinicTheme.h"
#include "utils/ClinicExceptions.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <fstream>

namespace scapms::gui {

AdminPanel::AdminPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    refreshData();
}

void AdminPanel::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Top Row: Report 3 (Category Distribution) + Report 4 (Daily Summary) + Admin Undo/File Controls
    auto* topGrid = new QGridLayout();
    topGrid->setSpacing(12);

    // Report 3: Patient Category Distribution
    auto* distGroup = new QGroupBox("Report 3: Patient Category Distribution");
    auto* distLayout = new QGridLayout(distGroup);

    generalCountLbl_ = new QLabel("General Patients: 0");
    emergencyCountLbl_ = new QLabel("Emergency Patients: 0");
    followUpCountLbl_ = new QLabel("Follow-Up Patients: 0");

    generalBar_ = new QProgressBar();
    generalBar_->setStyleSheet("QProgressBar::chunk { background-color: #16A34A; border-radius: 4px; }");
    emergencyBar_ = new QProgressBar();
    emergencyBar_->setStyleSheet("QProgressBar::chunk { background-color: #DC2626; border-radius: 4px; }");
    followUpBar_ = new QProgressBar();
    followUpBar_->setStyleSheet("QProgressBar::chunk { background-color: #D97706; border-radius: 4px; }");

    distLayout->addWidget(generalCountLbl_, 0, 0);
    distLayout->addWidget(generalBar_, 0, 1);
    distLayout->addWidget(emergencyCountLbl_, 1, 0);
    distLayout->addWidget(emergencyBar_, 1, 1);
    distLayout->addWidget(followUpCountLbl_, 2, 0);
    distLayout->addWidget(followUpBar_, 2, 1);

    // Report 4: Daily Appointment Summary
    auto* dailyGroup = new QGroupBox("Report 4: Daily Appointment Summary");
    auto* dailyLayout = new QVBoxLayout(dailyGroup);

    auto* filterRow = new QHBoxLayout();
    dateFilterEdit_ = new QLineEdit();
    dateFilterEdit_->setPlaceholderText("All dates (or YYYY-MM-DD, e.g., 2026-11-13)");
    dateFilterEdit_->setToolTip("Leave blank for overall clinic summary, or enter YYYY-MM-DD for a specific day.");
    auto* applyDateBtn = new QPushButton("Filter");
    applyDateBtn->setProperty("variant", "secondary");
    filterRow->addWidget(new QLabel("Date:"));
    filterRow->addWidget(dateFilterEdit_, 1);
    filterRow->addWidget(applyDateBtn);

    auto* summaryGrid = new QGridLayout();
    dailyTotalLbl_ = new QLabel("Total Appointments: 0");
    dailyTotalLbl_->setStyleSheet("font-weight: 700; color: #0F2942;");
    dailyCompletedLbl_ = new QLabel("Completed: 0");
    dailyCompletedLbl_->setStyleSheet("font-weight: 700; color: #166534;");
    dailyPendingLbl_ = new QLabel("Pending: 0");
    dailyPendingLbl_->setStyleSheet("font-weight: 700; color: #92400E;");
    dailyCancelledLbl_ = new QLabel("Cancelled: 0");
    dailyCancelledLbl_->setStyleSheet("font-weight: 700; color: #64748B;");

    summaryGrid->addWidget(dailyTotalLbl_, 0, 0);
    summaryGrid->addWidget(dailyCompletedLbl_, 0, 1);
    summaryGrid->addWidget(dailyPendingLbl_, 1, 0);
    summaryGrid->addWidget(dailyCancelledLbl_, 1, 1);

    dailyLayout->addLayout(filterRow);
    dailyLayout->addLayout(summaryGrid);

    // Admin Undo Stack & File Persistence Controls
    auto* adminGroup = new QGroupBox("Admin Undo Stack (std::stack) && File Persistence");
    auto* adminLayout = new QVBoxLayout(adminGroup);
    undoStackStatusLbl_ = new QLabel("Undo Stack Depth: 0 | Top: None");
    undoStackStatusLbl_->setWordWrap(true);
    undoStackStatusLbl_->setStyleSheet("color: #334155; font-size: 12px;");

    undoBtn_ = new QPushButton("Undo Last Administrative Operation");
    undoBtn_->setProperty("variant", "warning");
    undoBtn_->setToolTip("Reverses the most recent patient deletion, modification, or appointment removal.");

    auto* fileRow = new QHBoxLayout();
    auto* saveDlgBtn = new QPushButton("Export CSV...");
    saveDlgBtn->setToolTip("Choose directory/file to export clinic records using a file dialog.");
    auto* loadDlgBtn = new QPushButton("Import CSV...");
    loadDlgBtn->setProperty("variant", "secondary");
    loadDlgBtn->setToolTip("Choose a CSV file to import clinic records.");
    auto* exportReportBtn = new QPushButton("Export Report");
    exportReportBtn->setProperty("variant", "secondary");
    exportReportBtn->setToolTip("Save all 4 clinic reports to a formatted text file.");
    fileRow->addWidget(saveDlgBtn);
    fileRow->addWidget(loadDlgBtn);
    fileRow->addWidget(exportReportBtn);

    adminLayout->addWidget(undoStackStatusLbl_);
    adminLayout->addWidget(undoBtn_);
    adminLayout->addLayout(fileRow);

    topGrid->addWidget(distGroup, 0, 0);
    topGrid->addWidget(dailyGroup, 0, 1);
    topGrid->addWidget(adminGroup, 0, 2);
    rootLayout->addLayout(topGrid);

    // Bottom Row: Report 1 + Report 2 Tables
    auto* tablesRow = new QHBoxLayout();
    tablesRow->setSpacing(12);

    auto* attendedGroup = new QGroupBox("Report 1: Most Frequently Attended Patients (Completed Visits)");
    auto* attendedLayout = new QVBoxLayout(attendedGroup);
    attendedTable_ = new QTableWidget(0, 4);
    attendedTable_->setHorizontalHeaderLabels({"Patient ID", "Name", "Category", "Completed Visits"});
    attendedTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    attendedTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    attendedTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    attendedTable_->setAlternatingRowColors(true);
    attendedLayout->addWidget(attendedTable_);

    auto* pendingGroup = new QGroupBox("Report 2: Patients with Pending Appointments");
    auto* pendingLayout = new QVBoxLayout(pendingGroup);
    pendingTable_ = new QTableWidget(0, 5);
    pendingTable_->setHorizontalHeaderLabels(
        {"Patient ID", "Name", "Category", "Pending", "Next Scheduled"});
    pendingTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    pendingTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    pendingTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    pendingTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pendingTable_->setAlternatingRowColors(true);
    pendingLayout->addWidget(pendingTable_);

    tablesRow->addWidget(attendedGroup, 1);
    tablesRow->addWidget(pendingGroup, 1);
    rootLayout->addLayout(tablesRow, 1);

    connect(undoBtn_, &QPushButton::clicked, this, &AdminPanel::onUndoAction);
    connect(applyDateBtn, &QPushButton::clicked, this, &AdminPanel::onFilterDateChanged);
    connect(dateFilterEdit_, &QLineEdit::returnPressed, this, &AdminPanel::onFilterDateChanged);
    connect(saveDlgBtn, &QPushButton::clicked, this, &AdminPanel::requestSaveDialog);
    connect(loadDlgBtn, &QPushButton::clicked, this, &AdminPanel::requestLoadDialog);
    connect(exportReportBtn, &QPushButton::clicked, this, &AdminPanel::onExportReport);
}

void AdminPanel::onUndoAction() {
    try {
        const std::string msg = manager_.undoLastOperation();
        refreshData();
        emit dataChanged(QString::fromStdString(msg));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::information(this, "Undo Stack", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to undo administrative action.");
    }
}

void AdminPanel::onFilterDateChanged() {
    refreshData();
}

void AdminPanel::onExportReport() {
    const QString path = QFileDialog::getSaveFileName(
        this,
        "Export Clinic Summary Report",
        "clinic_report.txt",
        "Text Files (*.txt);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }

    try {
        std::ofstream out(path.toStdString(), std::ios::trunc);
        if (!out.is_open()) {
            throw utils::FileIOException("Cannot write report to: " + path.toStdString());
        }
        const auto dist = manager_.getCategoryDistribution();
        const auto daily = manager_.getDailyAppointmentSummary(
            dateFilterEdit_->text().trimmed().toStdString());
        const auto attended = manager_.getMostFrequentlyAttendedPatients();
        const auto pending = manager_.getPatientsWithPendingAppointments();

        out << "=== SCAPMS UNIVERSITY CLINIC REPORT ===\n\n";
        out << "1. Patient Category Distribution\n";
        out << "   General Patients:   " << dist.generalCount << "\n";
        out << "   Emergency Patients: " << dist.emergencyCount << "\n";
        out << "   Follow-Up Patients: " << dist.followUpCount << "\n";
        out << "   Total Patients:     " << dist.totalPatients << "\n\n";

        out << "2. Appointment Summary"
            << (daily.dateFilter.empty() ? " (All Dates)" : " (" + daily.dateFilter + ")") << "\n";
        out << "   Total Appointments: " << daily.totalAppointments << "\n";
        out << "   Completed:          " << daily.completedCount << "\n";
        out << "   Pending:            " << daily.pendingCount << "\n";
        out << "   Cancelled:          " << daily.cancelledCount << "\n\n";

        out << "3. Most Frequently Attended Patients\n";
        for (const auto& row : attended) {
            out << "   - " << row.patientId << " | " << row.name << " (" << row.category
                << ") : " << row.completedCount << " completed visits\n";
        }
        out << "\n4. Patients with Pending Appointments\n";
        for (const auto& row : pending) {
            out << "   - " << row.patientId << " | " << row.name << " (" << row.category
                << ") : " << row.pendingCount << " pending, next at " << row.nextDateTime << "\n";
        }

        emit dataChanged(QString("Exported clinic report to %1").arg(path));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Export Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to export clinic report.");
    }
}

void AdminPanel::refreshData() {
    // Report 1: Most Frequently Attended Patients
    const auto attended = manager_.getMostFrequentlyAttendedPatients();
    attendedTable_->setRowCount(static_cast<int>(attended.size()));
    for (int r = 0; r < static_cast<int>(attended.size()); ++r) {
        const auto& row = attended[static_cast<std::size_t>(r)];
        const QString cat = QString::fromStdString(row.category);
        attendedTable_->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(row.patientId)));
        attendedTable_->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(row.name)));
        auto* catItem = new QTableWidgetItem(cat);
        catItem->setBackground(categoryBgColor(cat));
        catItem->setForeground(categoryFgColor(cat));
        attendedTable_->setItem(r, 2, catItem);
        attendedTable_->setItem(r, 3, new QTableWidgetItem(QString::number(row.completedCount)));
    }

    // Report 2: Patients with Pending Appointments
    const auto pending = manager_.getPatientsWithPendingAppointments();
    pendingTable_->setRowCount(static_cast<int>(pending.size()));
    for (int r = 0; r < static_cast<int>(pending.size()); ++r) {
        const auto& row = pending[static_cast<std::size_t>(r)];
        const QString cat = QString::fromStdString(row.category);
        pendingTable_->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(row.patientId)));
        pendingTable_->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(row.name)));
        auto* catItem = new QTableWidgetItem(cat);
        catItem->setBackground(categoryBgColor(cat));
        catItem->setForeground(categoryFgColor(cat));
        pendingTable_->setItem(r, 2, catItem);
        pendingTable_->setItem(r, 3, new QTableWidgetItem(QString::number(row.pendingCount)));
        pendingTable_->setItem(r, 4, new QTableWidgetItem(QString::fromStdString(row.nextDateTime)));
    }

    // Report 3: Category Distribution
    const auto dist = manager_.getCategoryDistribution();
    const int maxBar = dist.totalPatients > 0 ? dist.totalPatients : 1;
    generalCountLbl_->setText(QString("General Patients: %1").arg(dist.generalCount));
    emergencyCountLbl_->setText(QString("Emergency Patients: %1").arg(dist.emergencyCount));
    followUpCountLbl_->setText(QString("Follow-Up Patients: %1").arg(dist.followUpCount));
    generalBar_->setRange(0, maxBar);
    generalBar_->setValue(dist.generalCount);
    emergencyBar_->setRange(0, maxBar);
    emergencyBar_->setValue(dist.emergencyCount);
    followUpBar_->setRange(0, maxBar);
    followUpBar_->setValue(dist.followUpCount);

    // Report 4: Daily Appointment Summary
    const auto daily = manager_.getDailyAppointmentSummary(
        dateFilterEdit_->text().trimmed().toStdString());
    dailyTotalLbl_->setText(QString("Total Appointments: %1").arg(daily.totalAppointments));
    dailyCompletedLbl_->setText(QString("Completed: %1").arg(daily.completedCount));
    dailyPendingLbl_->setText(QString("Pending: %1").arg(daily.pendingCount));
    dailyCancelledLbl_->setText(QString("Cancelled: %1").arg(daily.cancelledCount));

    // Undo Stack Status
    const auto& db = manager_.getDatabase();
    undoBtn_->setEnabled(db.canUndo());
    undoStackStatusLbl_->setText(
        QString("Undo Stack Depth: %1\nTop Action: %2")
            .arg(db.getUndoStackSize())
            .arg(QString::fromStdString(db.peekUndoDescription())));
}

} // namespace scapms::gui
