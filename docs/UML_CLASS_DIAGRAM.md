# SCAPMS — UML Class Diagram

The diagram below illustrates the object-oriented architecture of the **Smart Clinic Appointment & Patient Management System (SCAPMS)**, highlighting:
- **Abstract Classes:** `PatientRecord` and `Appointable`
- **Inheritance & Interface Realization:** `GeneralPatient`, `EmergencyPatient`, and `FollowUpPatient`
- **Runtime Polymorphism:** `ClinicManager::processPatientPolymorphically(const PatientRecord&)` and `ClinicDatabase` polymorphic container
- **Composition & Association:** `ClinicDatabase` owning `PatientRecord` and `Appointment` collections, `ClinicManager` coordinating controllers, and `MainWindow` hosting GUI panels.

```mermaid
classDiagram
    class PatientRecord {
        <<abstract>>
        #string patientId_
        #string name_
        #int age_
        #string contactInfo_
        #vector~string~ appointmentIds_
        #int accessCount_
        +getPatientId() string
        +getName() string
        +getAge() int
        +getContactInfo() string
        +addAppointmentId(id)
        +getPatientCategory()* string
        +getPriorityLevel()* int
        +getPriorityLabel()* string
        +getDetailsSummary()* string
        +clone()* unique_ptr~PatientRecord~
    }

    class Appointable {
        <<interface>>
        +scheduleAppointment(appointmentId)* bool
        +cancelAppointment(appointmentId)* bool
        +getSchedulingPolicy()* string
    }

    class GeneralPatient {
        -string visitReason_
        -string primaryPhysician_
        +getPatientCategory() string
        +getPriorityLevel() int
        +getPriorityLabel() string
        +scheduleAppointment(appointmentId) bool
        +cancelAppointment(appointmentId) bool
        +clone() unique_ptr~PatientRecord~
    }

    class EmergencyPatient {
        -string triageSeverity_
        -string emergencyContact_
        +getPatientCategory() string
        +getPriorityLevel() int
        +getPriorityLabel() string
        +scheduleAppointment(appointmentId) bool
        +cancelAppointment(appointmentId) bool
        +clone() unique_ptr~PatientRecord~
    }

    class FollowUpPatient {
        -string previousVisitDate_
        -string referringDoctor_
        +getPatientCategory() string
        +getPriorityLevel() int
        +getPriorityLabel() string
        +scheduleAppointment(appointmentId) bool
        +cancelAppointment(appointmentId) bool
        +clone() unique_ptr~PatientRecord~
    }

    class Appointment {
        -string appointmentId_
        -string patientId_
        -string doctorName_
        -string date_
        -string time_
        -string purpose_
        -AppointmentStatus status_
        +getAppointmentId() string
        +getPatientId() string
        +setStatus(status)
    }

    class ClinicDatabase {
        -vector~unique_ptr~PatientRecord~~ patients_
        -vector~Appointment~ appointments_
        -queue~string~ waitingQueue_
        -stack~AdminAction~ undoStack_
        -PatientRecord* recentPatients_[10]
        -SortField sortedState_
        +addPatient(patient)
        +removePatient(id) bool
        +updatePatient(id, updated)
        +enqueuePatient(id)
        +dequeueNextPatient() string
        +undoLastAction() string
        +recordPatientAccess(patient)
    }

    class SearchEngine {
        +linearSearchById(db, id) PatientRecord*
        +binarySearchById(db, id) PatientRecord*
        +recursiveSearchByName(db, query) vector~PatientRecord*~
        +sortPatients(db, algo, field, asc) SortOutcome
    }

    class AppointmentController {
        -ClinicDatabase& db_
        +scheduleAppointment(...) Appointment
        +cancelAppointment(id) bool
        +completeAppointment(id) bool
    }

    class ClinicManager {
        -ClinicDatabase db_
        -AppointmentController appointmentController_
        +registerPatient(...) string
        +processPatientPolymorphically(PatientRecord&) string
        +getMostFrequentlyAttendedPatients() vector
        +getCategoryDistribution() CategoryDistributionReport
        +getDailyAppointmentSummary() DailyAppointmentSummaryReport
    }

    class MainWindow {
        -ClinicManager manager_
        -DashboardPanel* dashboardPanel_
        -PatientPanel* patientPanel_
        -AppointmentPanel* appointmentPanel_
        -QueuePanel* queuePanel_
        -SearchPanel* searchPanel_
        -AdminPanel* adminPanel_
    }

    PatientRecord <|-- GeneralPatient
    PatientRecord <|-- EmergencyPatient
    PatientRecord <|-- FollowUpPatient
    Appointable <|.. GeneralPatient
    Appointable <|.. EmergencyPatient
    Appointable <|.. FollowUpPatient

    ClinicDatabase *-- PatientRecord : owns vector
    ClinicDatabase *-- Appointment : owns vector
    PatientRecord "1" -- "0..*" Appointment : associated via ID
    ClinicManager *-- ClinicDatabase : composes
    ClinicManager *-- AppointmentController : composes
    AppointmentController --> ClinicDatabase : updates
    SearchEngine ..> ClinicDatabase : searches & sorts
    ClinicManager ..> PatientRecord : runtime polymorphism
    MainWindow *-- ClinicManager : coordinates
```
