#include "model/Appointment.h"

#include "utils/ClinicExceptions.h"

#include <cctype>
#include <regex>

namespace scapms::model {

namespace {
std::string trimCopy(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}
} // namespace

Appointment::Appointment(std::string appointmentId,
                         std::string patientId,
                         std::string doctorName,
                         std::string date,
                         std::string time,
                         std::string purpose,
                         AppointmentStatus status,
                         std::string notes)
    : appointmentId_(trimCopy(appointmentId)),
      patientId_(trimCopy(patientId)),
      doctorName_(trimCopy(doctorName)),
      date_(trimCopy(date)),
      time_(trimCopy(time)),
      purpose_(trimCopy(purpose)),
      status_(status),
      notes_(trimCopy(notes)) {
    validateFields(appointmentId_, patientId_, doctorName_, date_, time_, purpose_);
}

const std::string& Appointment::getAppointmentId() const { return appointmentId_; }
const std::string& Appointment::getPatientId() const { return patientId_; }
const std::string& Appointment::getDoctorName() const { return doctorName_; }
const std::string& Appointment::getDate() const { return date_; }
const std::string& Appointment::getTime() const { return time_; }
const std::string& Appointment::getPurpose() const { return purpose_; }
AppointmentStatus Appointment::getStatus() const { return status_; }
std::string Appointment::getStatusString() const { return statusToString(status_); }
const std::string& Appointment::getNotes() const { return notes_; }

void Appointment::setDoctorName(const std::string& doctorName) {
    const std::string cleaned = trimCopy(doctorName);
    if (cleaned.empty()) {
        throw utils::InvalidAppointmentException("Doctor name cannot be empty.");
    }
    doctorName_ = cleaned;
}

void Appointment::setDate(const std::string& date) {
    const std::string cleaned = trimCopy(date);
    static const std::regex datePattern(R"(^\d{4}-\d{2}-\d{2}$)");
    if (!std::regex_match(cleaned, datePattern)) {
        throw utils::InvalidAppointmentException(
            "Appointment date must use YYYY-MM-DD format.");
    }
    const int month = std::stoi(cleaned.substr(5, 2));
    const int day = std::stoi(cleaned.substr(8, 2));
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        throw utils::InvalidAppointmentException("Appointment date contains an invalid month or day.");
    }
    date_ = cleaned;
}

void Appointment::setTime(const std::string& time) {
    const std::string cleaned = trimCopy(time);
    static const std::regex timePattern(R"(^\d{2}:\d{2}$)");
    if (!std::regex_match(cleaned, timePattern)) {
        throw utils::InvalidAppointmentException(
            "Appointment time must use HH:MM (24-hour) format.");
    }
    const int hour = std::stoi(cleaned.substr(0, 2));
    const int minute = std::stoi(cleaned.substr(3, 2));
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        throw utils::InvalidAppointmentException("Appointment time is out of range (00:00 - 23:59).");
    }
    time_ = cleaned;
}

void Appointment::setPurpose(const std::string& purpose) {
    const std::string cleaned = trimCopy(purpose);
    if (cleaned.empty()) {
        throw utils::InvalidAppointmentException("Appointment purpose cannot be empty.");
    }
    purpose_ = cleaned;
}

void Appointment::setStatus(AppointmentStatus status) { status_ = status; }
void Appointment::setNotes(const std::string& notes) { notes_ = trimCopy(notes); }

std::string Appointment::statusToString(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Pending:
            return "Pending";
        case AppointmentStatus::Completed:
            return "Completed";
        case AppointmentStatus::Cancelled:
            return "Cancelled";
    }
    return "Pending";
}

AppointmentStatus Appointment::stringToStatus(const std::string& value) {
    if (value == "Completed") {
        return AppointmentStatus::Completed;
    }
    if (value == "Cancelled") {
        return AppointmentStatus::Cancelled;
    }
    if (value == "Pending") {
        return AppointmentStatus::Pending;
    }
    throw utils::InvalidAppointmentException("Unknown appointment status: " + value);
}

void Appointment::validateFields(const std::string& appointmentId,
                                 const std::string& patientId,
                                 const std::string& doctorName,
                                 const std::string& date,
                                 const std::string& time,
                                 const std::string& purpose) {
    if (trimCopy(appointmentId).empty()) {
        throw utils::InvalidAppointmentException("Appointment ID cannot be empty.");
    }
    if (trimCopy(patientId).empty()) {
        throw utils::InvalidAppointmentException("Appointment must reference a valid Patient ID.");
    }
    if (trimCopy(doctorName).empty()) {
        throw utils::InvalidAppointmentException("Doctor name cannot be empty.");
    }

    static const std::regex datePattern(R"(^\d{4}-\d{2}-\d{2}$)");
    const std::string cleanedDate = trimCopy(date);
    if (!std::regex_match(cleanedDate, datePattern)) {
        throw utils::InvalidAppointmentException(
            "Appointment date must use YYYY-MM-DD format.");
    }
    const int month = std::stoi(cleanedDate.substr(5, 2));
    const int day = std::stoi(cleanedDate.substr(8, 2));
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        throw utils::InvalidAppointmentException("Appointment date contains an invalid month or day.");
    }

    static const std::regex timePattern(R"(^\d{2}:\d{2}$)");
    const std::string cleanedTime = trimCopy(time);
    if (!std::regex_match(cleanedTime, timePattern)) {
        throw utils::InvalidAppointmentException(
            "Appointment time must use HH:MM (24-hour) format.");
    }
    const int hour = std::stoi(cleanedTime.substr(0, 2));
    const int minute = std::stoi(cleanedTime.substr(3, 2));
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        throw utils::InvalidAppointmentException("Appointment time is out of range (00:00 - 23:59).");
    }

    if (trimCopy(purpose).empty()) {
        throw utils::InvalidAppointmentException("Appointment purpose cannot be empty.");
    }
}

} // namespace scapms::model
