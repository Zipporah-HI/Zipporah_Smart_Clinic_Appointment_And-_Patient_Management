#ifndef SCAPMS_GUI_DASHBOARD_PANEL_H
#define SCAPMS_GUI_DASHBOARD_PANEL_H

#include "controller/ClinicManager.h"

#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace scapms::gui {

class DashboardPanel : public QWidget {
    Q_OBJECT

public:
    explicit DashboardPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refreshData();
    void updateAlertBanner(const QString& alertMessage);

signals:
    void navigateToTab(int tabIndex);

private:
    void setupUi();

    controller::ClinicManager& manager_;
    QLabel* clockLabel_ = nullptr;
    QLabel* alertLabel_ = nullptr;
    QLabel* totalPatientsValue_ = nullptr;
    QLabel* pendingAppointmentsValue_ = nullptr;
    QLabel* queueSizeValue_ = nullptr;
    QLabel* emergencyCountValue_ = nullptr;
    QTableWidget* cacheTable_ = nullptr;
    QListWidget* polymorphicLogList_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_DASHBOARD_PANEL_H
