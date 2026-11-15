#ifndef SCAPMS_MODEL_FOLLOW_UP_PATIENT_H
#define SCAPMS_MODEL_FOLLOW_UP_PATIENT_H

#include "model/Appointable.h"
#include "model/PatientRecord.h"

#include <string>

namespace scapms::model {

/**
 * @brief Represents a returning patient scheduled for post-treatment review.
 */
class FollowUpPatient : public PatientRecord, public Appointable {
public:
    FollowUpPatient(std::string patientId,
                    std::string name,
                    int age,
                    std::string contactInfo,
                    std::string previousVisitDate = "2026-10-01",
                    std::string referringDoctor = "Dr. Clinic Specialist");

    const std::string& getPreviousVisitDate() const;
    const std::string& getReferringDoctor() const;
    void setPreviousVisitDate(const std::string& date);
    void setReferringDoctor(const std::string& doctor);

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
    std::string previousVisitDate_;
    std::string referringDoctor_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_FOLLOW_UP_PATIENT_H
