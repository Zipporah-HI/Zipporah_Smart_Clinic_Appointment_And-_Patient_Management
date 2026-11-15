#include "utils/FileHandler.h"

#include "model/EmergencyPatient.h"
#include "model/FollowUpPatient.h"
#include "model/GeneralPatient.h"
#include "utils/ClinicExceptions.h"
#include "utils/IDGenerator.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

namespace scapms::utils {

std::string FileHandler::escapeCSV(const std::string& field) {
    bool needsQuotes = false;
    for (char ch : field) {
        if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) {
        return field;
    }
    std::string out = "\"";
    for (char ch : field) {
        if (ch == '"') {
            out += "\"\"";
        } else if (ch == '\n' || ch == '\r') {
            out += ' ';
        } else {
            out += ch;
        }
    }
    out += "\"";
    return out;
}

std::vector<std::string> FileHandler::parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += ch;
            }
        } else {
            if (ch == '"') {
                inQuotes = true;
            } else if (ch == ',') {
                fields.push_back(current);
                current.clear();
            } else if (ch != '\r') {
                current += ch;
            }
        }
    }
    fields.push_back(current);
    return fields;
}

void FileHandler::savePatientsCSV(const model::ClinicDatabase& db, const std::string& filePath) {
    const std::filesystem::path path(filePath);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Unable to open patients file for writing: " + filePath);
    }

    out << "patient_id,category,name,age,contact_info,extra_field_1,extra_field_2,access_count\n";
    for (const auto& patient : db.getPatients()) {
        if (!patient) {
            continue;
        }
        out << escapeCSV(patient->getPatientId()) << ','
            << escapeCSV(patient->getPatientCategory()) << ','
            << escapeCSV(patient->getName()) << ','
            << patient->getAge() << ','
            << escapeCSV(patient->getContactInfo()) << ','
            << escapeCSV(patient->getTypeSpecificField1()) << ','
            << escapeCSV(patient->getTypeSpecificField2()) << ','
            << patient->getAccessCount() << '\n';
    }

    if (!out.good()) {
        throw FileIOException("Error occurred while writing patients file: " + filePath);
    }
}

void FileHandler::saveAppointmentsCSV(const model::ClinicDatabase& db, const std::string& filePath) {
    const std::filesystem::path path(filePath);
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ofstream out(filePath, std::ios::trunc);
    if (!out.is_open()) {
        throw FileIOException("Unable to open appointments file for writing: " + filePath);
    }

    out << "appointment_id,patient_id,doctor_name,date,time,purpose,status,notes\n";
    for (const auto& apt : db.getAppointments()) {
        out << escapeCSV(apt.getAppointmentId()) << ','
            << escapeCSV(apt.getPatientId()) << ','
            << escapeCSV(apt.getDoctorName()) << ','
            << escapeCSV(apt.getDate()) << ','
            << escapeCSV(apt.getTime()) << ','
            << escapeCSV(apt.getPurpose()) << ','
            << escapeCSV(apt.getStatusString()) << ','
            << escapeCSV(apt.getNotes()) << '\n';
    }

    if (!out.good()) {
        throw FileIOException("Error occurred while writing appointments file: " + filePath);
    }
}

void FileHandler::loadPatientsCSV(model::ClinicDatabase& db, const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        throw FileIOException("Unable to open patients file for reading: " + filePath);
    }

    std::string line;
    if (!std::getline(in, line)) {
        throw FileIOException("Patients CSV file is empty: " + filePath);
    }

    std::vector<std::unique_ptr<model::PatientRecord>> loaded;
    int maxIdNum = 1000;
    size_t lineNumber = 1;

    while (std::getline(in, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }
        const auto cols = parseCSVLine(line);
        if (cols.size() < 7) {
            throw FileIOException("Malformed patient record at line " +
                                  std::to_string(lineNumber) + " in " + filePath);
        }

        const std::string& id = cols[0];
        const std::string& category = cols[1];
        const std::string& name = cols[2];
        int age = 0;
        try {
            age = std::stoi(cols[3]);
        } catch (...) {
            throw FileIOException("Invalid age value at line " + std::to_string(lineNumber));
        }
        const std::string& contact = cols[4];
        const std::string& extra1 = cols[5];
        const std::string& extra2 = cols[6];
        int accessCount = 0;
        if (cols.size() >= 8 && !cols[7].empty()) {
            try {
                accessCount = std::stoi(cols[7]);
            } catch (...) {
                accessCount = 0;
            }
        }

        std::unique_ptr<model::PatientRecord> record;
        if (category == "General") {
            record = std::make_unique<model::GeneralPatient>(id, name, age, contact, extra1, extra2);
        } else if (category == "Emergency") {
            record = std::make_unique<model::EmergencyPatient>(id, name, age, contact, extra1, extra2);
        } else if (category == "Follow-Up") {
            record = std::make_unique<model::FollowUpPatient>(id, name, age, contact, extra1, extra2);
        } else {
            throw FileIOException("Unknown patient category '" + category + "' at line " +
                                  std::to_string(lineNumber));
        }

        record->setAccessCount(accessCount);
        const int num = IDGenerator::extractNumericSuffix(id);
        if (num > maxIdNum) {
            maxIdNum = num;
        }
        loaded.push_back(std::move(record));
    }

    db.clearAll();
    for (auto& p : loaded) {
        const int savedAccess = p->getAccessCount();
        db.addPatient(std::move(p));
        if (model::PatientRecord* inserted = db.findPatientRaw(db.getPatients().back()->getPatientId())) {
            inserted->setAccessCount(savedAccess);
        }
    }
    db.rebuildCache();
    IDGenerator::syncPatientCounter(maxIdNum);
}

void FileHandler::loadAppointmentsCSV(model::ClinicDatabase& db, const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) {
        throw FileIOException("Unable to open appointments file for reading: " + filePath);
    }

    std::string line;
    if (!std::getline(in, line)) {
        throw FileIOException("Appointments CSV file is empty: " + filePath);
    }

    int maxAptNum = 2000;
    size_t lineNumber = 1;

    while (std::getline(in, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }
        const auto cols = parseCSVLine(line);
        if (cols.size() < 7) {
            throw FileIOException("Malformed appointment record at line " +
                                  std::to_string(lineNumber) + " in " + filePath);
        }
        const std::string notes = (cols.size() >= 8) ? cols[7] : "";
        model::Appointment apt(
            cols[0],
            cols[1],
            cols[2],
            cols[3],
            cols[4],
            cols[5],
            model::Appointment::stringToStatus(cols[6]),
            notes
        );
        db.addAppointment(apt);
        const int num = IDGenerator::extractNumericSuffix(cols[0]);
        if (num > maxAptNum) {
            maxAptNum = num;
        }
    }
    db.rebuildCache();
    IDGenerator::syncAppointmentCounter(maxAptNum);
}

void FileHandler::saveClinicBundle(const model::ClinicDatabase& db,
                                   const std::string& patientsPath,
                                   const std::string& appointmentsPath) {
    savePatientsCSV(db, patientsPath);
    saveAppointmentsCSV(db, appointmentsPath);
}

void FileHandler::loadClinicBundle(model::ClinicDatabase& db,
                                   const std::string& patientsPath,
                                   const std::string& appointmentsPath) {
    loadPatientsCSV(db, patientsPath);
    if (std::filesystem::exists(appointmentsPath)) {
        loadAppointmentsCSV(db, appointmentsPath);
    }
}

} // namespace scapms::utils
