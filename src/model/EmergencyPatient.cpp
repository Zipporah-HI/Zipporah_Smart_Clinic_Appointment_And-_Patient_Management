#include "model/EmergencyPatient.h"

#include "utils/ClinicExceptions.h"

namespace scapms::model {

EmergencyPatient::EmergencyPatient(std::string patientId,
                                   std::string name,
                                   int age,
                                   std::string contactInfo,
                                   std::string triageSeverity,
                                   std::string emergencyContact)
    : PatientRecord(std::move(patientId), std::move(name), age, std::move(contactInfo)),
      triageSeverity_(triageSeverity.empty() ? "Critical" : std::move(triageSeverity)),
      emergencyContact_(emergencyContact.empty() ? "Next of Kin" : std::move(emergencyContact)) {}

const std::string& EmergencyPatient::getTriageSeverity() const { return triageSeverity_; }
const std::string& EmergencyPatient::getEmergencyContact() const { return emergencyContact_; }

void EmergencyPatient::setTriageSeverity(const std::string& severity) {
    if (severity.empty()) {
        throw utils::InvalidPatientException("Triage severity cannot be empty for an Emergency Patient.");
    }
    triageSeverity_ = severity;
}

void EmergencyPatient::setEmergencyContact(const std::string& emergencyContact) {
    if (emergencyContact.empty()) {
        throw utils::InvalidPatientException("Emergency contact cannot be empty for an Emergency Patient.");
    }
    emergencyContact_ = emergencyContact;
}

std::string EmergencyPatient::getPatientCategory() const { return "Emergency"; }
int EmergencyPatient::getPriorityLevel() const { return 1; }
std::string EmergencyPatient::getPriorityLabel() const { return "High Priority"; }

std::string EmergencyPatient::getDetailsSummary() const {
    return "Triage: " + triageSeverity_ + " | Emergency Contact: " + emergencyContact_;
}

std::string EmergencyPatient::getTypeSpecificField1() const { return triageSeverity_; }
std::string EmergencyPatient::getTypeSpecificField2() const { return emergencyContact_; }

std::unique_ptr<PatientRecord> EmergencyPatient::clone() const {
    return std::make_unique<EmergencyPatient>(*this);
}

bool EmergencyPatient::scheduleAppointment(const std::string& appointmentId) {
    if (appointmentId.empty()) {
        return false;
    }
    addAppointmentId(appointmentId);
    return true;
}

bool EmergencyPatient::cancelAppointment(const std::string& appointmentId) {
    return removeAppointmentId(appointmentId);
}

std::string EmergencyPatient::getSchedulingPolicy() const {
    return "Immediate triage override; bypasses routine wait restrictions.";
}

} // namespace scapms::model
