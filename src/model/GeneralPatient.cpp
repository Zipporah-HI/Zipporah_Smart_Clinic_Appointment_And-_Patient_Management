#include "model/GeneralPatient.h"

#include "utils/ClinicExceptions.h"

namespace scapms::model {

GeneralPatient::GeneralPatient(std::string patientId,
                               std::string name,
                               int age,
                               std::string contactInfo,
                               std::string visitReason,
                               std::string primaryPhysician)
    : PatientRecord(std::move(patientId), std::move(name), age, std::move(contactInfo)),
      visitReason_(visitReason.empty() ? "General Consultation" : std::move(visitReason)),
      primaryPhysician_(primaryPhysician.empty() ? "Duty Medical Officer" : std::move(primaryPhysician)) {}

const std::string& GeneralPatient::getVisitReason() const { return visitReason_; }
const std::string& GeneralPatient::getPrimaryPhysician() const { return primaryPhysician_; }

void GeneralPatient::setVisitReason(const std::string& visitReason) {
    if (visitReason.empty()) {
        throw utils::InvalidPatientException("Visit reason cannot be empty for a General Patient.");
    }
    visitReason_ = visitReason;
}

void GeneralPatient::setPrimaryPhysician(const std::string& primaryPhysician) {
    if (primaryPhysician.empty()) {
        throw utils::InvalidPatientException("Primary physician cannot be empty.");
    }
    primaryPhysician_ = primaryPhysician;
}

std::string GeneralPatient::getPatientCategory() const { return "General"; }
int GeneralPatient::getPriorityLevel() const { return 3; }
std::string GeneralPatient::getPriorityLabel() const { return "Normal"; }

std::string GeneralPatient::getDetailsSummary() const {
    return "Reason: " + visitReason_ + " | Physician: " + primaryPhysician_;
}

std::string GeneralPatient::getTypeSpecificField1() const { return visitReason_; }
std::string GeneralPatient::getTypeSpecificField2() const { return primaryPhysician_; }

std::unique_ptr<PatientRecord> GeneralPatient::clone() const {
    return std::make_unique<GeneralPatient>(*this);
}

bool GeneralPatient::scheduleAppointment(const std::string& appointmentId) {
    if (appointmentId.empty()) {
        return false;
    }
    addAppointmentId(appointmentId);
    return true;
}

bool GeneralPatient::cancelAppointment(const std::string& appointmentId) {
    return removeAppointmentId(appointmentId);
}

std::string GeneralPatient::getSchedulingPolicy() const {
    return "Standard outpatient queue & routine clinic slot.";
}

} // namespace scapms::model
