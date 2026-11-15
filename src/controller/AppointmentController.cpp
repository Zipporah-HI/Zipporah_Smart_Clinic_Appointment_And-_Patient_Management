#include "controller/AppointmentController.h"

#include "utils/ClinicExceptions.h"
#include "utils/IDGenerator.h"

namespace scapms::controller {

AppointmentController::AppointmentController(model::ClinicDatabase& db) : db_(db) {}

model::Appointment AppointmentController::scheduleAppointment(const std::string& patientId,
                                                                const std::string& doctorName,
                                                                const std::string& date,
                                                                const std::string& time,
                                                                const std::string& purpose,
                                                                const std::string& notes) {
    if (db_.findPatientRaw(patientId) == nullptr) {
        throw utils::InvalidAppointmentException(
            "Cannot schedule appointment: Patient ID '" + patientId + "' does not exist.");
    }

    const std::string aptId = utils::IDGenerator::nextAppointmentId();
    model::Appointment apt(aptId,
                           patientId,
                           doctorName,
                           date,
                           time,
                           purpose,
                           model::AppointmentStatus::Pending,
                           notes);
    db_.addAppointment(apt);
    return apt;
}

bool AppointmentController::cancelAppointment(const std::string& appointmentId) {
    if (appointmentId.empty()) {
        throw utils::InvalidAppointmentException("Appointment ID cannot be empty.");
    }
    return db_.cancelAppointment(appointmentId, true);
}

bool AppointmentController::removeAppointment(const std::string& appointmentId) {
    if (appointmentId.empty()) {
        throw utils::InvalidAppointmentException("Appointment ID cannot be empty.");
    }
    return db_.removeAppointment(appointmentId, true);
}

bool AppointmentController::completeAppointment(const std::string& appointmentId) {
    if (appointmentId.empty()) {
        throw utils::InvalidAppointmentException("Appointment ID cannot be empty.");
    }
    return db_.completeAppointment(appointmentId);
}

std::vector<model::Appointment> AppointmentController::getAppointmentsForPatient(
    const std::string& patientId) const {
    std::vector<model::Appointment> out;
    for (const auto& apt : db_.getAppointments()) {
        if (apt.getPatientId() == patientId) {
            out.push_back(apt);
        }
    }
    return out;
}

std::vector<model::Appointment> AppointmentController::getUpcomingPendingAppointments(
    const std::string& targetDate) const {
    std::vector<model::Appointment> out;
    for (const auto& apt : db_.getAppointments()) {
        if (apt.getStatus() == model::AppointmentStatus::Pending) {
            if (targetDate.empty() || apt.getDate() == targetDate) {
                out.push_back(apt);
            }
        }
    }
    return out;
}

} // namespace scapms::controller
