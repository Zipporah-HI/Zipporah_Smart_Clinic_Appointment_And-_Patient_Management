# SCAPMS — Smart Clinic Appointment & Patient Management System

COS202 Programming II Lab Assessment Project written in **C++17** with **Qt 6 Widgets**.

## Project Structure

```text
├── CMakeLists.txt
├── README.md
├── data/
│   ├── patients.csv
│   └── appointments.csv
├── docs/
│   ├── PROJECT_REPORT.md
│   ├── UML_CLASS_DIAGRAM.md
│   └── screenshots/
├── src/
│   ├── main.cpp
│   ├── model/
│   │   ├── Appointable.h
│   │   ├── Appointment.h / .cpp
│   │   ├── ClinicDatabase.h / .cpp
│   │   ├── EmergencyPatient.h / .cpp
│   │   ├── FollowUpPatient.h / .cpp
│   │   ├── GeneralPatient.h / .cpp
│   │   └── PatientRecord.h / .cpp
│   ├── controller/
│   │   ├── AppointmentController.h / .cpp
│   │   ├── ClinicManager.h / .cpp
│   │   └── SearchEngine.h / .cpp
│   ├── gui/
│   │   ├── AdminPanel.h / .cpp
│   │   ├── AppointmentPanel.h / .cpp
│   │   ├── ClinicTheme.h
│   │   ├── DashboardPanel.h / .cpp
│   │   ├── MainWindow.h / .cpp
│   │   ├── PatientPanel.h / .cpp
│   │   ├── QueuePanel.h / .cpp
│   │   └── SearchPanel.h / .cpp
│   └── utils/
│       ├── ClinicExceptions.h
│       ├── FileHandler.h / .cpp
│       └── IDGenerator.h / .cpp
└── tests/
    └── clinic_tests.cpp
```

## Build & Run (macOS / Linux with Qt 6)

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qtbase
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/scapms
```
# Zipporah_Smart_Clinic_Appointment_And-_Patient_Management
