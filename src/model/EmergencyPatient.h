#ifndef SCAPMS_MODEL_EMERGENCY_PATIENT_H
#define SCAPMS_MODEL_EMERGENCY_PATIENT_H

#include "model/Appointable.h"
#include "model/PatientRecord.h"

#include <string>

namespace scapms::model {

/**
 * @brief Represents an urgent/triage patient requiring immediate clinical attention.
 */
class EmergencyPatient : public PatientRecord, public Appointable {
public:
    EmergencyPatient(std::string patientId,
                     std::string name,
                     int age,
                     std::string contactInfo,
                     std::string triageSeverity = "Critical",
                     std::string emergencyContact = "Campus Security / Next of Kin");

    const std::string& getTriageSeverity() const;
    const std::string& getEmergencyContact() const;
    void setTriageSeverity(const std::string& severity);
    void setEmergencyContact(const std::string& emergencyContact);

    // PatientRecord polymorphic overrides
    std::string getPatientCategory() const override;
    int getPriorityLevel() const override;
    std::string getPriorityLabel() const override;
    std::string getDetailsSummary() const override;
    std::string getTypeSpecificField1() const override;
    std::string getTypeSpecificField2() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable interface implementation (priority emergency override)
    bool scheduleAppointment(const std::string& appointmentId) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    std::string getSchedulingPolicy() const override;

private:
    std::string triageSeverity_;
    std::string emergencyContact_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_EMERGENCY_PATIENT_H
