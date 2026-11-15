#include "model/ClinicDatabase.h"

#include "model/Appointable.h"
#include "utils/ClinicExceptions.h"

namespace scapms::model {

ClinicDatabase::ClinicDatabase() {
    for (std::size_t i = 0; i < CACHE_CAPACITY; ++i) {
        recentPatients_[i] = nullptr;
    }
}

void ClinicDatabase::addPatient(std::unique_ptr<PatientRecord> patient) {
    if (!patient) {
        throw utils::InvalidPatientException("Cannot add a null patient record.");
    }
    if (findPatientRaw(patient->getPatientId()) != nullptr) {
        throw utils::DuplicatePatientIdException(patient->getPatientId());
    }
    PatientRecord* rawPtr = patient.get();
    patients_.push_back(std::move(patient));
    sortedState_ = SortField::None;
    recordPatientAccess(rawPtr);
}

bool ClinicDatabase::removePatient(const std::string& patientId, bool recordUndo) {
    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->getPatientId() == patientId) {
            PatientRecord* dyingPtr = patients_[i].get();
            invalidateCachePointer(dyingPtr);
            removePatientFromQueue(patientId);

            std::vector<Appointment> removedAppointments;
            for (std::size_t a = 0; a < appointments_.size();) {
                if (appointments_[a].getPatientId() == patientId) {
                    removedAppointments.push_back(appointments_[a]);
                    appointments_.erase(appointments_.begin() + static_cast<std::ptrdiff_t>(a));
                } else {
                    ++a;
                }
            }

            if (recordUndo) {
                AdminAction action;
                action.type = AdminActionType::DeletePatient;
                action.description = "Deleted patient " + patients_[i]->getName() +
                                     " (" + patientId + ")";
                action.patientSnapshot = patients_[i]->clone();
                action.relatedAppointments = std::move(removedAppointments);
                undoStack_.push(std::move(action));
            }

            patients_.erase(patients_.begin() + static_cast<std::ptrdiff_t>(i));
            sortedState_ = SortField::None;
            return true;
        }
    }
    return false;
}

void ClinicDatabase::updatePatient(const std::string& patientId,
                                   std::unique_ptr<PatientRecord> updatedRecord,
                                   bool recordUndo) {
    if (!updatedRecord) {
        throw utils::InvalidPatientException("Updated patient record cannot be null.");
    }
    if (updatedRecord->getPatientId() != patientId) {
        throw utils::InvalidPatientException("Cannot change existing Patient ID during update.");
    }

    for (std::size_t i = 0; i < patients_.size(); ++i) {
        if (patients_[i]->getPatientId() == patientId) {
            // Preserve appointment links and access count
            for (const auto& aptId : patients_[i]->getAppointmentIds()) {
                updatedRecord->addAppointmentId(aptId);
            }
            updatedRecord->setAccessCount(patients_[i]->getAccessCount());

            PatientRecord* oldPtr = patients_[i].get();
            invalidateCachePointer(oldPtr);

            if (recordUndo) {
                AdminAction action;
                action.type = AdminActionType::ModifyPatient;
                action.description = "Modified patient " + patients_[i]->getName() +
                                     " (" + patientId + ")";
                action.patientSnapshot = patients_[i]->clone();
                undoStack_.push(std::move(action));
            }

            patients_[i] = std::move(updatedRecord);
            recordPatientAccess(patients_[i].get());
            sortedState_ = SortField::None;
            return;
        }
    }
    throw utils::InvalidPatientException("Patient ID not found: " + patientId);
}

void ClinicDatabase::clearAll() {
    patients_.clear();
    appointments_.clear();
    while (!waitingQueue_.empty()) {
        waitingQueue_.pop();
    }
    while (!undoStack_.empty()) {
        undoStack_.pop();
    }
    for (std::size_t i = 0; i < CACHE_CAPACITY; ++i) {
        recentPatients_[i] = nullptr;
    }
    cacheCount_ = 0;
    sortedState_ = SortField::None;
}

PatientRecord* ClinicDatabase::findPatientRaw(const std::string& patientId) {
    for (const auto& patient : patients_) {
        if (patient && patient->getPatientId() == patientId) {
            return patient.get();
        }
    }
    return nullptr;
}

const PatientRecord* ClinicDatabase::findPatientRaw(const std::string& patientId) const {
    for (const auto& patient : patients_) {
        if (patient && patient->getPatientId() == patientId) {
            return patient.get();
        }
    }
    return nullptr;
}

std::vector<std::unique_ptr<PatientRecord>>& ClinicDatabase::getPatientsMutable() {
    return patients_;
}

const std::vector<std::unique_ptr<PatientRecord>>& ClinicDatabase::getPatients() const {
    return patients_;
}

std::size_t ClinicDatabase::getPatientCount() const {
    return patients_.size();
}

void ClinicDatabase::addAppointment(const Appointment& appointment) {
    if (findAppointmentRaw(appointment.getAppointmentId()) != nullptr) {
        throw utils::InvalidAppointmentException(
            "Appointment ID already exists: " + appointment.getAppointmentId());
    }
    PatientRecord* patient = findPatientRaw(appointment.getPatientId());
    if (!patient) {
        throw utils::InvalidAppointmentException(
            "Cannot schedule appointment: Patient ID not found (" +
            appointment.getPatientId() + ").");
    }

    auto* appointable = dynamic_cast<Appointable*>(patient);
    if (!appointable) {
        throw utils::InvalidAppointmentException(
            "Patient category '" + patient->getPatientCategory() +
            "' does not support appointment scheduling.");
    }

    if (appointment.getStatus() != AppointmentStatus::Cancelled) {
        appointable->scheduleAppointment(appointment.getAppointmentId());
    }
    appointments_.push_back(appointment);
    recordPatientAccess(patient);
}

bool ClinicDatabase::removeAppointment(const std::string& appointmentId, bool recordUndo) {
    for (std::size_t i = 0; i < appointments_.size(); ++i) {
        if (appointments_[i].getAppointmentId() == appointmentId) {
            const std::string patientId = appointments_[i].getPatientId();
            if (PatientRecord* patient = findPatientRaw(patientId)) {
                if (auto* appointable = dynamic_cast<Appointable*>(patient)) {
                    appointable->cancelAppointment(appointmentId);
                } else {
                    patient->removeAppointmentId(appointmentId);
                }
            }

            if (recordUndo) {
                AdminAction action;
                action.type = AdminActionType::RemoveAppointment;
                action.description = "Removed appointment " + appointmentId +
                                     " (Patient " + patientId + ")";
                action.appointmentSnapshot = appointments_[i];
                undoStack_.push(std::move(action));
            }

            appointments_.erase(appointments_.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    }
    return false;
}

bool ClinicDatabase::cancelAppointment(const std::string& appointmentId, bool recordUndo) {
    Appointment* apt = findAppointmentRaw(appointmentId);
    if (!apt) {
        return false;
    }
    if (apt->getStatus() == AppointmentStatus::Cancelled) {
        return false;
    }

    if (recordUndo) {
        AdminAction action;
        action.type = AdminActionType::RemoveAppointment;
        action.description = "Cancelled appointment " + appointmentId +
                             " (Patient " + apt->getPatientId() + ")";
        action.appointmentSnapshot = *apt;
        undoStack_.push(std::move(action));
    }

    apt->setStatus(AppointmentStatus::Cancelled);
    if (PatientRecord* patient = findPatientRaw(apt->getPatientId())) {
        if (auto* appointable = dynamic_cast<Appointable*>(patient)) {
            appointable->cancelAppointment(appointmentId);
        }
    }
    return true;
}

bool ClinicDatabase::completeAppointment(const std::string& appointmentId) {
    Appointment* apt = findAppointmentRaw(appointmentId);
    if (!apt) {
        return false;
    }
    apt->setStatus(AppointmentStatus::Completed);
    if (PatientRecord* patient = findPatientRaw(apt->getPatientId())) {
        recordPatientAccess(patient);
    }
    return true;
}

Appointment* ClinicDatabase::findAppointmentRaw(const std::string& appointmentId) {
    for (auto& apt : appointments_) {
        if (apt.getAppointmentId() == appointmentId) {
            return &apt;
        }
    }
    return nullptr;
}

const Appointment* ClinicDatabase::findAppointmentRaw(const std::string& appointmentId) const {
    for (const auto& apt : appointments_) {
        if (apt.getAppointmentId() == appointmentId) {
            return &apt;
        }
    }
    return nullptr;
}

std::vector<Appointment>& ClinicDatabase::getAppointmentsMutable() {
    return appointments_;
}

const std::vector<Appointment>& ClinicDatabase::getAppointments() const {
    return appointments_;
}

std::string ClinicDatabase::getEarliestPendingOrLatestAppointmentDate(const std::string& patientId) const {
    std::string earliestPending;
    std::string latestAny;
    for (const auto& apt : appointments_) {
        if (apt.getPatientId() != patientId) {
            continue;
        }
        const std::string stamp = apt.getDate() + " " + apt.getTime();
        if (apt.getStatus() == AppointmentStatus::Pending) {
            if (earliestPending.empty() || stamp < earliestPending) {
                earliestPending = stamp;
            }
        }
        if (latestAny.empty() || stamp > latestAny) {
            latestAny = stamp;
        }
    }
    if (!earliestPending.empty()) {
        return earliestPending;
    }
    return latestAny.empty() ? "9999-99-99 99:99" : latestAny;
}

void ClinicDatabase::enqueuePatient(const std::string& patientId) {
    PatientRecord* patient = findPatientRaw(patientId);
    if (!patient) {
        throw utils::InvalidPatientException("Cannot enqueue unknown Patient ID: " + patientId);
    }
    if (isPatientInQueue(patientId)) {
        throw utils::InvalidPatientException("Patient " + patientId + " is already in the waiting queue.");
    }
    waitingQueue_.push(patientId);
    recordPatientAccess(patient);
}

std::string ClinicDatabase::dequeueNextPatient() {
    if (waitingQueue_.empty()) {
        throw utils::ClinicException("Waiting queue is currently empty.");
    }
    std::string nextId = waitingQueue_.front();
    waitingQueue_.pop();
    if (PatientRecord* patient = findPatientRaw(nextId)) {
        recordPatientAccess(patient);
    }
    return nextId;
}

bool ClinicDatabase::isPatientInQueue(const std::string& patientId) const {
    std::queue<std::string> copy = waitingQueue_;
    while (!copy.empty()) {
        if (copy.front() == patientId) {
            return true;
        }
        copy.pop();
    }
    return false;
}

void ClinicDatabase::removePatientFromQueue(const std::string& patientId) {
    std::queue<std::string> filtered;
    while (!waitingQueue_.empty()) {
        if (waitingQueue_.front() != patientId) {
            filtered.push(waitingQueue_.front());
        }
        waitingQueue_.pop();
    }
    waitingQueue_ = std::move(filtered);
}

std::vector<std::string> ClinicDatabase::getQueueSnapshot() const {
    std::vector<std::string> result;
    std::queue<std::string> copy = waitingQueue_;
    while (!copy.empty()) {
        result.push_back(copy.front());
        copy.pop();
    }
    return result;
}

std::size_t ClinicDatabase::getQueueSize() const {
    return waitingQueue_.size();
}

bool ClinicDatabase::canUndo() const {
    return !undoStack_.empty();
}

std::size_t ClinicDatabase::getUndoStackSize() const {
    return undoStack_.size();
}

std::string ClinicDatabase::peekUndoDescription() const {
    if (undoStack_.empty()) {
        return "Nothing to undo";
    }
    return undoStack_.top().description;
}

std::vector<std::string> ClinicDatabase::getUndoHistoryDescriptions() const {
    std::vector<std::string> items;
    std::stack<AdminAction> copy;
    // Note: AdminAction contains unique_ptr, so we cannot copy the stack directly.
    // Instead, we inspect only the top or maintain a parallel description list?
    // Wait: let's return the top description safely or store a non-owning summary.
    if (!undoStack_.empty()) {
        items.push_back("Top of Stack: " + undoStack_.top().description);
    }
    return items;
}

std::string ClinicDatabase::undoLastAction() {
    if (undoStack_.empty()) {
        throw utils::ClinicException("Undo stack is empty. No administrative action to reverse.");
    }

    AdminAction action = std::move(undoStack_.top());
    undoStack_.pop();

    switch (action.type) {
        case AdminActionType::DeletePatient: {
            if (!action.patientSnapshot) {
                throw utils::ClinicException("Corrupted undo snapshot for deleted patient.");
            }
            const std::string restoredId = action.patientSnapshot->getPatientId();
            if (findPatientRaw(restoredId) != nullptr) {
                throw utils::DuplicatePatientIdException(restoredId);
            }
            PatientRecord* rawPtr = action.patientSnapshot.get();
            patients_.push_back(std::move(action.patientSnapshot));
            for (const auto& apt : action.relatedAppointments) {
                if (findAppointmentRaw(apt.getAppointmentId()) == nullptr) {
                    appointments_.push_back(apt);
                }
            }
            recordPatientAccess(rawPtr);
            sortedState_ = SortField::None;
            return "Undid: " + action.description;
        }
        case AdminActionType::ModifyPatient: {
            if (!action.patientSnapshot) {
                throw utils::ClinicException("Corrupted undo snapshot for modified patient.");
            }
            const std::string targetId = action.patientSnapshot->getPatientId();
            updatePatient(targetId, std::move(action.patientSnapshot), false);
            return "Undid: " + action.description;
        }
        case AdminActionType::RemoveAppointment: {
            const std::string aptId = action.appointmentSnapshot.getAppointmentId();
            Appointment* existing = findAppointmentRaw(aptId);
            if (existing) {
                *existing = action.appointmentSnapshot;
                if (PatientRecord* p = findPatientRaw(existing->getPatientId())) {
                    if (existing->getStatus() != AppointmentStatus::Cancelled) {
                        if (auto* appointable = dynamic_cast<Appointable*>(p)) {
                            appointable->scheduleAppointment(aptId);
                        }
                    }
                }
            } else {
                addAppointment(action.appointmentSnapshot);
            }
            return "Undid: " + action.description;
        }
    }

    return "Undid administrative action.";
}

void ClinicDatabase::recordPatientAccess(PatientRecord* patient) {
    if (!patient) {
        return;
    }
    patient->incrementAccessCount();

    // Check if already in fixed-size array cache
    std::size_t existingIdx = cacheCount_;
    for (std::size_t i = 0; i < cacheCount_; ++i) {
        if (recentPatients_[i] == patient) {
            existingIdx = i;
            break;
        }
    }

    if (existingIdx < cacheCount_) {
        // Move to front (index 0)
        for (std::size_t i = existingIdx; i > 0; --i) {
            recentPatients_[i] = recentPatients_[i - 1];
        }
        recentPatients_[0] = patient;
    } else {
        // Shift right and insert at index 0, evicting oldest/least-recent if full
        const std::size_t limit = (cacheCount_ < CACHE_CAPACITY) ? cacheCount_ : (CACHE_CAPACITY - 1);
        for (std::size_t i = limit; i > 0; --i) {
            recentPatients_[i] = recentPatients_[i - 1];
        }
        recentPatients_[0] = patient;
        if (cacheCount_ < CACHE_CAPACITY) {
            ++cacheCount_;
        }
    }

    // Re-order cache by accessCount descending (manual insertion pass on <=10 items)
    for (std::size_t i = 1; i < cacheCount_; ++i) {
        PatientRecord* key = recentPatients_[i];
        std::size_t j = i;
        while (j > 0 && recentPatients_[j - 1]->getAccessCount() < key->getAccessCount()) {
            recentPatients_[j] = recentPatients_[j - 1];
            --j;
        }
        recentPatients_[j] = key;
    }
}

void ClinicDatabase::invalidateCachePointer(const PatientRecord* patient) {
    if (!patient) {
        return;
    }
    for (std::size_t i = 0; i < cacheCount_; ++i) {
        if (recentPatients_[i] == patient) {
            for (std::size_t j = i; j + 1 < cacheCount_; ++j) {
                recentPatients_[j] = recentPatients_[j + 1];
            }
            recentPatients_[cacheCount_ - 1] = nullptr;
            --cacheCount_;
            break;
        }
    }
}

void ClinicDatabase::rebuildCache() {
    for (std::size_t i = 0; i < CACHE_CAPACITY; ++i) {
        recentPatients_[i] = nullptr;
    }
    cacheCount_ = 0;
    for (const auto& p : patients_) {
        if (p && p->getAccessCount() > 0) {
            if (cacheCount_ < CACHE_CAPACITY) {
                recentPatients_[cacheCount_++] = p.get();
            } else {
                // Replace lowest accessCount if current is higher
                std::size_t minIdx = 0;
                for (std::size_t i = 1; i < CACHE_CAPACITY; ++i) {
                    if (recentPatients_[i]->getAccessCount() < recentPatients_[minIdx]->getAccessCount()) {
                        minIdx = i;
                    }
                }
                if (p->getAccessCount() > recentPatients_[minIdx]->getAccessCount()) {
                    recentPatients_[minIdx] = p.get();
                }
            }
        }
    }
    for (std::size_t i = 1; i < cacheCount_; ++i) {
        PatientRecord* key = recentPatients_[i];
        std::size_t j = i;
        while (j > 0 && recentPatients_[j - 1]->getAccessCount() < key->getAccessCount()) {
            recentPatients_[j] = recentPatients_[j - 1];
            --j;
        }
        recentPatients_[j] = key;
    }
}

std::vector<PatientRecord*> ClinicDatabase::getFrequentlyAccessedPatients() const {
    std::vector<PatientRecord*> result;
    result.reserve(cacheCount_);
    for (std::size_t i = 0; i < cacheCount_; ++i) {
        if (recentPatients_[i] != nullptr) {
            result.push_back(recentPatients_[i]);
        }
    }
    return result;
}

SortField ClinicDatabase::getSortedState() const {
    return sortedState_;
}

void ClinicDatabase::setSortedState(SortField field) {
    sortedState_ = field;
}

} // namespace scapms::model
