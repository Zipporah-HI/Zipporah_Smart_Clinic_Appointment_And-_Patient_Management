#include "utils/IDGenerator.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace scapms::utils {

int IDGenerator::patientCounter_ = 1000;
int IDGenerator::appointmentCounter_ = 2000;

std::string IDGenerator::nextPatientId() {
    ++patientCounter_;
    std::ostringstream oss;
    oss << "PAT-" << std::setw(4) << std::setfill('0') << patientCounter_;
    return oss.str();
}

std::string IDGenerator::nextAppointmentId() {
    ++appointmentCounter_;
    std::ostringstream oss;
    oss << "APT-" << std::setw(4) << std::setfill('0') << appointmentCounter_;
    return oss.str();
}

void IDGenerator::syncPatientCounter(int maxExistingNumber) {
    if (maxExistingNumber > patientCounter_) {
        patientCounter_ = maxExistingNumber;
    }
}

void IDGenerator::syncAppointmentCounter(int maxExistingNumber) {
    if (maxExistingNumber > appointmentCounter_) {
        appointmentCounter_ = maxExistingNumber;
    }
}

void IDGenerator::resetCounters() {
    patientCounter_ = 1000;
    appointmentCounter_ = 2000;
}

int IDGenerator::extractNumericSuffix(const std::string& id) {
    std::string digits;
    for (char ch : id) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            digits.push_back(ch);
        }
    }
    if (digits.empty()) {
        return 0;
    }
    try {
        return std::stoi(digits);
    } catch (...) {
        return 0;
    }
}

} // namespace scapms::utils
