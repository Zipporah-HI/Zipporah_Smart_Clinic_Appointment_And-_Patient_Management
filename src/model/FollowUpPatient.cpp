#include "model/FollowUpPatient.h"

#include "utils/ClinicExceptions.h"

namespace scapms::model {

FollowUpPatient::FollowUpPatient(std::string patientId,
                                 std::string name,
                                 int age,
                                 std::string contactInfo,
                                 std::string previousVisitDate,
                                 std::string referringDoctor)
    : PatientRecord(std::move(patientId), std::move(name), age, std::move(contactInfo)),
      previousVisitDate_(previousVisitDate.empty() ? "2026-10-01" : std::move(previousVisitDate)),
      referringDoctor_(referringDoctor.empty() ? "Dr. Clinic Specialist" : std::move(referringDoctor)) {}

const std::string& FollowUpPatient::getPreviousVisitDate() const { return previousVisitDate_; }
const std::string& FollowUpPatient::getReferringDoctor() const { return referringDoctor_; }

void FollowUpPatient::setPreviousVisitDate(const std::string& date) {
    if (date.empty()) {
        throw utils::InvalidPatientException("Previous visit date cannot be empty for a Follow-Up Patient.");
    }
    previousVisitDate_ = date;
}

void FollowUpPatient::setReferringDoctor(const std::string& doctor) {
    if (doctor.empty()) {
        throw utils::InvalidPatientException("Referring doctor cannot be empty for a Follow-Up Patient.");
    }
    referringDoctor_ = doctor;
}

std::string FollowUpPatient::getPatientCategory() const { return "Follow-Up"; }
int FollowUpPatient::getPriorityLevel() const { return 2; }
std::string FollowUpPatient::getPriorityLabel() const { return "Scheduled"; }

std::string FollowUpPatient::getDetailsSummary() const {
    return "Prev Visit: " + previousVisitDate_ + " | Referring: " + referringDoctor_;
}

std::string FollowUpPatient::getTypeSpecificField1() const { return previousVisitDate_; }
std::string FollowUpPatient::getTypeSpecificField2() const { return referringDoctor_; }

std::unique_ptr<PatientRecord> FollowUpPatient::clone() const {
    return std::make_unique<FollowUpPatient>(*this);
}

bool FollowUpPatient::scheduleAppointment(const std::string& appointmentId) {
    if (appointmentId.empty()) {
        return false;
    }
    addAppointmentId(appointmentId);
    return true;
}

bool FollowUpPatient::cancelAppointment(const std::string& appointmentId) {
    return removeAppointmentId(appointmentId);
}

std::string FollowUpPatient::getSchedulingPolicy() const {
    return "Pre-scheduled review slot linked to previous visit.";
}

} // namespace scapms::model
