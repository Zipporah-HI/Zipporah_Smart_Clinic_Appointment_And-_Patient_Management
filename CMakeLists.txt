cmake_minimum_required(VERSION 3.20)
project(SCAPMS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(scapms_core STATIC
    src/model/PatientRecord.cpp
    src/model/GeneralPatient.cpp
    src/model/EmergencyPatient.cpp
    src/model/FollowUpPatient.cpp
    src/model/Appointment.cpp
    src/model/ClinicDatabase.cpp
    src/controller/SearchEngine.cpp
    src/controller/AppointmentController.cpp
    src/controller/ClinicManager.cpp
    src/utils/IDGenerator.cpp
    src/utils/FileHandler.cpp
)

target_include_directories(scapms_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(scapms
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/DashboardPanel.cpp
    src/gui/PatientPanel.cpp
    src/gui/AppointmentPanel.cpp
    src/gui/QueuePanel.cpp
    src/gui/SearchPanel.cpp
    src/gui/AdminPanel.cpp
)

target_link_libraries(scapms PRIVATE
    scapms_core
    Qt6::Widgets
)

target_compile_options(scapms_core PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(scapms PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(scapms_tests tests/clinic_tests.cpp)
target_link_libraries(scapms_tests PRIVATE scapms_core)
target_compile_options(scapms_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME SCAPMSCoreTests COMMAND scapms_tests)
