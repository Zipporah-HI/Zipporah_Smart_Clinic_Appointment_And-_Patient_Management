#ifndef SCAPMS_GUI_ADMIN_PANEL_H
#define SCAPMS_GUI_ADMIN_PANEL_H

#include "controller/ClinicManager.h"

#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace scapms::gui {

class AdminPanel : public QWidget {
    Q_OBJECT

public:
    explicit AdminPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refreshData();

signals:
    void dataChanged(const QString& statusMessage);
    void requestSaveDialog();
    void requestLoadDialog();

private slots:
    void onUndoAction();
    void onFilterDateChanged();
    void onExportReport();

private:
    void setupUi();

    controller::ClinicManager& manager_;

    // Report 1 & 2 tables
    QTableWidget* attendedTable_ = nullptr;
    QTableWidget* pendingTable_ = nullptr;

    // Report 3: Category Distribution
    QLabel* generalCountLbl_ = nullptr;
    QLabel* emergencyCountLbl_ = nullptr;
    QLabel* followUpCountLbl_ = nullptr;
    QProgressBar* generalBar_ = nullptr;
    QProgressBar* emergencyBar_ = nullptr;
    QProgressBar* followUpBar_ = nullptr;

    // Report 4: Daily Appointment Summary
    QLineEdit* dateFilterEdit_ = nullptr;
    QLabel* dailyTotalLbl_ = nullptr;
    QLabel* dailyCompletedLbl_ = nullptr;
    QLabel* dailyPendingLbl_ = nullptr;
    QLabel* dailyCancelledLbl_ = nullptr;

    // Admin Undo Stack
    QLabel* undoStackStatusLbl_ = nullptr;
    QPushButton* undoBtn_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_ADMIN_PANEL_H
