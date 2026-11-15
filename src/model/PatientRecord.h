#ifndef SCAPMS_MODEL_PATIENT_RECORD_H
#define SCAPMS_MODEL_PATIENT_RECORD_H

#include <memory>
#include <string>
#include <vector>

namespace scapms::model {

/**
 * @brief Abstract base class representing a patient registered at the university clinic.
 *
 * Enforces a polymorphic interface for category identification, triage priority,
 * clinical intake summary, and deep cloning (used by Undo/Admin snapshots).
 */
class PatientRecord {
public:
    PatientRecord(std::string patientId,
                  std::string name,
                  int age,
                  std::string contactInfo);
    virtual ~PatientRecord() = default;

    // Common encapsulated getters
    const std::string& getPatientId() const;
    const std::string& getName() const;
    int getAge() const;
    const std::string& getContactInfo() const;
    const std::vector<std::string>& getAppointmentIds() const;
    int getAccessCount() const;

    // Common encapsulated setters with validation
    void setName(const std::string& name);
    void setAge(int age);
    void setContactInfo(const std::string& contactInfo);

    // Appointment association helpers
    void addAppointmentId(const std::string& appointmentId);
    bool removeAppointmentId(const std::string& appointmentId);
    bool hasAppointmentId(const std::string& appointmentId) const;

    void incrementAccessCount();
    void setAccessCount(int count);

    // Pure virtual polymorphic interface
    virtual std::string getPatientCategory() const = 0;
    virtual int getPriorityLevel() const = 0; // 1 = Highest (Emergency), 2 = Medium (Follow-Up), 3 = Normal (General)
    virtual std::string getPriorityLabel() const = 0;
    virtual std::string getDetailsSummary() const = 0;
    virtual std::string getTypeSpecificField1() const = 0;
    virtual std::string getTypeSpecificField2() const = 0;
    virtual std::unique_ptr<PatientRecord> clone() const = 0;

    static void validateCommonFields(const std::string& patientId,
                                     const std::string& name,
                                     int age,
                                     const std::string& contactInfo);

protected:
    std::string patientId_;
    std::string name_;
    int age_ = 0;
    std::string contactInfo_;
    std::vector<std::string> appointmentIds_;
    int accessCount_ = 0;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_PATIENT_RECORD_H
