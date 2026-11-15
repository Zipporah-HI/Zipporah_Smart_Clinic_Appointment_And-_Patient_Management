#ifndef SCAPMS_UTILS_ID_GENERATOR_H
#define SCAPMS_UTILS_ID_GENERATOR_H

#include <string>

namespace scapms::utils {

/**
 * @brief Generates unique, human-readable identifiers for patients and appointments.
 */
class IDGenerator {
public:
    static std::string nextPatientId();
    static std::string nextAppointmentId();

    static void syncPatientCounter(int maxExistingNumber);
    static void syncAppointmentCounter(int maxExistingNumber);
    static void resetCounters();

    static int extractNumericSuffix(const std::string& id);

private:
    static int patientCounter_;
    static int appointmentCounter_;
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_ID_GENERATOR_H
