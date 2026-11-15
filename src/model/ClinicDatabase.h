#ifndef SCAPMS_MODEL_CLINIC_DATABASE_H
#define SCAPMS_MODEL_CLINIC_DATABASE_H

#include "model/Appointment.h"
#include "model/PatientRecord.h"

#include <cstddef>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include <vector>

namespace scapms::model {

enum class SortField {
    None,
    PatientId,
    Name,
    Age,
    AppointmentDate
};

enum class AdminActionType {
    DeletePatient,
    ModifyPatient,
    RemoveAppointment
};

struct AdminAction {
    AdminActionType type;
    std::string description;
    std::unique_ptr<PatientRecord> patientSnapshot;
    std::vector<Appointment> relatedAppointments;
    Appointment appointmentSnapshot;
};

/**
 * @brief Encapsulates all clinic domain collections:
 *        - std::vector of polymorphic PatientRecord pointers
 *        - std::vector of Appointment objects
 *        - std::queue for consultation waiting room (FIFO)
 *        - std::stack for Undo Last Administrative Operation
 *        - Fixed-size C++ array (PatientRecord*[10]) cache for frequently accessed patients
 */
class ClinicDatabase {
public:
    static constexpr std::size_t CACHE_CAPACITY = 10;

    ClinicDatabase();

    // Patient collection management
    void addPatient(std::unique_ptr<PatientRecord> patient);
    bool removePatient(const std::string& patientId, bool recordUndo = true);
    void updatePatient(const std::string& patientId,
                       std::unique_ptr<PatientRecord> updatedRecord,
                       bool recordUndo = true);
    void clearAll();

    PatientRecord* findPatientRaw(const std::string& patientId);
    const PatientRecord* findPatientRaw(const std::string& patientId) const;
    std::vector<std::unique_ptr<PatientRecord>>& getPatientsMutable();
    const std::vector<std::unique_ptr<PatientRecord>>& getPatients() const;
    std::size_t getPatientCount() const;

    // Appointment collection management
    void addAppointment(const Appointment& appointment);
    bool removeAppointment(const std::string& appointmentId, bool recordUndo = true);
    bool cancelAppointment(const std::string& appointmentId, bool recordUndo = true);
    bool completeAppointment(const std::string& appointmentId);
    Appointment* findAppointmentRaw(const std::string& appointmentId);
    const Appointment* findAppointmentRaw(const std::string& appointmentId) const;
    std::vector<Appointment>& getAppointmentsMutable();
    const std::vector<Appointment>& getAppointments() const;
    std::string getEarliestPendingOrLatestAppointmentDate(const std::string& patientId) const;

    // Waiting queue (std::queue FIFO)
    void enqueuePatient(const std::string& patientId);
    std::string dequeueNextPatient();
    bool isPatientInQueue(const std::string& patientId) const;
    void removePatientFromQueue(const std::string& patientId);
    std::vector<std::string> getQueueSnapshot() const;
    std::size_t getQueueSize() const;

    // Undo stack (std::stack)
    bool canUndo() const;
    std::size_t getUndoStackSize() const;
    std::string peekUndoDescription() const;
    std::vector<std::string> getUndoHistoryDescriptions() const;
    std::string undoLastAction();

    // Frequently accessed cache (fixed-size C++ array)
    void recordPatientAccess(PatientRecord* patient);
    void invalidateCachePointer(const PatientRecord* patient);
    void rebuildCache();
    std::vector<PatientRecord*> getFrequentlyAccessedPatients() const;

    // Sorted state tracking for Searching & Sorting coordination
    SortField getSortedState() const;
    void setSortedState(SortField field);

private:
    std::vector<std::unique_ptr<PatientRecord>> patients_;
    std::vector<Appointment> appointments_;
    std::queue<std::string> waitingQueue_;
    std::stack<AdminAction> undoStack_;
    PatientRecord* recentPatients_[CACHE_CAPACITY];
    std::size_t cacheCount_ = 0;
    SortField sortedState_ = SortField::None;
};

} // namespace scapms::model

#endif // SCAPMS_MODEL_CLINIC_DATABASE_H
