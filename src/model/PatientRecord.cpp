#include "model/PatientRecord.h"

#include "utils/ClinicExceptions.h"

#include <cctype>

namespace scapms::model {

namespace {
std::string trimString(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}
} // namespace

PatientRecord::PatientRecord(std::string patientId,
                             std::string name,
                             int age,
                             std::string contactInfo)
    : patientId_(trimString(patientId)),
      name_(trimString(name)),
      age_(age),
      contactInfo_(trimString(contactInfo)) {
    validateCommonFields(patientId_, name_, age_, contactInfo_);
}

const std::string& PatientRecord::getPatientId() const { return patientId_; }
const std::string& PatientRecord::getName() const { return name_; }
int PatientRecord::getAge() const { return age_; }
const std::string& PatientRecord::getContactInfo() const { return contactInfo_; }
const std::vector<std::string>& PatientRecord::getAppointmentIds() const { return appointmentIds_; }
int PatientRecord::getAccessCount() const { return accessCount_; }

void PatientRecord::setName(const std::string& name) {
    const std::string cleaned = trimString(name);
    if (cleaned.empty()) {
        throw utils::InvalidPatientException("Patient name cannot be empty.");
    }
    name_ = cleaned;
}

void PatientRecord::setAge(int age) {
    if (age < 0 || age > 120) {
        throw utils::InvalidAgeException(age);
    }
    age_ = age;
}

void PatientRecord::setContactInfo(const std::string& contactInfo) {
    const std::string cleaned = trimString(contactInfo);
    if (cleaned.empty()) {
        throw utils::InvalidPatientException("Contact information cannot be empty.");
    }
    contactInfo_ = cleaned;
}

void PatientRecord::addAppointmentId(const std::string& appointmentId) {
    if (!hasAppointmentId(appointmentId)) {
        appointmentIds_.push_back(appointmentId);
    }
}

bool PatientRecord::removeAppointmentId(const std::string& appointmentId) {
    for (size_t i = 0; i < appointmentIds_.size(); ++i) {
        if (appointmentIds_[i] == appointmentId) {
            appointmentIds_.erase(appointmentIds_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

bool PatientRecord::hasAppointmentId(const std::string& appointmentId) const {
    for (const auto& id : appointmentIds_) {
        if (id == appointmentId) {
            return true;
        }
    }
    return false;
}

void PatientRecord::incrementAccessCount() { ++accessCount_; }
void PatientRecord::setAccessCount(int count) { accessCount_ = count; }

void PatientRecord::validateCommonFields(const std::string& patientId,
                                         const std::string& name,
                                         int age,
                                         const std::string& contactInfo) {
    if (trimString(patientId).empty()) {
        throw utils::InvalidPatientException("Patient ID cannot be empty.");
    }
    if (trimString(name).empty()) {
        throw utils::InvalidPatientException("Patient name cannot be empty.");
    }
    if (age < 0 || age > 120) {
        throw utils::InvalidAgeException(age);
    }
    if (trimString(contactInfo).empty()) {
        throw utils::InvalidPatientException("Contact information cannot be empty.");
    }
}

} // namespace scapms::model
