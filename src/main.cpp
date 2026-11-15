#include "gui/MainWindow.h"

#include <QApplication>
#include <QPixmap>
#include <QStringList>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SCAPMS");
    app.setOrganizationName("COS202 University Clinic");

    std::string dataDir = "data";
    std::string screenshotDir;

    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "--data-dir" && i + 1 < args.size()) {
            dataDir = args[++i].toStdString();
        } else if (args[i] == "--capture-screenshots" && i + 1 < args.size()) {
            screenshotDir = args[++i].toStdString();
        }
    }

    scapms::gui::MainWindow window(dataDir);
    window.show();

    if (!screenshotDir.empty()) {
        std::filesystem::create_directories(screenshotDir);
        const char* names[] = {
            "01_main_dashboard.png",
            "02_patient_registration.png",
            "03_appointment_management.png",
            "04_waiting_queue.png",
            "05_search_sort_interface.png",
            "06_reports_admin.png"
        };
        for (int i = 0; i < 6; ++i) {
            window.selectTab(i);
            app.processEvents();
            QPixmap pixmap = window.grab();
            const std::string outFile =
                (std::filesystem::path(screenshotDir) / names[i]).string();
            pixmap.save(QString::fromStdString(outFile), "PNG");
        }
        return 0;
    }

    return app.exec();
}
