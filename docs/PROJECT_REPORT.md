# COS202 Programming II — Lab Assessment Project Report
**Project:** Smart Clinic Appointment & Patient Management System (SCAPMS)  
**Language & Framework:** C++17 with Qt 6 Widgets  

---

## i. Project Description
The **Smart Clinic Appointment & Patient Management System (SCAPMS)** is a desktop C++ graphical application developed for a university health clinic to streamline patient registration, clinical triage, appointment scheduling, consultation queue management, searching, sorting, and operational reporting. Rather than storing records in flat, unstructured arrays, SCAPMS models clinic operations using an object-oriented domain layer separated cleanly from controllers, utilities, and an interactive multi-tab Qt 6 GUI. All data persists across sessions in structured CSV files (`data/patients.csv` and `data/appointments.csv`).

## ii. System Features
SCAPMS implements all mandatory clinic workflows:
- **Patient Records Management:** Register, view, edit, and delete General, Emergency, and Follow-Up patients with dynamic category-specific intake forms and strict input validation.
- **Appointment Management:** Schedule, complete, cancel, and remove patient appointments while enforcing patient-category scheduling policies via the `Appointable` interface.
- **Consultation Waiting Queue:** Enqueue arriving patients and call the next patient in strict First-In, First-Out (FIFO) order, accompanied by live waiting-time counters.
- **Manual Search & Sort:** Search by Patient ID, Patient Name, or Category using manual Linear, Binary, or Recursive search algorithms; sort records by Name, ID, Age, or Appointment Date using manual Selection, Insertion, or Merge Sort.
- **Administrative Undo:** Reverse recent administrative actions (patient deletion, patient modification, appointment cancellation/removal) using a stack-based undo mechanism.
- **Clinic Reports & Persistence:** Generate four real-time reports (Most Frequently Attended Patients, Patients with Pending Appointments, Patient Category Distribution, and Daily Appointment Summary) and import/export data via file dialogs.

## iii. Class Hierarchy
The codebase is modularized across four namespaces (`scapms::model`, `scapms::controller`, `scapms::gui`, and `scapms::utils`):
- **Abstract Base Class (`PatientRecord`):** Encapsulates common fields (`patientId_`, `name_`, `age_`, `contactInfo_`, `appointmentIds_`, `accessCount_`) and declares pure virtual methods (`getPatientCategory()`, `getPriorityLevel()`, `getPriorityLabel()`, `getDetailsSummary()`, `clone()`, etc.).
- **Interface (`Appointable`):** Pure abstract class defining `scheduleAppointment()`, `cancelAppointment()`, and `getSchedulingPolicy()`.
- **Concrete Subclasses:** `GeneralPatient`, `EmergencyPatient`, and `FollowUpPatient` each inherit from both `PatientRecord` and `Appointable`, providing specialized fields (e.g., `triageSeverity_`, `previousVisitDate_`), priority rankings, and scheduling policies.
- **Composition & Association:** `ClinicDatabase` owns a `std::vector<std::unique_ptr<PatientRecord>>` and `std::vector<Appointment>`, while `PatientRecord` tracks associated appointment IDs. `ClinicManager::processPatientPolymorphically(const PatientRecord&)` demonstrates runtime polymorphism across all subclasses via a base-class reference.

## iv. Data Structures Used
1. **`std::vector<std::unique_ptr<PatientRecord>>` & `std::vector<Appointment>`:** Primary dynamic storage supporting indexed random access for sorting and binary search while safely managing polymorphic lifetimes.
2. **`std::queue<std::string>`:** Manages the consultation waiting room in strict FIFO order (`enqueuePatient` pushes to back, `dequeueNextPatient` serves from front).
3. **`std::stack<AdminAction>`:** Powers the Undo Last Administrative Operation feature by pushing deep-cloned `PatientRecord` and `Appointment` snapshots before destructive edits or deletions.
4. **Fixed-Size C++ Array (`PatientRecord* recentPatients_[10]`):** Implements a 10-slot Frequently Accessed Patient Records cache ordered by lookup frequency with pointer invalidation on deletion/update.

## v. Searching Algorithms Used
- **Linear Search (`linearSearchById`, `linearSearchByCategory`):** Sequentially scans `patients_` in $O(n)$ time. Used when records are unsorted by ID or when filtering by polymorphic category.
- **Binary Search (`binarySearchById`):** Operates in $O(\log n)$ time using `low`, `mid`, and `high` bounds when records are sorted by `PatientId` (or automatically sorts via Merge Sort first when invoked on unsorted data).
- **Auto-Adaptive Dispatch:** `SearchEngine::executeSearch()` inspects `ClinicDatabase::getSortedState()` and the query pattern to choose Binary Search on sorted IDs, Linear Search on unsorted IDs, or Recursive Search on names.

## vi. Sorting Algorithms Used
All sorting routines are student-implemented without `std::sort` or `qsort`:
- **Selection Sort (`selectionSortImpl`):** $O(n^2)$ comparisons with at most $n - 1$ swaps and $O(1)$ auxiliary memory.
- **Insertion Sort (`insertionSortImpl`):** Stable $O(n^2)$ worst-case / $O(n)$ best-case sort for nearly-sorted rosters.
- **Merge Sort (`mergeSortImpl` / `mergeImpl`):** Divide-and-conquer stable $O(n \log n)$ sort across `Patient Name`, `Patient ID`, `Age`, and `Appointment Date`.

## vii. Recursive Algorithm Used
- **Recursive Name Search (`SearchEngine::recursiveSearchHelper`):** Recursively traverses `patients_` starting at `index = 0` with base case `index >= patients.size()`. At each frame, it performs a case-insensitive substring comparison on `getName()`, appends matching pointers, and recurses to `index + 1`. Merge Sort (`mergeSortImpl`) also uses divide-and-conquer recursion.

## viii. GUI Design
Built with **Qt 6 Widgets**, `MainWindow` organizes six dedicated tabs (`DashboardPanel`, `PatientPanel`, `AppointmentPanel`, `QueuePanel`, `SearchPanel`, `AdminPanel`) using non-fixed layouts (`QVBoxLayout`, `QHBoxLayout`, `QGridLayout`, `QFormLayout`, and `QStackedLayout`):
- **Dynamic Components:** `PatientPanel` uses `QStackedLayout` to switch category-specific intake fields dynamically when changing the Patient Type combo box.
- **Custom Presentation:** Tables render color-coded priority/category cells (`Emergency -> High Priority`, `Follow-Up -> Scheduled`, `General -> Normal`) and appointment status badges (`Pending`, `Completed`, `Cancelled`).
- **Timers, Shortcuts & Tooltips:** `QTimer` drives the live clock, waiting-room elapsed timers, and appointment/queue reminder banners; menu actions provide `Ctrl+S`, `Ctrl+O`, `Ctrl+F`, `Ctrl+Z`, and `Ctrl+Q` shortcuts alongside tooltips and `QFileDialog` import/export.

## ix. Exception Handling Approach
A custom hierarchy rooted at `scapms::utils::ClinicException` (inheriting `std::runtime_error`) defines `InvalidPatientException`, `DuplicatePatientIdException`, `InvalidAgeException`, `InvalidAppointmentException`, `FileIOException`, and `InvalidSearchException`. Domain constructors, setters, search routines, and CSV file handlers validate inputs and throw targeted exceptions. Every GUI action wraps controller calls in `try { ... } catch (const ClinicException&) { ... } catch (...) { ... }` blocks, displaying informative `QMessageBox` dialogs without crashing.

## x. Algorithms Chosen and Reasons
- **Merge Sort** was selected as the primary sort because clinic rosters grow over a semester and require guaranteed $O(n \log n)$ performance and stability (preserving relative order of ties), as well as reliable pre-sorting for Binary Search.
- **Insertion Sort** was included because clinics frequently append small numbers of newly arrived patients to an already-sorted list, where Insertion Sort achieves near-linear $O(n)$ behavior.
- **Selection Sort** complements the above by minimizing swaps ($O(n)$ writes) when reordering `std::unique_ptr<PatientRecord>` objects.
- **Binary Search + Linear Search + Recursive Search** ensure both fast exact ID lookup ($O(\log n)$ on sorted data) and flexible partial name/unsorted lookup ($O(n)$) without forcing unnecessary re-sorts on every operation.

## xi. Challenges Encountered
1. **Dangling Pointers in Fixed Array Cache:** Storing raw `PatientRecord*` pointers inside `recentPatients_[10]` risked dangling references whenever a patient was updated (replacing the `std::unique_ptr`) or deleted.
2. **Unsigned Index Underflow in Sorting & Binary Search:** Computing `mid - 1` or `arr.size() - 1` with `std::size_t` causes underflow when `mid == 0` or `arr.empty()`.
3. **Undo Stack Ownership with Polymorphic Records:** `std::stack<AdminAction>` stores `std::unique_ptr<PatientRecord>` snapshots, which cannot be copied by standard CSV loading or naive inspection routines.

## xii. How the Challenges Were Resolved
1. **Explicit Cache Invalidation:** `ClinicDatabase::invalidateCachePointer()` compacts `recentPatients_` immediately before any `erase` or `unique_ptr` replacement, and `rebuildCache()` reconstructs the array after bulk CSV loads.
2. **Boundary Guards:** Added explicit `if (patients.empty())` and `if (mid == 0) break;` guards in `binarySearchById()` and `if (size > 1)` before `mergeSortImpl()`.
3. **Virtual `clone()` & Move Semantics:** Added `virtual std::unique_ptr<PatientRecord> clone() const = 0;` across the `PatientRecord` hierarchy so undo snapshots deep-copy polymorphic subclasses cleanly and transfer ownership into `undoStack_` via `std::move`.
