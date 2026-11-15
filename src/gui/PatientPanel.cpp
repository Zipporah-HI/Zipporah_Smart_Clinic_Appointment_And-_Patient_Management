#include "gui/PatientPanel.h"

#include "gui/ClinicTheme.h"
#include "utils/ClinicExceptions.h"

#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace scapms::gui {

PatientPanel::PatientPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    refreshTable();
}

void PatientPanel::setupUi() {
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(14);

    // Left: Patient Registration & Dynamic Edit Form
    auto* formGroup = new QGroupBox("Patient Registration && Profile");
    formGroup->setMinimumWidth(360);
    formGroup->setMaximumWidth(430);
    auto* formOuterLayout = new QVBoxLayout(formGroup);

    auto* commonForm = new QFormLayout();
    commonForm->setSpacing(10);

    idEdit_ = new QLineEdit();
    idEdit_->setPlaceholderText("Auto-generated if empty (e.g., PAT-1009)");
    idEdit_->setToolTip("Leave blank to auto-generate a unique Patient ID, or enter a custom ID.");

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"General", "Emergency", "Follow-Up"});
    categoryCombo_->setToolTip("Dynamically adjusts extra medical intake fields below.");

    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Full patient name");
    nameEdit_->setToolTip("Enter the patient's full legal name.");

    ageSpin_ = new QSpinBox();
    ageSpin_->setRange(-10, 200);
    ageSpin_->setValue(21);
    ageSpin_->setToolTip("Valid clinic patient age: 0 to 120 years.");

    contactEdit_ = new QLineEdit();
    contactEdit_->setPlaceholderText("Phone or university email");
    contactEdit_->setToolTip("Enter phone number or campus email contact.");

    commonForm->addRow("Patient ID:", idEdit_);
    commonForm->addRow("Patient Type:", categoryCombo_);
    commonForm->addRow("Full Name:", nameEdit_);
    commonForm->addRow("Age:", ageSpin_);
    commonForm->addRow("Contact Info:", contactEdit_);

    formOuterLayout->addLayout(commonForm);

    // Dynamic Type-Specific Fields using QStackedLayout (PRD 9b)
    auto* dynamicBox = new QGroupBox("Category-Specific Intake Fields (Dynamic)");
    dynamicFieldsStack_ = new QStackedLayout(dynamicBox);

    // Page 0: GeneralPatient
    auto* generalPage = new QWidget();
    auto* generalForm = new QFormLayout(generalPage);
    generalReasonEdit_ = new QLineEdit();
    generalReasonEdit_->setPlaceholderText("e.g., Annual Checkup, Fever");
    generalPhysicianEdit_ = new QLineEdit();
    generalPhysicianEdit_->setPlaceholderText("e.g., Dr. Amina Bello");
    generalForm->addRow("Visit Reason:", generalReasonEdit_);
    generalForm->addRow("Primary Physician:", generalPhysicianEdit_);

    // Page 1: EmergencyPatient
    auto* emergencyPage = new QWidget();
    auto* emergencyForm = new QFormLayout(emergencyPage);
    emergencySeverityCombo_ = new QComboBox();
    emergencySeverityCombo_->setEditable(true);
    emergencySeverityCombo_->addItems({"Critical", "High - Immediate", "Urgent"});
    emergencyContactEdit_ = new QLineEdit();
    emergencyContactEdit_->setPlaceholderText("Next of kin / campus security");
    emergencyForm->addRow("Triage Severity:", emergencySeverityCombo_);
    emergencyForm->addRow("Emergency Contact:", emergencyContactEdit_);

    // Page 2: FollowUpPatient
    auto* followUpPage = new QWidget();
    auto* followUpForm = new QFormLayout(followUpPage);
    followUpPrevDateEdit_ = new QLineEdit();
    followUpPrevDateEdit_->setPlaceholderText("YYYY-MM-DD (e.g., 2026-10-28)");
    followUpDoctorEdit_ = new QLineEdit();
    followUpDoctorEdit_->setPlaceholderText("e.g., Dr. Chidi Okafor");
    followUpForm->addRow("Previous Visit Date:", followUpPrevDateEdit_);
    followUpForm->addRow("Referring Doctor:", followUpDoctorEdit_);

    dynamicFieldsStack_->addWidget(generalPage);
    dynamicFieldsStack_->addWidget(emergencyPage);
    dynamicFieldsStack_->addWidget(followUpPage);
    formOuterLayout->addWidget(dynamicBox);

    validationHintLabel_ = new QLabel("Ready to register or select a patient row to edit.");
    validationHintLabel_->setWordWrap(true);
    validationHintLabel_->setStyleSheet("color: #0F766E; font-size: 12px; padding: 4px 0;");
    formOuterLayout->addWidget(validationHintLabel_);

    // Action Buttons
    auto* btnGrid = new QVBoxLayout();
    btnGrid->setSpacing(8);

    auto* row1 = new QHBoxLayout();
    auto* addBtn = new QPushButton("Add Patient");
    addBtn->setToolTip("Register a new patient record (validates fields and ID uniqueness).");
    auto* updateBtn = new QPushButton("Update Selected");
    updateBtn->setProperty("variant", "warning");
    updateBtn->setToolTip("Modify the selected patient record (pushes snapshot to Undo stack).");
    row1->addWidget(addBtn);
    row1->addWidget(updateBtn);

    auto* row2 = new QHBoxLayout();
    auto* deleteBtn = new QPushButton("Delete Selected");
    deleteBtn->setProperty("variant", "danger");
    deleteBtn->setToolTip("Delete selected patient record (reversible via Undo Stack).");
    auto* enqueueBtn = new QPushButton("Add to Queue");
    enqueueBtn->setToolTip("Place selected patient into the consultation waiting queue.");
    row2->addWidget(deleteBtn);
    row2->addWidget(enqueueBtn);

    auto* row3 = new QHBoxLayout();
    undoBtn_ = new QPushButton("Undo Last Action (Stack)");
    undoBtn_->setProperty("variant", "secondary");
    undoBtn_->setToolTip("Pop the std::stack and reverse the most recent administrative operation.");
    auto* clearBtn = new QPushButton("Clear Form");
    clearBtn->setProperty("variant", "secondary");
    clearBtn->setToolTip("Reset all input fields to register a new patient.");
    row3->addWidget(undoBtn_);
    row3->addWidget(clearBtn);

    btnGrid->addLayout(row1);
    btnGrid->addLayout(row2);
    btnGrid->addLayout(row3);
    formOuterLayout->addLayout(btnGrid);
    formOuterLayout->addStretch();

    // Right: Registered Patients Table
    auto* tableGroup = new QGroupBox("Registered Patient Directory (std::vector<std::unique_ptr<PatientRecord>>)");
    auto* tableLayout = new QVBoxLayout(tableGroup);

    patientTable_ = new QTableWidget(0, 7);
    patientTable_->setHorizontalHeaderLabels(
        {"Patient ID", "Name", "Age", "Category", "Priority Badge", "Contact", "Clinical Details"});
    patientTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    patientTable_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    patientTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    patientTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    patientTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    patientTable_->setAlternatingRowColors(true);
    tableLayout->addWidget(patientTable_);

    rootLayout->addWidget(formGroup);
    rootLayout->addWidget(tableGroup, 1);

    // Event wiring
    connect(categoryCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &PatientPanel::onCategoryChanged);
    connect(patientTable_, &QTableWidget::itemSelectionChanged,
            this, &PatientPanel::onTableSelectionChanged);
    connect(addBtn, &QPushButton::clicked, this, &PatientPanel::onAddPatient);
    connect(updateBtn, &QPushButton::clicked, this, &PatientPanel::onUpdatePatient);
    connect(deleteBtn, &QPushButton::clicked, this, &PatientPanel::onDeletePatient);
    connect(enqueueBtn, &QPushButton::clicked, this, &PatientPanel::onEnqueueSelected);
    connect(undoBtn_, &QPushButton::clicked, this, &PatientPanel::onUndoAction);
    connect(clearBtn, &QPushButton::clicked, this, &PatientPanel::onClearForm);

    // Text field live validation events (PRD 6c)
    connect(idEdit_, &QLineEdit::textChanged, this, &PatientPanel::onLiveValidateInput);
    connect(nameEdit_, &QLineEdit::textChanged, this, &PatientPanel::onLiveValidateInput);
    connect(ageSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &PatientPanel::onLiveValidateInput);
}

void PatientPanel::onCategoryChanged(int index) {
    if (dynamicFieldsStack_ && index >= 0 && index < dynamicFieldsStack_->count()) {
        dynamicFieldsStack_->setCurrentIndex(index);
    }
}

void PatientPanel::onLiveValidateInput() {
    const int age = ageSpin_->value();
    const QString name = nameEdit_->text().trimmed();
    const QString customId = idEdit_->text().trimmed();

    if (age < 0 || age > 120) {
        validationHintLabel_->setStyleSheet("color: #DC2626; font-weight: 600; font-size: 12px;");
        validationHintLabel_->setText("Validation Warning: Age must be between 0 and 120.");
        return;
    }
    if (!customId.isEmpty() && manager_.getDatabase().findPatientRaw(customId.toStdString()) != nullptr) {
        validationHintLabel_->setStyleSheet("color: #D97706; font-weight: 600; font-size: 12px;");
        validationHintLabel_->setText(
            QString("Note: ID '%1' exists — use Update Selected or choose a new ID to Add.").arg(customId));
        return;
    }
    if (name.isEmpty()) {
        validationHintLabel_->setStyleSheet("color: #64748B; font-size: 12px;");
        validationHintLabel_->setText("Enter patient full name and contact info to register.");
        return;
    }
    validationHintLabel_->setStyleSheet("color: #0F766E; font-weight: 600; font-size: 12px;");
    validationHintLabel_->setText("Input looks valid for " + categoryCombo_->currentText() + " Patient.");
}

std::pair<std::string, std::string> PatientPanel::getDynamicFieldValues() const {
    const QString cat = categoryCombo_->currentText();
    if (cat == "General") {
        return {generalReasonEdit_->text().trimmed().toStdString(),
                generalPhysicianEdit_->text().trimmed().toStdString()};
    }
    if (cat == "Emergency") {
        return {emergencySeverityCombo_->currentText().trimmed().toStdString(),
                emergencyContactEdit_->text().trimmed().toStdString()};
    }
    return {followUpPrevDateEdit_->text().trimmed().toStdString(),
            followUpDoctorEdit_->text().trimmed().toStdString()};
}

void PatientPanel::setDynamicFieldValues(const std::string& category,
                                         const std::string& f1,
                                         const std::string& f2) {
    if (category == "General") {
        generalReasonEdit_->setText(QString::fromStdString(f1));
        generalPhysicianEdit_->setText(QString::fromStdString(f2));
    } else if (category == "Emergency") {
        emergencySeverityCombo_->setCurrentText(QString::fromStdString(f1));
        emergencyContactEdit_->setText(QString::fromStdString(f2));
    } else if (category == "Follow-Up") {
        followUpPrevDateEdit_->setText(QString::fromStdString(f1));
        followUpDoctorEdit_->setText(QString::fromStdString(f2));
    }
}

void PatientPanel::onTableSelectionChanged() {
    const auto selected = patientTable_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int row = selected.first()->row();
    const QString patientId = patientTable_->item(row, 0)->text();
    auto* patient = manager_.getDatabase().findPatientRaw(patientId.toStdString());
    if (!patient) {
        return;
    }
    manager_.getDatabase().recordPatientAccess(patient);

    idEdit_->setText(QString::fromStdString(patient->getPatientId()));
    nameEdit_->setText(QString::fromStdString(patient->getName()));
    ageSpin_->setValue(patient->getAge());
    contactEdit_->setText(QString::fromStdString(patient->getContactInfo()));

    const QString cat = QString::fromStdString(patient->getPatientCategory());
    const int catIdx = categoryCombo_->findText(cat);
    if (catIdx >= 0) {
        categoryCombo_->setCurrentIndex(catIdx);
    }
    setDynamicFieldValues(patient->getPatientCategory(),
                          patient->getTypeSpecificField1(),
                          patient->getTypeSpecificField2());
}

void PatientPanel::onAddPatient() {
    try {
        const auto [f1, f2] = getDynamicFieldValues();
        const std::string newId = manager_.registerPatient(
            categoryCombo_->currentText().toStdString(),
            idEdit_->text().trimmed().toStdString(),
            nameEdit_->text().trimmed().toStdString(),
            ageSpin_->value(),
            contactEdit_->text().trimmed().toStdString(),
            f1,
            f2);

        refreshTable();
        onClearForm();
        emit dataChanged(QString("Registered patient %1 successfully.").arg(QString::fromStdString(newId)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Validation Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to register patient due to an unknown error.");
    }
}

void PatientPanel::onUpdatePatient() {
    try {
        const std::string patientId = idEdit_->text().trimmed().toStdString();
        if (patientId.empty()) {
            throw utils::InvalidPatientException("Select a patient row or enter an existing Patient ID to update.");
        }
        const auto [f1, f2] = getDynamicFieldValues();
        manager_.updatePatient(patientId,
                               categoryCombo_->currentText().toStdString(),
                               nameEdit_->text().trimmed().toStdString(),
                               ageSpin_->value(),
                               contactEdit_->text().trimmed().toStdString(),
                               f1,
                               f2);
        refreshTable();
        emit dataChanged(QString("Updated patient %1 (previous version pushed to Undo stack).")
                             .arg(QString::fromStdString(patientId)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Update Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to update patient.");
    }
}

void PatientPanel::onDeletePatient() {
    try {
        const std::string patientId = idEdit_->text().trimmed().toStdString();
        if (patientId.empty()) {
            throw utils::InvalidPatientException("Please select a patient to delete.");
        }
        const auto reply = QMessageBox::question(
            this,
            "Confirm Delete",
            QString("Delete patient %1 and associated appointments?\n(This can be undone from the Undo Stack.)")
                .arg(QString::fromStdString(patientId)));
        if (reply != QMessageBox::Yes) {
            return;
        }
        if (!manager_.deletePatient(patientId)) {
            throw utils::InvalidPatientException("Patient ID not found: " + patientId);
        }
        refreshTable();
        onClearForm();
        emit dataChanged(QString("Deleted patient %1 (pushed to Undo stack).").arg(QString::fromStdString(patientId)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Delete Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to delete patient.");
    }
}

void PatientPanel::onEnqueueSelected() {
    try {
        const std::string patientId = idEdit_->text().trimmed().toStdString();
        if (patientId.empty()) {
            throw utils::InvalidPatientException("Select a patient first to add to the waiting queue.");
        }
        manager_.addToWaitingQueue(patientId);
        emit dataChanged(QString("Added patient %1 to the waiting queue.").arg(QString::fromStdString(patientId)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Waiting Queue", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Could not enqueue patient.");
    }
}

void PatientPanel::onUndoAction() {
    try {
        const std::string msg = manager_.undoLastOperation();
        refreshTable();
        emit dataChanged(QString::fromStdString(msg));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::information(this, "Undo Stack", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Undo failed.");
    }
}

void PatientPanel::onClearForm() {
    idEdit_->clear();
    nameEdit_->clear();
    ageSpin_->setValue(21);
    contactEdit_->clear();
    generalReasonEdit_->clear();
    generalPhysicianEdit_->clear();
    emergencySeverityCombo_->setCurrentIndex(0);
    emergencyContactEdit_->clear();
    followUpPrevDateEdit_->clear();
    followUpDoctorEdit_->clear();
    patientTable_->clearSelection();
    onLiveValidateInput();
}

void PatientPanel::refreshTable() {
    const auto& patients = manager_.getDatabase().getPatients();
    patientTable_->setRowCount(static_cast<int>(patients.size()));

    for (int row = 0; row < static_cast<int>(patients.size()); ++row) {
        const auto& p = patients[static_cast<std::size_t>(row)];
        const QString cat = QString::fromStdString(p->getPatientCategory());
        const QString pri = QString::fromStdString(p->getPriorityLabel());

        patientTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p->getPatientId())));
        patientTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p->getName())));
        patientTable_->setItem(row, 2, new QTableWidgetItem(QString::number(p->getAge())));

        auto* catItem = new QTableWidgetItem(cat);
        catItem->setBackground(categoryBgColor(cat));
        catItem->setForeground(categoryFgColor(cat));
        QFont boldFont = catItem->font();
        boldFont.setBold(true);
        catItem->setFont(boldFont);
        patientTable_->setItem(row, 3, catItem);

        auto* priItem = new QTableWidgetItem(pri);
        priItem->setBackground(categoryBgColor(cat));
        priItem->setForeground(categoryFgColor(cat));
        priItem->setFont(boldFont);
        patientTable_->setItem(row, 4, priItem);

        patientTable_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(p->getContactInfo())));
        patientTable_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(p->getDetailsSummary())));
    }

    if (undoBtn_) {
        undoBtn_->setEnabled(manager_.getDatabase().canUndo());
        undoBtn_->setText(
            manager_.getDatabase().canUndo()
                ? QString("Undo Last Action (%1)").arg(manager_.getDatabase().getUndoStackSize())
                : "Undo Last Action (Empty)");
    }
}

} // namespace scapms::gui
