#ifndef SCAPMS_CONTROLLER_CLINIC_MANAGER_H
#define SCAPMS_CONTROLLER_CLINIC_MANAGER_H

#include "controller/AppointmentController.h"
#include "controller/SearchEngine.h"
#include "model/ClinicDatabase.h"

#include <string>
#include <vector>

namespace scapms::controller {

struct PatientAttendanceRow {
    std::string patientId;
    std::string name;
    std::string category;
    int completedCount = 0;
};

struct PendingPatientRow {
    std::string patientId;
    std::string name;
    std::string category;
    int pendingCount = 0;
    std::string nextDateTime;
};

struct CategoryDistributionReport {
    int generalCount = 0;
    int emergencyCount = 0;
    int followUpCount = 0;
    int totalPatients = 0;
};

struct DailyAppointmentSummaryReport {
    std::string dateFilter; // Empty means all dates
    int totalAppointments = 0;
    int completedCount = 0;
    int pendingCount = 0;
    int cancelledCount = 0;
};

/**
 * @brief Top-level clinic business-logic coordinator separating domain models
 *        and algorithms from the GUI presentation layer.
 */
class ClinicManager {
public:
    ClinicManager();

    model::ClinicDatabase& getDatabase();
    const model::ClinicDatabase& getDatabase() const;
    AppointmentController& getAppointmentController();
    const AppointmentController& getAppointmentController() const;

    // Patient registration & management
    std::string registerPatient(const std::string& category,
                                const std::string& customId,
                                const std::string& name,
                                int age,
                                const std::string& contactInfo,
                                const std::string& extra1,
                                const std::string& extra2);

    void updatePatient(const std::string& patientId,
                       const std::string& category,
                       const std::string& name,
                       int age,
                       const std::string& contactInfo,
                       const std::string& extra1,
                       const std::string& extra2);

    bool deletePatient(const std::string& patientId);

    // Explicit runtime polymorphism demonstration
    static std::string processPatientPolymorphically(const model::PatientRecord& patient);
    std::vector<std::string> generatePolymorphicAssessmentLog() const;

    // Waiting queue management
    void addToWaitingQueue(const std::string& patientId);
    model::PatientRecord* callNextPatient();

    // Administrative undo
    std::string undoLastOperation();

    // Reports (PRD Section: Simple Reports)
    std::vector<PatientAttendanceRow> getMostFrequentlyAttendedPatients() const;
    std::vector<PendingPatientRow> getPatientsWithPendingAppointments() const;
    CategoryDistributionReport getCategoryDistribution() const;
    DailyAppointmentSummaryReport getDailyAppointmentSummary(const std::string& date = "") const;

    // Persistence & initialization
    void saveClinicData(const std::string& patientsPath, const std::string& appointmentsPath) const;
    void loadClinicData(const std::string& patientsPath, const std::string& appointmentsPath);
    void startNewSession();
    void seedSampleClinicData();

private:
    static std::unique_ptr<model::PatientRecord> buildPatientByCategory(
        const std::string& category,
        const std::string& id,
        const std::string& name,
        int age,
        const std::string& contactInfo,
        const std::string& extra1,
        const std::string& extra2);

    model::ClinicDatabase db_;
    AppointmentController appointmentController_;
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_CLINIC_MANAGER_H
