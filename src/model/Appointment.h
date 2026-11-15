#ifndef SCAPMS_MODEL_APPOINTMENT_H
#define SCAPMS_MODEL_APPOINTMENT_H

#include <string>

namespace scapms::model {

enum class AppointmentStatus {
    Pending,
    Completed,
    Cancelled
};

/**
 * @brief Represents a clinic consultation appointment linked to a PatientRecord.
 */
class Appointment {
public:
    Appointment() = default;
    Appointment(std::string appointmentId,
                std::string patientId,
                std::string doctorName,
                std::string date,
                std::string time,
                std::string purpose,
                AppointmentStatus status = AppointmentStatus::Pending,
                std::string notes = "");

    const std::string& getAppointmentId() const;
    const std::string& getPatientId() const;
    const std::string& getDoctorName() const;
    const std::string& getDate() const;
    const std::string& getTime() const;
    const std::string& getPurpose() const;
    AppointmentStatus getStatus() const;
    std::string getStatusString() const;
    const std::string& getNotes() const;

    void setDoctorName(const std::string& doctorName);
    void setDate(const std::string& date);
    void setTime(const std::string& time);
    void setPurpose(const std::string& purpose);
    void setStatus(AppointmentStatus status);
    void setNotes(const std::string& notes);

    static std::string statusToString(AppointmentStatus status);
    static AppointmentStatus stringToStatus(const std::string& value);
    static void validateFields(const std::string& appointmentId,
                               const std::string& patientId,
                               const std::string& doctorName,
                               const std::string& date,
                               const std::string& time,
                               const std::string& purpose);

private:
    std::string appointmentId_;
    std::string patientId_;
    std::string doctorName_;
    std::string date_; // YYYY-MM-DD
    std::string time_; // HH:MM
    std::string purpose_;
    AppointmentStatus status_ = AppointmentStatus::Pending;
    std::string notes_;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_APPOINTMENT_H
