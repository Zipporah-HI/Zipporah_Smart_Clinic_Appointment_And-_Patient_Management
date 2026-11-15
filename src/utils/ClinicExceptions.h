#ifndef SCAPMS_UTILS_CLINIC_EXCEPTIONS_H
#define SCAPMS_UTILS_CLINIC_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace scapms::utils {

/**
 * @brief Base exception class for all Smart Clinic (SCAPMS) domain and I/O errors.
 */
class ClinicException : public std::runtime_error {
public:
    explicit ClinicException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Thrown when patient fields (ID, name, contact, subclass metadata) fail validation.
 */
class InvalidPatientException : public ClinicException {
public:
    explicit InvalidPatientException(const std::string& message)
        : ClinicException(message) {}
};

/**
 * @brief Thrown when attempting to register a patient with an existing Patient ID.
 */
class DuplicatePatientIdException : public ClinicException {
public:
    explicit DuplicatePatientIdException(const std::string& patientId)
        : ClinicException("Patient ID already exists: " + patientId) {}
};

/**
 * @brief Thrown when patient age falls outside the valid clinic range [0, 120].
 */
class InvalidAgeException : public ClinicException {
public:
    explicit InvalidAgeException(int age)
        : ClinicException("Patient age must be between 0 and 120 (received: " +
                          std::to_string(age) + ").") {}
};

/**
 * @brief Thrown when appointment scheduling, cancellation, or fields are invalid.
 */
class InvalidAppointmentException : public ClinicException {
public:
    explicit InvalidAppointmentException(const std::string& message)
        : ClinicException(message) {}
};

/**
 * @brief Thrown when reading or writing clinic data files fails.
 */
class FileIOException : public ClinicException {
public:
    explicit FileIOException(const std::string& message)
        : ClinicException(message) {}
};

/**
 * @brief Thrown when search parameters are empty or incompatible with the chosen algorithm.
 */
class InvalidSearchException : public ClinicException {
public:
    explicit InvalidSearchException(const std::string& message)
        : ClinicException(message) {}
};

} // namespace scapms::utils

#endif // SCAPMS_UTILS_CLINIC_EXCEPTIONS_H
