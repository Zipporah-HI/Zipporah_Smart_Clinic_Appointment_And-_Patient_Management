#ifndef SCAPMS_GUI_PATIENT_PANEL_H
#define SCAPMS_GUI_PATIENT_PANEL_H

#include "controller/ClinicManager.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedLayout>
#include <QTableWidget>
#include <QWidget>

namespace scapms::gui {

class PatientPanel : public QWidget {
    Q_OBJECT

public:
    explicit PatientPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refreshTable();

signals:
    void dataChanged(const QString& statusMessage);

private slots:
    void onCategoryChanged(int index);
    void onTableSelectionChanged();
    void onAddPatient();
    void onUpdatePatient();
    void onDeletePatient();
    void onEnqueueSelected();
    void onUndoAction();
    void onClearForm();
    void onLiveValidateInput();

private:
    void setupUi();
    std::pair<std::string, std::string> getDynamicFieldValues() const;
    void setDynamicFieldValues(const std::string& category,
                               const std::string& f1,
                               const std::string& f2);

    controller::ClinicManager& manager_;

    QLineEdit* idEdit_ = nullptr;
    QComboBox* categoryCombo_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QSpinBox* ageSpin_ = nullptr;
    QLineEdit* contactEdit_ = nullptr;
    QLabel* validationHintLabel_ = nullptr;

    // Dynamic components via QStackedLayout (PRD 8v & 9b)
    QStackedLayout* dynamicFieldsStack_ = nullptr;
    QLineEdit* generalReasonEdit_ = nullptr;
    QLineEdit* generalPhysicianEdit_ = nullptr;
    QComboBox* emergencySeverityCombo_ = nullptr;
    QLineEdit* emergencyContactEdit_ = nullptr;
    QLineEdit* followUpPrevDateEdit_ = nullptr;
    QLineEdit* followUpDoctorEdit_ = nullptr;

    QTableWidget* patientTable_ = nullptr;
    QPushButton* undoBtn_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_PATIENT_PANEL_H
