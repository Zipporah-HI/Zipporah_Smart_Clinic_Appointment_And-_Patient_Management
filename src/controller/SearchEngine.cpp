#include "controller/SearchEngine.h"

#include "utils/ClinicExceptions.h"

#include <cctype>
#include <utility>

namespace scapms::controller {

std::string SearchEngine::toLowerCopy(const std::string& text) {
    std::string out = text;
    for (char& ch : out) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

std::string SearchEngine::sortFieldToString(model::SortField field) {
    switch (field) {
        case model::SortField::PatientId:
            return "Patient ID";
        case model::SortField::Name:
            return "Patient Name";
        case model::SortField::Age:
            return "Age";
        case model::SortField::AppointmentDate:
            return "Appointment Date";
        case model::SortField::None:
            return "Unsorted";
    }
    return "Unsorted";
}

model::PatientRecord* SearchEngine::linearSearchById(model::ClinicDatabase& db,
                                                     const std::string& patientId,
                                                     std::size_t* comparisonsOut) {
    if (patientId.empty()) {
        throw utils::InvalidSearchException("Search Patient ID cannot be empty.");
    }
    const std::string targetLower = toLowerCopy(patientId);
    auto& patients = db.getPatientsMutable();
    std::size_t comparisons = 0;

    for (std::size_t i = 0; i < patients.size(); ++i) {
        ++comparisons;
        if (patients[i] && toLowerCopy(patients[i]->getPatientId()) == targetLower) {
            if (comparisonsOut) {
                *comparisonsOut = comparisons;
            }
            db.recordPatientAccess(patients[i].get());
            return patients[i].get();
        }
    }

    if (comparisonsOut) {
        *comparisonsOut = comparisons;
    }
    return nullptr;
}

model::PatientRecord* SearchEngine::binarySearchById(model::ClinicDatabase& db,
                                                     const std::string& patientId,
                                                     bool autoSortIfUnsorted,
                                                     std::size_t* comparisonsOut) {
    if (patientId.empty()) {
        throw utils::InvalidSearchException("Binary search Patient ID cannot be empty.");
    }

    if (db.getSortedState() != model::SortField::PatientId) {
        if (!autoSortIfUnsorted) {
            throw utils::InvalidSearchException(
                "Records are not sorted by Patient ID. Sort by Patient ID first or enable auto-sort.");
        }
        sortPatients(db, SortAlgorithm::MergeSort, model::SortField::PatientId, true);
    }

    auto& patients = db.getPatientsMutable();
    std::size_t comparisons = 0;
    if (patients.empty()) {
        if (comparisonsOut) {
            *comparisonsOut = 0;
        }
        return nullptr;
    }

    const std::string targetLower = toLowerCopy(patientId);
    std::size_t low = 0;
    std::size_t high = patients.size() - 1;

    while (low <= high) {
        const std::size_t mid = low + (high - low) / 2;
        ++comparisons;
        const std::string midId = toLowerCopy(patients[mid]->getPatientId());

        if (midId == targetLower) {
            if (comparisonsOut) {
                *comparisonsOut = comparisons;
            }
            db.recordPatientAccess(patients[mid].get());
            return patients[mid].get();
        }
        if (midId < targetLower) {
            low = mid + 1;
        } else {
            if (mid == 0) {
                break;
            }
            high = mid - 1;
        }
    }

    if (comparisonsOut) {
        *comparisonsOut = comparisons;
    }
    return nullptr;
}

void SearchEngine::recursiveSearchHelper(
    const std::vector<std::unique_ptr<model::PatientRecord>>& patients,
    std::size_t index,
    const std::string& lowerQuery,
    std::vector<model::PatientRecord*>& results,
    std::size_t& comparisons) {
    if (index >= patients.size()) {
        return; // Base case
    }
    ++comparisons;
    if (patients[index]) {
        const std::string lowerName = toLowerCopy(patients[index]->getName());
        if (lowerName.find(lowerQuery) != std::string::npos) {
            results.push_back(patients[index].get());
        }
    }
    recursiveSearchHelper(patients, index + 1, lowerQuery, results, comparisons);
}

std::vector<model::PatientRecord*> SearchEngine::recursiveSearchByName(
    model::ClinicDatabase& db,
    const std::string& nameQuery,
    std::size_t* comparisonsOut) {
    if (nameQuery.empty()) {
        throw utils::InvalidSearchException("Patient name query cannot be empty.");
    }
    std::vector<model::PatientRecord*> results;
    std::size_t comparisons = 0;
    recursiveSearchHelper(db.getPatients(), 0, toLowerCopy(nameQuery), results, comparisons);

    for (model::PatientRecord* match : results) {
        db.recordPatientAccess(match);
    }
    if (comparisonsOut) {
        *comparisonsOut = comparisons;
    }
    return results;
}

std::vector<model::PatientRecord*> SearchEngine::linearSearchByCategory(
    model::ClinicDatabase& db,
    const std::string& category,
    std::size_t* comparisonsOut) {
    if (category.empty()) {
        throw utils::InvalidSearchException("Category filter cannot be empty.");
    }
    const std::string targetLower = toLowerCopy(category);
    std::vector<model::PatientRecord*> results;
    std::size_t comparisons = 0;

    for (const auto& patient : db.getPatients()) {
        ++comparisons;
        if (patient && toLowerCopy(patient->getPatientCategory()).find(targetLower) != std::string::npos) {
            results.push_back(patient.get());
        }
    }

    if (comparisonsOut) {
        *comparisonsOut = comparisons;
    }
    return results;
}

SearchOutcome SearchEngine::executeSearch(model::ClinicDatabase& db,
                                          const std::string& query,
                                          SearchAlgorithm algorithm) {
    if (query.empty()) {
        throw utils::InvalidSearchException("Please enter a search keyword, ID, or category.");
    }

    SearchOutcome outcome;
    const std::string lowerQuery = toLowerCopy(query);

    SearchAlgorithm chosen = algorithm;
    if (chosen == SearchAlgorithm::AutoAdaptive) {
        if (lowerQuery == "general" || lowerQuery == "emergency" || lowerQuery == "follow-up") {
            chosen = SearchAlgorithm::CategoryLinearSearch;
        } else if (lowerQuery.rfind("pat-", 0) == 0) {
            if (db.getSortedState() == model::SortField::PatientId) {
                chosen = SearchAlgorithm::BinarySearchId;
                outcome.rationale = "Data is already sorted by Patient ID, so Binary Search (O(log n)) was automatically selected.";
            } else {
                chosen = SearchAlgorithm::LinearSearchId;
                outcome.rationale = "Data is currently unsorted by Patient ID, so Linear Search (O(n)) was selected without disturbing display order.";
            }
        } else {
            chosen = SearchAlgorithm::RecursiveNameSearch;
            outcome.rationale = "Name substring query detected; Recursive Name Search was selected to locate all partial matches.";
        }
    }

    switch (chosen) {
        case SearchAlgorithm::LinearSearchId: {
            outcome.algorithmUsed = "Linear Search (by Patient ID)";
            if (outcome.rationale.empty()) {
                outcome.rationale = "Scans unsorted or arbitrary-order records sequentially in O(n) time.";
            }
            if (model::PatientRecord* found = linearSearchById(db, query, &outcome.comparisons)) {
                outcome.matches.push_back(found);
            }
            break;
        }
        case SearchAlgorithm::BinarySearchId: {
            const bool wasSorted = (db.getSortedState() == model::SortField::PatientId);
            outcome.algorithmUsed = "Binary Search (by Patient ID)";
            if (outcome.rationale.empty()) {
                outcome.rationale = wasSorted
                    ? "Records were already sorted by Patient ID; executed O(log n) Binary Search directly."
                    : "Records were unsorted; sorted by Patient ID via Merge Sort first, then executed O(log n) Binary Search.";
            }
            if (model::PatientRecord* found = binarySearchById(db, query, true, &outcome.comparisons)) {
                outcome.matches.push_back(found);
            }
            break;
        }
        case SearchAlgorithm::RecursiveNameSearch: {
            outcome.algorithmUsed = "Recursive Search (by Patient Name)";
            if (outcome.rationale.empty()) {
                outcome.rationale = "Recursively traverses patient records (base case: index == size) for case-insensitive name matching.";
            }
            outcome.matches = recursiveSearchByName(db, query, &outcome.comparisons);
            break;
        }
        case SearchAlgorithm::CategoryLinearSearch: {
            outcome.algorithmUsed = "Linear Scan (by Patient Category)";
            if (outcome.rationale.empty()) {
                outcome.rationale = "Scans records using polymorphic getPatientCategory() to filter matching patient types.";
            }
            outcome.matches = linearSearchByCategory(db, query, &outcome.comparisons);
            break;
        }
        case SearchAlgorithm::AutoAdaptive:
            break;
    }

    return outcome;
}

int SearchEngine::comparePatients(const model::ClinicDatabase& db,
                                  const model::PatientRecord& a,
                                  const model::PatientRecord& b,
                                  model::SortField field,
                                  bool ascending) {
    int cmp = 0;
    switch (field) {
        case model::SortField::PatientId: {
            const std::string idA = toLowerCopy(a.getPatientId());
            const std::string idB = toLowerCopy(b.getPatientId());
            cmp = (idA < idB) ? -1 : ((idA > idB) ? 1 : 0);
            break;
        }
        case model::SortField::Name: {
            const std::string nameA = toLowerCopy(a.getName());
            const std::string nameB = toLowerCopy(b.getName());
            cmp = (nameA < nameB) ? -1 : ((nameA > nameB) ? 1 : 0);
            break;
        }
        case model::SortField::Age: {
            cmp = (a.getAge() < b.getAge()) ? -1 : ((a.getAge() > b.getAge()) ? 1 : 0);
            break;
        }
        case model::SortField::AppointmentDate: {
            const std::string dateA = db.getEarliestPendingOrLatestAppointmentDate(a.getPatientId());
            const std::string dateB = db.getEarliestPendingOrLatestAppointmentDate(b.getPatientId());
            cmp = (dateA < dateB) ? -1 : ((dateA > dateB) ? 1 : 0);
            break;
        }
        case model::SortField::None:
            cmp = 0;
            break;
    }
    return ascending ? cmp : -cmp;
}

void SearchEngine::selectionSortImpl(model::ClinicDatabase& db,
                                     model::SortField field,
                                     bool ascending,
                                     std::size_t& comparisons,
                                     std::size_t& swaps) {
    auto& arr = db.getPatientsMutable();
    const std::size_t n = arr.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        std::size_t bestIdx = i;
        for (std::size_t j = i + 1; j < n; ++j) {
            ++comparisons;
            if (comparePatients(db, *arr[j], *arr[bestIdx], field, ascending) < 0) {
                bestIdx = j;
            }
        }
        if (bestIdx != i) {
            std::swap(arr[i], arr[bestIdx]);
            ++swaps;
        }
    }
}

void SearchEngine::insertionSortImpl(model::ClinicDatabase& db,
                                     model::SortField field,
                                     bool ascending,
                                     std::size_t& comparisons,
                                     std::size_t& moves) {
    auto& arr = db.getPatientsMutable();
    const std::size_t n = arr.size();
    for (std::size_t i = 1; i < n; ++i) {
        std::unique_ptr<model::PatientRecord> key = std::move(arr[i]);
        std::size_t j = i;
        while (j > 0) {
            ++comparisons;
            if (comparePatients(db, *arr[j - 1], *key, field, ascending) > 0) {
                arr[j] = std::move(arr[j - 1]);
                ++moves;
                --j;
            } else {
                break;
            }
        }
        arr[j] = std::move(key);
    }
}

void SearchEngine::mergeImpl(model::ClinicDatabase& db,
                             std::vector<std::unique_ptr<model::PatientRecord>>& arr,
                             std::size_t left,
                             std::size_t mid,
                             std::size_t right,
                             model::SortField field,
                             bool ascending,
                             std::size_t& comparisons,
                             std::size_t& moves) {
    const std::size_t leftCount = mid - left + 1;
    const std::size_t rightCount = right - mid;

    std::vector<std::unique_ptr<model::PatientRecord>> leftVec;
    std::vector<std::unique_ptr<model::PatientRecord>> rightVec;
    leftVec.reserve(leftCount);
    rightVec.reserve(rightCount);

    for (std::size_t i = 0; i < leftCount; ++i) {
        leftVec.push_back(std::move(arr[left + i]));
    }
    for (std::size_t j = 0; j < rightCount; ++j) {
        rightVec.push_back(std::move(arr[mid + 1 + j]));
    }

    std::size_t i = 0;
    std::size_t j = 0;
    std::size_t k = left;

    while (i < leftCount && j < rightCount) {
        ++comparisons;
        if (comparePatients(db, *leftVec[i], *rightVec[j], field, ascending) <= 0) {
            arr[k++] = std::move(leftVec[i++]);
        } else {
            arr[k++] = std::move(rightVec[j++]);
        }
        ++moves;
    }

    while (i < leftCount) {
        arr[k++] = std::move(leftVec[i++]);
        ++moves;
    }
    while (j < rightCount) {
        arr[k++] = std::move(rightVec[j++]);
        ++moves;
    }
}

void SearchEngine::mergeSortImpl(model::ClinicDatabase& db,
                                 std::vector<std::unique_ptr<model::PatientRecord>>& arr,
                                 std::size_t left,
                                 std::size_t right,
                                 model::SortField field,
                                 bool ascending,
                                 std::size_t& comparisons,
                                 std::size_t& moves) {
    if (left >= right) {
        return;
    }
    const std::size_t mid = left + (right - left) / 2;
    mergeSortImpl(db, arr, left, mid, field, ascending, comparisons, moves);
    mergeSortImpl(db, arr, mid + 1, right, field, ascending, comparisons, moves);
    mergeImpl(db, arr, left, mid, right, field, ascending, comparisons, moves);
}

SortOutcome SearchEngine::sortPatients(model::ClinicDatabase& db,
                                       SortAlgorithm algorithm,
                                       model::SortField field,
                                       bool ascending) {
    if (field == model::SortField::None) {
        throw utils::InvalidSearchException("Please select a valid field to sort by.");
    }

    SortOutcome outcome;
    outcome.sortFieldLabel = sortFieldToString(field);
    std::size_t comparisons = 0;
    std::size_t swapsOrMoves = 0;

    switch (algorithm) {
        case SortAlgorithm::SelectionSort:
            outcome.algorithmUsed = "Selection Sort";
            selectionSortImpl(db, field, ascending, comparisons, swapsOrMoves);
            break;
        case SortAlgorithm::InsertionSort:
            outcome.algorithmUsed = "Insertion Sort";
            insertionSortImpl(db, field, ascending, comparisons, swapsOrMoves);
            break;
        case SortAlgorithm::MergeSort:
            outcome.algorithmUsed = "Merge Sort";
            if (db.getPatientsMutable().size() > 1) {
                mergeSortImpl(db,
                              db.getPatientsMutable(),
                              0,
                              db.getPatientsMutable().size() - 1,
                              field,
                              ascending,
                              comparisons,
                              swapsOrMoves);
            }
            break;
    }

    db.setSortedState(ascending ? field : model::SortField::None);
    outcome.comparisons = comparisons;
    outcome.swapsOrMoves = swapsOrMoves;
    outcome.rationale = explainSortChoice(algorithm, field, db.getPatientCount());
    return outcome;
}

std::string SearchEngine::explainSortChoice(SortAlgorithm algorithm,
                                            model::SortField field,
                                            std::size_t recordCount) {
    const std::string fieldName = sortFieldToString(field);
    switch (algorithm) {
        case SortAlgorithm::SelectionSort:
            return "Selection Sort by " + fieldName + " (" + std::to_string(recordCount) +
                   " records): performs at most n-1 swaps (O(n^2) comparisons, O(1) extra space), "
                   "minimizing memory writes when moving heavy polymorphic handles.";
        case SortAlgorithm::InsertionSort:
            return "Insertion Sort by " + fieldName + " (" + std::to_string(recordCount) +
                   " records): stable O(n^2) worst-case and O(n) best-case on nearly-sorted clinic rosters, "
                   "ideal when small batches of newly registered patients are appended.";
        case SortAlgorithm::MergeSort:
            return "Merge Sort by " + fieldName + " (" + std::to_string(recordCount) +
                   " records): guaranteed O(n log n) worst-case time and stable ordering, "
                   "ideal for larger patient datasets and preparing records for O(log n) Binary Search.";
    }
    return "";
}

} // namespace scapms::controller
