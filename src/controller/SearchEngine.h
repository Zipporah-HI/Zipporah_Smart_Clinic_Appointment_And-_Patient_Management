#ifndef SCAPMS_CONTROLLER_SEARCH_ENGINE_H
#define SCAPMS_CONTROLLER_SEARCH_ENGINE_H

#include "model/ClinicDatabase.h"

#include <cstddef>
#include <string>
#include <vector>

namespace scapms::controller {

enum class SearchAlgorithm {
    AutoAdaptive,
    LinearSearchId,
    BinarySearchId,
    RecursiveNameSearch,
    CategoryLinearSearch
};

enum class SortAlgorithm {
    SelectionSort,
    InsertionSort,
    MergeSort
};

struct SearchOutcome {
    std::vector<model::PatientRecord*> matches;
    std::string algorithmUsed;
    std::string rationale;
    std::size_t comparisons = 0;
};

struct SortOutcome {
    std::string algorithmUsed;
    std::string sortFieldLabel;
    std::string rationale;
    std::size_t comparisons = 0;
    std::size_t swapsOrMoves = 0;
};

/**
 * @brief Implements all manual searching and sorting algorithms (without STL sort/search helpers).
 */
class SearchEngine {
public:
    // Searching algorithms (all manually implemented)
    static model::PatientRecord* linearSearchById(model::ClinicDatabase& db,
                                                  const std::string& patientId,
                                                  std::size_t* comparisonsOut = nullptr);

    static model::PatientRecord* binarySearchById(model::ClinicDatabase& db,
                                                  const std::string& patientId,
                                                  bool autoSortIfUnsorted = true,
                                                  std::size_t* comparisonsOut = nullptr);

    static std::vector<model::PatientRecord*> recursiveSearchByName(
        model::ClinicDatabase& db,
        const std::string& nameQuery,
        std::size_t* comparisonsOut = nullptr);

    static std::vector<model::PatientRecord*> linearSearchByCategory(
        model::ClinicDatabase& db,
        const std::string& category,
        std::size_t* comparisonsOut = nullptr);

    static SearchOutcome executeSearch(model::ClinicDatabase& db,
                                       const std::string& query,
                                       SearchAlgorithm algorithm);

    // Sorting algorithms (all manually implemented)
    static SortOutcome sortPatients(model::ClinicDatabase& db,
                                    SortAlgorithm algorithm,
                                    model::SortField field,
                                    bool ascending = true);

    static std::string explainSortChoice(SortAlgorithm algorithm,
                                         model::SortField field,
                                         std::size_t recordCount);

    static std::string sortFieldToString(model::SortField field);
    static std::string toLowerCopy(const std::string& text);

private:
    static void recursiveSearchHelper(const std::vector<std::unique_ptr<model::PatientRecord>>& patients,
                                      std::size_t index,
                                      const std::string& lowerQuery,
                                      std::vector<model::PatientRecord*>& results,
                                      std::size_t& comparisons);

    static int comparePatients(const model::ClinicDatabase& db,
                               const model::PatientRecord& a,
                               const model::PatientRecord& b,
                               model::SortField field,
                               bool ascending);

    static void selectionSortImpl(model::ClinicDatabase& db,
                                  model::SortField field,
                                  bool ascending,
                                  std::size_t& comparisons,
                                  std::size_t& swaps);

    static void insertionSortImpl(model::ClinicDatabase& db,
                                  model::SortField field,
                                  bool ascending,
                                  std::size_t& comparisons,
                                  std::size_t& moves);

    static void mergeSortImpl(model::ClinicDatabase& db,
                              std::vector<std::unique_ptr<model::PatientRecord>>& arr,
                              std::size_t left,
                              std::size_t right,
                              model::SortField field,
                              bool ascending,
                              std::size_t& comparisons,
                              std::size_t& moves);

    static void mergeImpl(model::ClinicDatabase& db,
                          std::vector<std::unique_ptr<model::PatientRecord>>& arr,
                          std::size_t left,
                          std::size_t mid,
                          std::size_t right,
                          model::SortField field,
                          bool ascending,
                          std::size_t& comparisons,
                          std::size_t& moves);
};

} // namespace scapms::controller

#endif // SCAPMS_CONTROLLER_SEARCH_ENGINE_H
