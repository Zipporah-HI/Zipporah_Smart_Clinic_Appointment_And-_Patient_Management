#ifndef SCAPMS_MODEL_APPOINTABLE_H
#define SCAPMS_MODEL_APPOINTABLE_H

#include <string>

namespace scapms::model {

/**
 * @brief Interface-like abstract class defining scheduling operations for
 *        patient types that can be scheduled for clinic appointments.
 */
class Appointable {
public:
    virtual ~Appointable() = default;

    /**
     * @brief Associates a newly scheduled appointment ID with the patient.
     * @return true if scheduling succeeded under the patient type's policy.
     */
    virtual bool scheduleAppointment(const std::string& appointmentId) = 0;

    /**
     * @brief Removes or marks cancelled an appointment associated with the patient.
     * @return true if the appointment was found and cancelled.
     */
    virtual bool cancelAppointment(const std::string& appointmentId) = 0;

    /**
     * @brief Describes the scheduling rules specific to this patient category.
     */
    virtual std::string getSchedulingPolicy() const = 0;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_APPOINTABLE_H
