#ifndef SCAPMS_MODEL_GENERAL_PATIENT_H
#define SCAPMS_MODEL_GENERAL_PATIENT_H

#include "model/Appointable.h"
#include "model/PatientRecord.h"

#include <string>

namespace scapms::model {

/**
 * @brief Represents a routine outpatient visiting the university clinic.
 */
class GeneralPatient : public PatientRecord, public Appointable {
public:
    GeneralPatient(std::string patientId,
                   std::string name,
                   int age,
                   std::string contactInfo,
                   std::string visitReason = "General Consultation",
                   std::string primaryPhysician = "Duty Medical Officer");

    const std::string& getVisitReason() const;
    const std::string& getPrimaryPhysician() const;
    void setVisitReason(const std::string& visitReason);
    void setPrimaryPhysician(const std::string& primaryPhysician);

    // PatientRecord polymorphic overrides
    std::string getPatientCategory() const override;
    int getPriorityLevel() const override;
    std::string getPriorityLabel() const override;
    std::string getDetailsSummary() const override;
    std::string getTypeSpecificField1() const override;
    std::string getTypeSpecificField2() const override;
    std::unique_ptr<PatientRecord> clone() const override;

    // Appointable interface implementation
    bool scheduleAppointment(const std::string& appointmentId) override;
    bool cancelAppointment(const std::string& appointmentId) override;
    std::string getSchedulingPolicy() const override;

private:
    std::string visitReason_;
    std::string primaryPhysician_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_GENERAL_PATIENT_H
