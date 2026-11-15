#ifndef SCAPMS_CONTROLLER_APPOINTMENT_CONTROLLER_H
#define SCAPMS_CONTROLLER_APPOINTMENT_CONTROLLER_H

#include "model/Appointment.h"
#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

/**
 * @brief Coordinates appointment scheduling, completion, cancellation, and lookup.
 */
class AppointmentController {
public:
    explicit AppointmentController(model::ClinicDatabase& db);

    model::Appointment scheduleAppointment(const std::string& patientId,
                                           const std::string& doctorName,
                                           const std::string& date,
                                           const std::string& time,
                                           const std::string& purpose,
                                           const std::string& notes = "");

    bool cancelAppointment(const std::string& appointmentId);
    bool removeAppointment(const std::string& appointmentId);
    bool completeAppointment(const std::string& appointmentId);

    std::vector<model::Appointment> getAppointmentsForPatient(const std::string& patientId) const;
    std::vector<model::Appointment> getUpcomingPendingAppointments(const std::string& targetDate = "") const;

private:
    model::ClinicDatabase& db_;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_APPOINTMENT_CONTROLLER_H
