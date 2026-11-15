#ifndef SCAPMS_GUI_APPOINTMENT_PANEL_H
#define SCAPMS_GUI_APPOINTMENT_PANEL_H

#include "controller/ClinicManager.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace scapms::gui {

class AppointmentPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppointmentPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refreshData();

signals:
    void dataChanged(const QString& statusMessage);

private slots:
    void onPatientComboChanged(int index);
    void onScheduleAppointment();
    void onCompleteSelected();
    void onCancelSelected();
    void onRemoveSelected();
    void onUndoAction();
    void onLiveValidateFields();

private:
    void setupUi();
    QString selectedAppointmentId() const;

    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QLabel* policyHintLabel_ = nullptr;
    QLineEdit* doctorEdit_ = nullptr;
    QLineEdit* dateEdit_ = nullptr;
    QLineEdit* timeEdit_ = nullptr;
    QLineEdit* purposeEdit_ = nullptr;
    QLineEdit* notesEdit_ = nullptr;
    QLabel* validationLabel_ = nullptr;
    QTableWidget* appointmentTable_ = nullptr;
    QPushButton* undoBtn_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_APPOINTMENT_PANEL_H
