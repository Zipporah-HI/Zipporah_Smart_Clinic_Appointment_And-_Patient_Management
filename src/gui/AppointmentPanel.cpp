#include "gui/AppointmentPanel.h"

#include "gui/ClinicTheme.h"
#include "model/Appointable.h"
#include "utils/ClinicExceptions.h"

#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <regex>

namespace scapms::gui {

AppointmentPanel::AppointmentPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    refreshData();
}

void AppointmentPanel::setupUi() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(14);

    // Left: Schedule Form
    auto* formGroup = new QGroupBox("Schedule && Manage Appointments");
    formGroup->setMinimumWidth(340);
    formGroup->setMaximumWidth(390);
    auto* formOuter = new QVBoxLayout(formGroup);

    auto* form = new QFormLayout();
    form->setSpacing(10);

    patientCombo_ = new QComboBox();
    patientCombo_->setToolTip("Select a registered patient implementing the Appointable interface.");

    policyHintLabel_ = new QLabel();
    policyHintLabel_->setWordWrap(true);
    policyHintLabel_->setStyleSheet("color: #0F766E; font-size: 12px; font-style: italic;");

    doctorEdit_ = new QLineEdit("Dr. Amina Bello");
    doctorEdit_->setPlaceholderText("Attending physician name");

    dateEdit_ = new QLineEdit("2026-11-13");
    dateEdit_->setPlaceholderText("YYYY-MM-DD");
    dateEdit_->setToolTip("Appointment date in YYYY-MM-DD format.");

    timeEdit_ = new QLineEdit("15:30");
    timeEdit_->setPlaceholderText("HH:MM (24-hour)");
    timeEdit_->setToolTip("Appointment time in 24-hour HH:MM format.");

    purposeEdit_ = new QLineEdit();
    purposeEdit_->setPlaceholderText("e.g., Clinical Consultation, Lab Review");

    notesEdit_ = new QLineEdit();
    notesEdit_->setPlaceholderText("Optional triage or preparation notes");

    form->addRow("Patient:", patientCombo_);
    form->addRow("Policy:", policyHintLabel_);
    form->addRow("Doctor Name:", doctorEdit_);
    form->addRow("Date (YYYY-MM-DD):", dateEdit_);
    form->addRow("Time (HH:MM):", timeEdit_);
    form->addRow("Purpose:", purposeEdit_);
    form->addRow("Notes:", notesEdit_);

    formOuter->addLayout(form);

    validationLabel_ = new QLabel("Ready to schedule a clinic appointment.");
    validationLabel_->setWordWrap(true);
    validationLabel_->setStyleSheet("color: #64748B; font-size: 12px; padding: 4px 0;");
    formOuter->addWidget(validationLabel_);

    auto* btnCol = new QVBoxLayout();
    btnCol->setSpacing(8);

    auto* scheduleBtn = new QPushButton("Schedule Appointment");
    scheduleBtn->setToolTip("Create a pending appointment and link it via Appointable::scheduleAppointment().");

    auto* rowAction = new QHBoxLayout();
    auto* completeBtn = new QPushButton("Mark Completed");
    completeBtn->setToolTip("Mark the selected appointment as Completed.");
    auto* cancelBtn = new QPushButton("Cancel Appointment");
    cancelBtn->setProperty("variant", "warning");
    cancelBtn->setToolTip("Cancel selected appointment (reversible via Undo Stack).");
    rowAction->addWidget(completeBtn);
    rowAction->addWidget(cancelBtn);

    auto* rowRemoveUndo = new QHBoxLayout();
    auto* removeBtn = new QPushButton("Remove Entry");
    removeBtn->setProperty("variant", "danger");
    removeBtn->setToolTip("Remove selected appointment from log (reversible via Undo Stack).");
    undoBtn_ = new QPushButton("Undo Last Action");
    undoBtn_->setProperty("variant", "secondary");
    undoBtn_->setToolTip("Reverse last administrative appointment/patient action.");
    rowRemoveUndo->addWidget(removeBtn);
    rowRemoveUndo->addWidget(undoBtn_);

    btnCol->addWidget(scheduleBtn);
    btnCol->addLayout(rowAction);
    btnCol->addLayout(rowRemoveUndo);
    formOuter->addLayout(btnCol);
    formOuter->addStretch();

    // Right: Appointments Table
    auto* tableGroup = new QGroupBox("Clinic Appointment Schedule");
    auto* tableLayout = new QVBoxLayout(tableGroup);

    appointmentTable_ = new QTableWidget(0, 8);
    appointmentTable_->setHorizontalHeaderLabels(
        {"Appt ID", "Patient ID", "Patient Name", "Doctor", "Date", "Time", "Status", "Purpose"});
    appointmentTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    appointmentTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    appointmentTable_->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    appointmentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    appointmentTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    appointmentTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    appointmentTable_->setAlternatingRowColors(true);
    tableLayout->addWidget(appointmentTable_);

    rootLayout->addWidget(formGroup);
    rootLayout->addWidget(tableGroup, 1);

    connect(patientCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &AppointmentPanel::onPatientComboChanged);
    connect(scheduleBtn, &QPushButton::clicked, this, &AppointmentPanel::onScheduleAppointment);
    connect(completeBtn, &QPushButton::clicked, this, &AppointmentPanel::onCompleteSelected);
    connect(cancelBtn, &QPushButton::clicked, this, &AppointmentPanel::onCancelSelected);
    connect(removeBtn, &QPushButton::clicked, this, &AppointmentPanel::onRemoveSelected);
    connect(undoBtn_, &QPushButton::clicked, this, &AppointmentPanel::onUndoAction);

    connect(dateEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onLiveValidateFields);
    connect(timeEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onLiveValidateFields);
    connect(purposeEdit_, &QLineEdit::textChanged, this, &AppointmentPanel::onLiveValidateFields);
}

void AppointmentPanel::onPatientComboChanged(int index) {
    if (index < 0) {
        policyHintLabel_->setText("No patient selected.");
        return;
    }
    const std::string pid = patientCombo_->itemData(index).toString().toStdString();
    const auto* patient = manager_.getDatabase().findPatientRaw(pid);
    if (!patient) {
        policyHintLabel_->setText("Select a registered patient.");
        return;
    }
    if (const auto* appointable = dynamic_cast<const model::Appointable*>(patient)) {
        policyHintLabel_->setText(QString::fromStdString(appointable->getSchedulingPolicy()));
    } else {
        policyHintLabel_->setText("Patient type does not implement Appointable.");
    }
}

void AppointmentPanel::onLiveValidateFields() {
    static const std::regex dateRx(R"(^\d{4}-\d{2}-\d{2}$)");
    static const std::regex timeRx(R"(^\d{2}:\d{2}$)");

    if (!std::regex_match(dateEdit_->text().trimmed().toStdString(), dateRx)) {
        validationLabel_->setStyleSheet("color: #DC2626; font-weight: 600; font-size: 12px;");
        validationLabel_->setText("Date format hint: use YYYY-MM-DD (e.g., 2026-11-13).");
        return;
    }
    if (!std::regex_match(timeEdit_->text().trimmed().toStdString(), timeRx)) {
        validationLabel_->setStyleSheet("color: #DC2626; font-weight: 600; font-size: 12px;");
        validationLabel_->setText("Time format hint: use HH:MM 24-hour (e.g., 14:30).");
        return;
    }
    if (purposeEdit_->text().trimmed().isEmpty()) {
        validationLabel_->setStyleSheet("color: #64748B; font-size: 12px;");
        validationLabel_->setText("Enter the appointment purpose to schedule.");
        return;
    }
    validationLabel_->setStyleSheet("color: #0F766E; font-weight: 600; font-size: 12px;");
    validationLabel_->setText("Appointment details look valid.");
}

QString AppointmentPanel::selectedAppointmentId() const {
    const auto selected = appointmentTable_->selectedItems();
    if (selected.isEmpty()) {
        return {};
    }
    return appointmentTable_->item(selected.first()->row(), 0)->text();
}

void AppointmentPanel::onScheduleAppointment() {
    try {
        if (patientCombo_->currentIndex() < 0) {
            throw utils::InvalidAppointmentException("Please register and select a patient first.");
        }
        const std::string pid = patientCombo_->currentData().toString().toStdString();
        const auto apt = manager_.getAppointmentController().scheduleAppointment(
            pid,
            doctorEdit_->text().trimmed().toStdString(),
            dateEdit_->text().trimmed().toStdString(),
            timeEdit_->text().trimmed().toStdString(),
            purposeEdit_->text().trimmed().toStdString(),
            notesEdit_->text().trimmed().toStdString());

        purposeEdit_->clear();
        notesEdit_->clear();
        refreshData();
        emit dataChanged(QString("Scheduled appointment %1 for patient %2.")
                             .arg(QString::fromStdString(apt.getAppointmentId()),
                                  QString::fromStdString(pid)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Appointment Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to schedule appointment.");
    }
}

void AppointmentPanel::onCompleteSelected() {
    try {
        const QString aptId = selectedAppointmentId();
        if (aptId.isEmpty()) {
            throw utils::InvalidAppointmentException("Select an appointment row first.");
        }
        if (!manager_.getAppointmentController().completeAppointment(aptId.toStdString())) {
            throw utils::InvalidAppointmentException("Unable to mark appointment completed.");
        }
        refreshData();
        emit dataChanged(QString("Marked appointment %1 as Completed.").arg(aptId));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Appointment Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to complete appointment.");
    }
}

void AppointmentPanel::onCancelSelected() {
    try {
        const QString aptId = selectedAppointmentId();
        if (aptId.isEmpty()) {
            throw utils::InvalidAppointmentException("Select an appointment row to cancel.");
        }
        if (!manager_.getAppointmentController().cancelAppointment(aptId.toStdString())) {
            throw utils::InvalidAppointmentException("Appointment is already cancelled or not found.");
        }
        refreshData();
        emit dataChanged(QString("Cancelled appointment %1 (pushed to Undo stack).").arg(aptId));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Appointment Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to cancel appointment.");
    }
}

void AppointmentPanel::onRemoveSelected() {
    try {
        const QString aptId = selectedAppointmentId();
        if (aptId.isEmpty()) {
            throw utils::InvalidAppointmentException("Select an appointment row to remove.");
        }
        if (!manager_.getAppointmentController().removeAppointment(aptId.toStdString())) {
            throw utils::InvalidAppointmentException("Appointment not found.");
        }
        refreshData();
        emit dataChanged(QString("Removed appointment %1 (pushed to Undo stack).").arg(aptId));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Appointment Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to remove appointment.");
    }
}

void AppointmentPanel::onUndoAction() {
    try {
        const std::string msg = manager_.undoLastOperation();
        refreshData();
        emit dataChanged(QString::fromStdString(msg));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::information(this, "Undo Stack", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Undo failed.");
    }
}

void AppointmentPanel::refreshData() {
    const QString prevSelection = patientCombo_->currentData().toString();
    patientCombo_->blockSignals(true);
    patientCombo_->clear();
    for (const auto& p : manager_.getDatabase().getPatients()) {
        const QString label = QString("%1 — %2 (%3)")
                                  .arg(QString::fromStdString(p->getPatientId()),
                                       QString::fromStdString(p->getName()),
                                       QString::fromStdString(p->getPatientCategory()));
        patientCombo_->addItem(label, QString::fromStdString(p->getPatientId()));
    }
    const int restoreIdx = patientCombo_->findData(prevSelection);
    if (restoreIdx >= 0) {
        patientCombo_->setCurrentIndex(restoreIdx);
    }
    patientCombo_->blockSignals(false);
    onPatientComboChanged(patientCombo_->currentIndex());

    const auto& appointments = manager_.getDatabase().getAppointments();
    appointmentTable_->setRowCount(static_cast<int>(appointments.size()));

    for (int row = 0; row < static_cast<int>(appointments.size()); ++row) {
        const auto& apt = appointments[static_cast<std::size_t>(row)];
        const auto* patient = manager_.getDatabase().findPatientRaw(apt.getPatientId());
        const QString patientName =
            patient ? QString::fromStdString(patient->getName()) : "(Unknown)";
        const QString status = QString::fromStdString(apt.getStatusString());

        appointmentTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(apt.getAppointmentId())));
        appointmentTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(apt.getPatientId())));
        appointmentTable_->setItem(row, 2, new QTableWidgetItem(patientName));
        appointmentTable_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(apt.getDoctorName())));
        appointmentTable_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(apt.getDate())));
        appointmentTable_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(apt.getTime())));

        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setBackground(statusBgColor(status));
        statusItem->setForeground(statusFgColor(status));
        QFont boldFont = statusItem->font();
        boldFont.setBold(true);
        statusItem->setFont(boldFont);
        appointmentTable_->setItem(row, 6, statusItem);

        appointmentTable_->setItem(row, 7, new QTableWidgetItem(QString::fromStdString(apt.getPurpose())));
    }

    if (undoBtn_) {
        undoBtn_->setEnabled(manager_.getDatabase().canUndo());
        undoBtn_->setText(
            manager_.getDatabase().canUndo()
                ? QString("Undo (%1)").arg(manager_.getDatabase().getUndoStackSize())
                : "Undo (Empty)");
    }
}

} // namespace scapms::gui
