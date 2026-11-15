#include "controller/ClinicManager.h"

#include "model/Appointable.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicExceptions.h"
#include "utils/FileHandler.h"
#include "utils/IDGenerator.h"

#include <memory>

namespace scapms::controller {

ClinicManager::ClinicManager() : appointmentController_(db_) {}

model::ClinicDatabase& ClinicManager::getDatabase() { return db_; }
const model::ClinicDatabase& ClinicManager::getDatabase() const { return db_; }
AppointmentController& ClinicManager::getAppointmentController() { return appointmentController_; }
const AppointmentController& ClinicManager::getAppointmentController() const {
    return appointmentController_;
}

std::unique_ptr<model::PatientRecord> ClinicManager::buildPatientByCategory(
    const std::string& category,
    const std::string& id,
    const std::string& name,
    int age,
    const std::string& contactInfo,
    const std::string& extra1,
    const std::string& extra2) {
    if (category == "General") {
        return std::make_unique<model::GeneralPatient>(id, name, age, contactInfo, extra1, extra2);
    }
    if (category == "Emergency") {
        return std::make_unique<model::EmergencyPatient>(id, name, age, contactInfo, extra1, extra2);
    }
    if (category == "Follow-Up") {
        return std::make_unique<model::FollowUpPatient>(id, name, age, contactInfo, extra1, extra2);
    }
    throw utils::InvalidPatientException("Unsupported patient category: " + category);
}

std::string ClinicManager::registerPatient(const std::string& category,
                                           const std::string& customId,
                                           const std::string& name,
                                           int age,
                                           const std::string& contactInfo,
                                           const std::string& extra1,
                                           const std::string& extra2) {
    const std::string assignedId =
        customId.empty() ? utils::IDGenerator::nextPatientId() : customId;
    if (!customId.empty()) {
        const int suffix = utils::IDGenerator::extractNumericSuffix(customId);
        utils::IDGenerator::syncPatientCounter(suffix);
    }

    auto record = buildPatientByCategory(category, assignedId, name, age, contactInfo, extra1, extra2);
    db_.addPatient(std::move(record));
    return assignedId;
}

void ClinicManager::updatePatient(const std::string& patientId,
                                  const std::string& category,
                                  const std::string& name,
                                  int age,
                                  const std::string& contactInfo,
                                  const std::string& extra1,
                                  const std::string& extra2) {
    auto updated = buildPatientByCategory(category, patientId, name, age, contactInfo, extra1, extra2);
    db_.updatePatient(patientId, std::move(updated), true);
}

bool ClinicManager::deletePatient(const std::string& patientId) {
    if (patientId.empty()) {
        throw utils::InvalidPatientException("Please select a valid Patient ID to delete.");
    }
    return db_.removePatient(patientId, true);
}

std::string ClinicManager::processPatientPolymorphically(const model::PatientRecord& patient) {
    // Demonstrates runtime polymorphism via base-class reference & dynamic interface check
    std::string policy = "Non-schedulable";
    if (const auto* appointable = dynamic_cast<const model::Appointable*>(&patient)) {
        policy = appointable->getSchedulingPolicy();
    }
    return "[" + patient.getPatientId() + "] " + patient.getName() +
           " | Category: " + patient.getPatientCategory() +
           " | Priority: " + patient.getPriorityLabel() +
           " (Level " + std::to_string(patient.getPriorityLevel()) + ")" +
           " | " + patient.getDetailsSummary() +
           " | Policy: " + policy;
}

std::vector<std::string> ClinicManager::generatePolymorphicAssessmentLog() const {
    std::vector<std::string> log;
    log.reserve(db_.getPatientCount());
    for (const auto& patientPtr : db_.getPatients()) {
        if (patientPtr) {
            log.push_back(processPatientPolymorphically(*patientPtr));
        }
    }
    return log;
}

void ClinicManager::addToWaitingQueue(const std::string& patientId) {
    db_.enqueuePatient(patientId);
}

model::PatientRecord* ClinicManager::callNextPatient() {
    const std::string nextId = db_.dequeueNextPatient();
    return db_.findPatientRaw(nextId);
}

std::string ClinicManager::undoLastOperation() {
    return db_.undoLastAction();
}

std::vector<PatientAttendanceRow> ClinicManager::getMostFrequentlyAttendedPatients() const {
    std::vector<PatientAttendanceRow> rows;
    for (const auto& patient : db_.getPatients()) {
        if (!patient) {
            continue;
        }
        int completed = 0;
        for (const auto& apt : db_.getAppointments()) {
            if (apt.getPatientId() == patient->getPatientId() &&
                apt.getStatus() == model::AppointmentStatus::Completed) {
                ++completed;
            }
        }
        if (completed > 0) {
            rows.push_back({patient->getPatientId(),
                            patient->getName(),
                            patient->getPatientCategory(),
                            completed});
        }
    }

    // Manual insertion sort descending by completedCount (no std::sort)
    for (std::size_t i = 1; i < rows.size(); ++i) {
        PatientAttendanceRow key = rows[i];
        std::size_t j = i;
        while (j > 0 && rows[j - 1].completedCount < key.completedCount) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }
    return rows;
}

std::vector<PendingPatientRow> ClinicManager::getPatientsWithPendingAppointments() const {
    std::vector<PendingPatientRow> rows;
    for (const auto& patient : db_.getPatients()) {
        if (!patient) {
            continue;
        }
        int pending = 0;
        std::string earliest;
        for (const auto& apt : db_.getAppointments()) {
            if (apt.getPatientId() == patient->getPatientId() &&
                apt.getStatus() == model::AppointmentStatus::Pending) {
                ++pending;
                const std::string dt = apt.getDate() + " " + apt.getTime();
                if (earliest.empty() || dt < earliest) {
                    earliest = dt;
                }
            }
        }
        if (pending > 0) {
            rows.push_back({patient->getPatientId(),
                            patient->getName(),
                            patient->getPatientCategory(),
                            pending,
                            earliest});
        }
    }

    // Manual insertion sort by nextDateTime ascending
    for (std::size_t i = 1; i < rows.size(); ++i) {
        PendingPatientRow key = rows[i];
        std::size_t j = i;
        while (j > 0 && rows[j - 1].nextDateTime > key.nextDateTime) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = key;
    }
    return rows;
}

CategoryDistributionReport ClinicManager::getCategoryDistribution() const {
    CategoryDistributionReport report;
    for (const auto& patient : db_.getPatients()) {
        if (!patient) {
            continue;
        }
        ++report.totalPatients;
        const std::string cat = patient->getPatientCategory();
        if (cat == "General") {
            ++report.generalCount;
        } else if (cat == "Emergency") {
            ++report.emergencyCount;
        } else if (cat == "Follow-Up") {
            ++report.followUpCount;
        }
    }
    return report;
}

DailyAppointmentSummaryReport ClinicManager::getDailyAppointmentSummary(const std::string& date) const {
    DailyAppointmentSummaryReport report;
    report.dateFilter = date;
    for (const auto& apt : db_.getAppointments()) {
        if (!date.empty() && apt.getDate() != date) {
            continue;
        }
        ++report.totalAppointments;
        switch (apt.getStatus()) {
            case model::AppointmentStatus::Completed:
                ++report.completedCount;
                break;
            case model::AppointmentStatus::Pending:
                ++report.pendingCount;
                break;
            case model::AppointmentStatus::Cancelled:
                ++report.cancelledCount;
                break;
        }
    }
    return report;
}

void ClinicManager::saveClinicData(const std::string& patientsPath,
                                   const std::string& appointmentsPath) const {
    utils::FileHandler::saveClinicBundle(db_, patientsPath, appointmentsPath);
}

void ClinicManager::loadClinicData(const std::string& patientsPath,
                                   const std::string& appointmentsPath) {
    utils::FileHandler::loadClinicBundle(db_, patientsPath, appointmentsPath);
}

void ClinicManager::startNewSession() {
    db_.clearAll();
    utils::IDGenerator::resetCounters();
}

void ClinicManager::seedSampleClinicData() {
    startNewSession();

    const std::string p1 = registerPatient("General", "PAT-1001", "Aisha Mohammed", 21,
                                           "+234-803-112-4401", "Annual Wellness Check", "Dr. Amina Bello");
    const std::string p2 = registerPatient("Emergency", "PAT-1002", "Chukwuemeka Obi", 24,
                                           "+234-809-441-9920", "Critical - Acute Asthma", "Grace Obi (Mother)");
    const std::string p3 = registerPatient("Follow-Up", "PAT-1003", "Toluwani Adeyemi", 19,
                                           "+234-812-300-7715", "2026-10-28", "Dr. Chidi Okafor");
    const std::string p4 = registerPatient("General", "PAT-1004", "Zainab Ibrahim", 22,
                                           "+234-706-881-2304", "Persistent Migraine", "Dr. Amina Bello");
    const std::string p5 = registerPatient("Emergency", "PAT-1005", "Daniel Etuk", 20,
                                           "+234-805-612-9083", "Urgent - Sports Ankle Sprain", "Hostel Warden");
    const std::string p6 = registerPatient("Follow-Up", "PAT-1006", "Ngozi Nwosu", 23,
                                           "+234-816-225-3189", "2026-11-02", "Dr. Fatima Yusuf");
    const std::string p7 = registerPatient("General", "PAT-1007", "Samuel Olatunji", 25,
                                           "+234-802-774-1190", "Lab Result Review", "Dr. Chidi Okafor");
    const std::string p8 = registerPatient("Follow-Up", "PAT-1008", "Khadija Musa", 20,
                                           "+234-813-990-5412", "2026-11-05", "Dr. Amina Bello");

    // Seed appointments across Completed, Pending, and Cancelled statuses
    auto a1 = appointmentController_.scheduleAppointment(
        p1, "Dr. Amina Bello", "2026-11-10", "09:00", "Routine Physical Exam", "Vitals normal");
    appointmentController_.completeAppointment(a1.getAppointmentId());

    auto a2 = appointmentController_.scheduleAppointment(
        p1, "Dr. Amina Bello", "2026-11-13", "11:30", "Blood Panel Follow-Up", "Fasting requested");
    appointmentController_.completeAppointment(a2.getAppointmentId());

    auto a3 = appointmentController_.scheduleAppointment(
        p2, "Dr. Fatima Yusuf", "2026-11-13", "08:45", "Emergency Nebulizer Stabilization", "Priority 1 triage");
    appointmentController_.completeAppointment(a3.getAppointmentId());

    auto a4 = appointmentController_.scheduleAppointment(
        p3, "Dr. Chidi Okafor", "2026-11-13", "14:00", "Post-Antibiotic Review", "Bring prescription sheet");
    (void)a4;

    auto a5 = appointmentController_.scheduleAppointment(
        p4, "Dr. Amina Bello", "2026-11-13", "15:15", "Neurology Screening", "Migraine diary check");
    (void)a5;

    auto a6 = appointmentController_.scheduleAppointment(
        p5, "Dr. Fatima Yusuf", "2026-11-13", "10:15", "X-Ray Ankle Assessment", "Ice & compression applied");
    appointmentController_.completeAppointment(a6.getAppointmentId());

    auto a7 = appointmentController_.scheduleAppointment(
        p6, "Dr. Fatima Yusuf", "2026-11-14", "09:30", "Physiotherapy Follow-Up", "Check mobility range");
    (void)a7;

    auto a8 = appointmentController_.scheduleAppointment(
        p7, "Dr. Chidi Okafor", "2026-11-12", "16:00", "Dermatology Consult", "Rescheduled by student");
    db_.cancelAppointment(a8.getAppointmentId(), false);

    auto a9 = appointmentController_.scheduleAppointment(
        p8, "Dr. Amina Bello", "2026-11-13", "16:30", "Iron Supplement Check", "Pending lab confirmation");
    (void)a9;

    auto a10 = appointmentController_.scheduleAppointment(
        p3, "Dr. Chidi Okafor", "2026-11-01", "10:00", "Initial Fever Consult", "Recovered well");
    appointmentController_.completeAppointment(a10.getAppointmentId());

    // Pre-populate waiting queue with 3 patients
    addToWaitingQueue(p2);
    addToWaitingQueue(p4);
    addToWaitingQueue(p3);
}

} // namespace scapms::controller
