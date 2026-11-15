#ifndef SCAPMS_GUI_QUEUE_PANEL_H
#define SCAPMS_GUI_QUEUE_PANEL_H

#include "controller/ClinicManager.h"

#include <QComboBox>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include <unordered_map>

namespace scapms::gui {

class QueuePanel : public QWidget {
    Q_OBJECT

public:
    explicit QueuePanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refreshData();
    void updateWaitingTimers();

signals:
    void dataChanged(const QString& statusMessage);

private slots:
    void onEnqueuePatient();
    void onCallNextPatient();

private:
    void setupUi();

    controller::ClinicManager& manager_;
    QComboBox* patientCombo_ = nullptr;
    QLabel* currentCalledLabel_ = nullptr;
    QLabel* queueSummaryLabel_ = nullptr;
    QTableWidget* queueTable_ = nullptr;
    std::unordered_map<std::string, QDateTime> enqueueTimes_;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_QUEUE_PANEL_H
