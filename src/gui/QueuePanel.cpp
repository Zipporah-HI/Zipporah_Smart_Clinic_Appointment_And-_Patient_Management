#include "gui/QueuePanel.h"

#include "gui/ClinicTheme.h"
#include "utils/ClinicExceptions.h"

#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace scapms::gui {

QueuePanel::QueuePanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    refreshData();
}

void QueuePanel::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(14);

    // Top Controls Card
    auto* controlsGroup = new QGroupBox("Consultation Waiting Queue Controls (std::queue FIFO)");
    auto* controlsLayout = new QHBoxLayout(controlsGroup);
    controlsLayout->setSpacing(12);

    patientCombo_ = new QComboBox();
    patientCombo_->setMinimumWidth(300);
    patientCombo_->setToolTip("Select a registered patient to check into the waiting room queue.");

    auto* enqueueBtn = new QPushButton("Add Patient to Queue");
    enqueueBtn->setToolTip("Push patient to the back of the FIFO consultation queue.");

    auto* callNextBtn = new QPushButton("Call Next Patient (Dequeue)");
    callNextBtn->setProperty("variant", "warning");
    callNextBtn->setToolTip("Serve and remove the patient at the front of the waiting queue.");

    controlsLayout->addWidget(new QLabel("Select Patient:"));
    controlsLayout->addWidget(patientCombo_, 1);
    controlsLayout->addWidget(enqueueBtn);
    controlsLayout->addWidget(callNextBtn);
    rootLayout->addWidget(controlsGroup);

    // Now Serving Banner
    auto* servingGroup = new QGroupBox("Consultation Call Status");
    auto* servingLayout = new QHBoxLayout(servingGroup);
    currentCalledLabel_ = new QLabel("No patient called yet. Click 'Call Next Patient' to begin consultations.");
    currentCalledLabel_->setStyleSheet("font-size: 14px; font-weight: 700; color: #0F2942;");
    queueSummaryLabel_ = new QLabel("Waiting: 0");
    queueSummaryLabel_->setStyleSheet(
        "color: #0F766E; font-weight: 700; background-color: #CCFBF1; padding: 6px 12px; border-radius: 6px;");
    servingLayout->addWidget(currentCalledLabel_, 1);
    servingLayout->addWidget(queueSummaryLabel_);
    rootLayout->addWidget(servingGroup);

    // Queue Table
    auto* tableGroup = new QGroupBox("Current Waiting Room Queue (First-In, First-Out Order)");
    auto* tableLayout = new QVBoxLayout(tableGroup);

    queueTable_ = new QTableWidget(0, 7);
    queueTable_->setHorizontalHeaderLabels(
        {"Queue Pos", "Patient ID", "Name", "Category", "Priority", "Wait Elapsed", "Clinical Intake Summary"});
    queueTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    queueTable_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    queueTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    queueTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    queueTable_->setAlternatingRowColors(true);
    tableLayout->addWidget(queueTable_);

    rootLayout->addWidget(tableGroup, 1);

    connect(enqueueBtn, &QPushButton::clicked, this, &QueuePanel::onEnqueuePatient);
    connect(callNextBtn, &QPushButton::clicked, this, &QueuePanel::onCallNextPatient);
}

void QueuePanel::onEnqueuePatient() {
    try {
        if (patientCombo_->currentIndex() < 0) {
            throw utils::InvalidPatientException("No registered patients available to enqueue.");
        }
        const std::string pid = patientCombo_->currentData().toString().toStdString();
        manager_.addToWaitingQueue(pid);
        enqueueTimes_[pid] = QDateTime::currentDateTime();
        refreshData();
        emit dataChanged(QString("Enqueued patient %1 to waiting room.").arg(QString::fromStdString(pid)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Queue Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to add patient to queue.");
    }
}

void QueuePanel::onCallNextPatient() {
    try {
        model::PatientRecord* next = manager_.callNextPatient();
        if (!next) {
            currentCalledLabel_->setText("Called next patient (record no longer available).");
        } else {
            const std::string pid = next->getPatientId();
            enqueueTimes_.erase(pid);
            currentCalledLabel_->setText(
                QString("Now Serving: %1 — %2 [%3 | %4]")
                    .arg(QString::fromStdString(next->getPatientId()),
                         QString::fromStdString(next->getName()),
                         QString::fromStdString(next->getPatientCategory()),
                         QString::fromStdString(next->getPriorityLabel())));
        }
        refreshData();
        emit dataChanged(currentCalledLabel_->text());
    } catch (const utils::ClinicException& ex) {
        QMessageBox::information(this, "Waiting Queue", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Failed to dequeue next patient.");
    }
}

void QueuePanel::updateWaitingTimers() {
    const QDateTime now = QDateTime::currentDateTime();
    for (int row = 0; row < queueTable_->rowCount(); ++row) {
        auto* idItem = queueTable_->item(row, 1);
        auto* waitItem = queueTable_->item(row, 5);
        if (!idItem || !waitItem) {
            continue;
        }
        const std::string pid = idItem->text().toStdString();
        auto it = enqueueTimes_.find(pid);
        if (it == enqueueTimes_.end()) {
            enqueueTimes_[pid] = now.addSecs(-(row + 1) * 95);
            it = enqueueTimes_.find(pid);
        }
        const qint64 secs = it->second.secsTo(now);
        const qint64 mins = secs / 60;
        const qint64 rem = secs % 60;
        waitItem->setText(QString("%1m %2s").arg(mins, 2, 10, QChar('0')).arg(rem, 2, 10, QChar('0')));
    }
}

void QueuePanel::refreshData() {
    const QString prevPid = patientCombo_->currentData().toString();
    patientCombo_->clear();
    for (const auto& p : manager_.getDatabase().getPatients()) {
        patientCombo_->addItem(
            QString("%1 — %2 (%3)")
                .arg(QString::fromStdString(p->getPatientId()),
                     QString::fromStdString(p->getName()),
                     QString::fromStdString(p->getPatientCategory())),
            QString::fromStdString(p->getPatientId()));
    }
    const int idx = patientCombo_->findData(prevPid);
    if (idx >= 0) {
        patientCombo_->setCurrentIndex(idx);
    }

    const auto snapshot = manager_.getDatabase().getQueueSnapshot();
    queueSummaryLabel_->setText(QString("Waiting in Queue: %1").arg(snapshot.size()));
    queueTable_->setRowCount(static_cast<int>(snapshot.size()));

    const QDateTime now = QDateTime::currentDateTime();
    for (int row = 0; row < static_cast<int>(snapshot.size()); ++row) {
        const std::string& pid = snapshot[static_cast<std::size_t>(row)];
        const auto* p = manager_.getDatabase().findPatientRaw(pid);
        if (enqueueTimes_.find(pid) == enqueueTimes_.end()) {
            enqueueTimes_[pid] = now.addSecs(-(row + 1) * 95);
        }

        const QString posLabel = (row == 0) ? "#1 (Next)" : QString("#%1").arg(row + 1);
        queueTable_->setItem(row, 0, new QTableWidgetItem(posLabel));
        queueTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(pid)));

        if (p) {
            const QString cat = QString::fromStdString(p->getPatientCategory());
            const QString pri = QString::fromStdString(p->getPriorityLabel());
            queueTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(p->getName())));

            auto* catItem = new QTableWidgetItem(cat);
            catItem->setBackground(categoryBgColor(cat));
            catItem->setForeground(categoryFgColor(cat));
            QFont boldFont = catItem->font();
            boldFont.setBold(true);
            catItem->setFont(boldFont);
            queueTable_->setItem(row, 3, catItem);

            auto* priItem = new QTableWidgetItem(pri);
            priItem->setBackground(categoryBgColor(cat));
            priItem->setForeground(categoryFgColor(cat));
            priItem->setFont(boldFont);
            queueTable_->setItem(row, 4, priItem);

            queueTable_->setItem(row, 5, new QTableWidgetItem("00m 00s"));
            queueTable_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(p->getDetailsSummary())));
        }
    }
    updateWaitingTimers();
}

} // namespace scapms::gui
