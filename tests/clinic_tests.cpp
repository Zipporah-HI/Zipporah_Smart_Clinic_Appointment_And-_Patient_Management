#include "controller/ClinicManager.h"
#include "controller/SearchEngine.h"
#include "model/Appointable.h"
#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicExceptions.h"
#include "utils/FileHandler.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>

using namespace scapms::model;
using namespace scapms::controller;
using namespace scapms::utils;

namespace {

void testPolymorphismAndInterfaces() {
    GeneralPatient gp("PAT-001", "Alice", 22, "0801-111", "Checkup", "Dr. Bello");
    EmergencyPatient ep("PAT-002", "Bob", 25, "0801-222", "Critical", "Mom");
    FollowUpPatient fp("PAT-003", "Clara", 30, "0801-333", "2026-10-10", "Dr. Okafor");

    const PatientRecord* records[3] = {&gp, &ep, &fp};
    assert(records[0]->getPatientCategory() == "General");
    assert(records[0]->getPriorityLevel() == 3);
    assert(records[1]->getPatientCategory() == "Emergency");
    assert(records[1]->getPriorityLevel() == 1);
    assert(records[2]->getPatientCategory() == "Follow-Up");
    assert(records[2]->getPriorityLevel() == 2);

    // Runtime polymorphism function via base-class reference
    const std::string log0 = ClinicManager::processPatientPolymorphically(*records[0]);
    const std::string log1 = ClinicManager::processPatientPolymorphically(*records[1]);
    assert(log0.find("Normal") != std::string::npos);
    assert(log1.find("High Priority") != std::string::npos);

    // Appointable interface
    Appointable* ap = &gp;
    assert(ap->scheduleAppointment("APT-100"));
    assert(gp.hasAppointmentId("APT-100"));
    assert(ap->cancelAppointment("APT-100"));
    assert(!gp.hasAppointmentId("APT-100"));
}

void testExceptionsAndValidation() {
    bool caughtAge = false;
    try {
        GeneralPatient badAge("PAT-999", "Test", 150, "0800");
    } catch (const InvalidAgeException&) {
        caughtAge = true;
    }
    assert(caughtAge);

    ClinicManager mgr;
    mgr.registerPatient("General", "PAT-100", "John Doe", 20, "0800", "Cold", "Dr. A");

    bool caughtDup = false;
    try {
        mgr.registerPatient("General", "PAT-100", "Jane Doe", 22, "0801", "Cough", "Dr. B");
    } catch (const DuplicatePatientIdException&) {
        caughtDup = true;
    }
    assert(caughtDup);

    bool caughtAppt = false;
    try {
        mgr.getAppointmentController().scheduleAppointment(
            "PAT-404", "Dr. A", "2026-13-40", "25:99", "Bad Date");
    } catch (const InvalidAppointmentException&) {
        caughtAppt = true;
    }
    assert(caughtAppt);

    bool caughtFile = false;
    try {
        FileHandler::loadPatientsCSV(mgr.getDatabase(), "/nonexistent_dir_12345/missing.csv");
    } catch (const FileIOException&) {
        caughtFile = true;
    }
    assert(caughtFile);
}

void testDataStructuresQueueStackAndCache() {
    ClinicManager mgr;
    mgr.registerPatient("General", "PAT-10", "First Patient", 21, "111", "Flu", "Dr. A");
    mgr.registerPatient("Emergency", "PAT-20", "Second Patient", 22, "222", "Critical", "Kin");
    mgr.registerPatient("Follow-Up", "PAT-30", "Third Patient", 23, "333", "2026-10-01", "Dr. B");

    // FIFO Queue
    mgr.addToWaitingQueue("PAT-10");
    mgr.addToWaitingQueue("PAT-20");
    assert(mgr.getDatabase().getQueueSize() == 2);
    PatientRecord* next = mgr.callNextPatient();
    assert(next != nullptr && next->getPatientId() == "PAT-10");
    assert(mgr.getDatabase().getQueueSize() == 1);

    // Undo Stack: Modify patient, then Delete patient, then Undo both
    mgr.updatePatient("PAT-30", "Follow-Up", "Third Updated", 24, "333", "2026-10-02", "Dr. C");
    assert(mgr.getDatabase().findPatientRaw("PAT-30")->getName() == "Third Updated");
    mgr.deletePatient("PAT-30");
    assert(mgr.getDatabase().findPatientRaw("PAT-30") == nullptr);

    mgr.undoLastOperation(); // restores deleted PAT-30
    assert(mgr.getDatabase().findPatientRaw("PAT-30") != nullptr);
    assert(mgr.getDatabase().findPatientRaw("PAT-30")->getName() == "Third Updated");

    mgr.undoLastOperation(); // reverts modification back to Third Patient
    assert(mgr.getDatabase().findPatientRaw("PAT-30")->getName() == "Third Patient");

    // Fixed-size array cache <= 10
    for (int i = 0; i < 15; ++i) {
        mgr.registerPatient("General",
                            "PAT-C" + std::to_string(i),
                            "Cached " + std::to_string(i),
                            20,
                            "555",
                            "Check",
                            "Dr. X");
    }
    assert(mgr.getDatabase().getFrequentlyAccessedPatients().size() == ClinicDatabase::CACHE_CAPACITY);
}

void testManualSearchAndSortAlgorithms() {
    ClinicManager mgr;
    mgr.registerPatient("General", "PAT-300", "Zara Musa", 29, "111", "Check", "Dr. A");
    mgr.registerPatient("Emergency", "PAT-100", "Amina Bello", 19, "222", "High", "Kin");
    mgr.registerPatient("Follow-Up", "PAT-200", "Chidi Okafor", 24, "333", "2026-10-12", "Dr. B");

    // Linear Search on unsorted data
    auto* linearHit = SearchEngine::linearSearchById(mgr.getDatabase(), "PAT-100");
    assert(linearHit != nullptr && linearHit->getName() == "Amina Bello");

    // Recursive Name Search
    auto recHits = SearchEngine::recursiveSearchByName(mgr.getDatabase(), "okafor");
    assert(recHits.size() == 1 && recHits[0]->getPatientId() == "PAT-200");

    // Selection Sort by Age ascending
    SearchEngine::sortPatients(mgr.getDatabase(), SortAlgorithm::SelectionSort, SortField::Age, true);
    assert(mgr.getDatabase().getPatients()[0]->getAge() == 19);
    assert(mgr.getDatabase().getPatients()[1]->getAge() == 24);
    assert(mgr.getDatabase().getPatients()[2]->getAge() == 29);

    // Insertion Sort by Name ascending
    SearchEngine::sortPatients(mgr.getDatabase(), SortAlgorithm::InsertionSort, SortField::Name, true);
    assert(mgr.getDatabase().getPatients()[0]->getName() == "Amina Bello");
    assert(mgr.getDatabase().getPatients()[2]->getName() == "Zara Musa");

    // Merge Sort by Patient ID + Binary Search
    SearchEngine::sortPatients(mgr.getDatabase(), SortAlgorithm::MergeSort, SortField::PatientId, true);
    assert(mgr.getDatabase().getSortedState() == SortField::PatientId);
    auto* binHit = SearchEngine::binarySearchById(mgr.getDatabase(), "PAT-200", false);
    assert(binHit != nullptr && binHit->getName() == "Chidi Okafor");
}

void testPersistenceAndReports() {
    ClinicManager mgr;
    mgr.seedSampleClinicData();

    const auto dist = mgr.getCategoryDistribution();
    assert(dist.totalPatients == 8);
    assert(dist.generalCount == 3);
    assert(dist.emergencyCount == 2);
    assert(dist.followUpCount == 3);

    const auto summary = mgr.getDailyAppointmentSummary();
    assert(summary.totalAppointments == 10);
    assert(summary.completedCount == 5);
    assert(summary.pendingCount == 4);
    assert(summary.cancelledCount == 1);

    const auto attended = mgr.getMostFrequentlyAttendedPatients();
    assert(!attended.empty());
    assert(attended[0].completedCount == 2); // PAT-1001 has 2 completed visits

    const std::filesystem::path tmpDir = std::filesystem::temp_directory_path() / "scapms_test_io";
    std::filesystem::create_directories(tmpDir);
    const std::string pFile = (tmpDir / "patients.csv").string();
    const std::string aFile = (tmpDir / "appointments.csv").string();

    mgr.saveClinicData(pFile, aFile);

    ClinicManager reloaded;
    reloaded.loadClinicData(pFile, aFile);
    assert(reloaded.getDatabase().getPatientCount() == 8);
    assert(reloaded.getDatabase().getAppointments().size() == 10);
    assert(reloaded.getDatabase().findPatientRaw("PAT-1002")->getPatientCategory() == "Emergency");

    std::filesystem::remove_all(tmpDir);
}

} // namespace

int main() {
    testPolymorphismAndInterfaces();
    testExceptionsAndValidation();
    testDataStructuresQueueStackAndCache();
    testManualSearchAndSortAlgorithms();
    testPersistenceAndReports();
    std::cout << "All SCAPMS unit tests passed successfully.\n";
    return 0;
}
