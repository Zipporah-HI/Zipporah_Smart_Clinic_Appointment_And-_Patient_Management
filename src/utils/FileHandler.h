#ifndef SCAPMS_UTILS_FILE_HANDLER_H
#define SCAPMS_UTILS_FILE_HANDLER_H

#include "model/ClinicDatabase.h"

#include <string>

namespace scapms::utils {

/**
 * @brief Handles persistent CSV saving and loading of polymorphic PatientRecord
 *        subclasses and Appointment records.
 */
class FileHandler {
public:
    static void savePatientsCSV(const model::ClinicDatabase& db, const std::string& filePath);
    static void saveAppointmentsCSV(const model::ClinicDatabase& db, const std::string& filePath);

    static void loadPatientsCSV(model::ClinicDatabase& db, const std::string& filePath);
    static void loadAppointmentsCSV(model::ClinicDatabase& db, const std::string& filePath);

    static void saveClinicBundle(const model::ClinicDatabase& db,
                                 const std::string& patientsPath,
                                 const std::string& appointmentsPath);
    static void loadClinicBundle(model::ClinicDatabase& db,
                                 const std::string& patientsPath,
                                 const std::string& appointmentsPath);

    static std::string escapeCSV(const std::string& field);
    static std::vector<std::string> parseCSVLine(const std::string& line);
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_FILE_HANDLER_H
