#include "gui/DashboardPanel.h"

#include "gui/ClinicTheme.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace scapms::gui {

namespace {
QFrame* createStatCard(const QString& title, const QString& accentHex, QLabel*& valueLabelOut) {
    auto* card = new QFrame();
    card->setObjectName("StatCard");
    card->setStyleSheet(
        QString("QFrame#StatCard { background-color: #FFFFFF; border: 1px solid #E2E8F0; "
                "border-left: 5px solid %1; border-radius: 10px; padding: 12px; }")
            .arg(accentHex));

    auto* layout = new QVBoxLayout(card);
    layout->setSpacing(4);

    auto* titleLbl = new QLabel(title);
    titleLbl->setStyleSheet("color: #64748B; font-size: 12px; font-weight: 600;");

    valueLabelOut = new QLabel("0");
    valueLabelOut->setStyleSheet("color: #0F2942; font-size: 26px; font-weight: 700;");

    layout->addWidget(titleLbl);
    layout->addWidget(valueLabelOut);
    return card;
}
} // namespace

DashboardPanel::DashboardPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    refreshData();
}

void DashboardPanel::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(14);

    // Header row
    auto* headerLayout = new QHBoxLayout();
    auto* titleBlock = new QVBoxLayout();
    auto* titleLabel = new QLabel("University Clinic Operations Dashboard");
    titleLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #0F2942;");
    auto* subtitleLabel = new QLabel(
        "Smart Clinic Appointment & Patient Management System (SCAPMS) — Real-Time Overview");
    subtitleLabel->setStyleSheet("color: #64748B; font-size: 12px;");
    titleBlock->addWidget(titleLabel);
    titleBlock->addWidget(subtitleLabel);

    clockLabel_ = new QLabel();
    clockLabel_->setStyleSheet(
        "color: #0F766E; font-weight: 600; background-color: #CCFBF1; "
        "padding: 6px 12px; border-radius: 6px;");

    headerLayout->addLayout(titleBlock);
    headerLayout->addStretch();
    headerLayout->addWidget(clockLabel_);
    rootLayout->addLayout(headerLayout);

    // Live Timer Alert Banner
    auto* alertBanner = new QFrame();
    alertBanner->setObjectName("AlertBanner");
    auto* alertLayout = new QHBoxLayout(alertBanner);
    alertLabel_ = new QLabel("Timer Active: Monitoring upcoming clinic appointments and waiting room queue.");
    alertLabel_->setStyleSheet("color: #1E3A8A; font-weight: 600;");
    alertLabel_->setWordWrap(true);
    alertLayout->addWidget(alertLabel_);
    rootLayout->addWidget(alertBanner);

    // KPI Stat Grid (QGridLayout)
    auto* statsGrid = new QGridLayout();
    statsGrid->setSpacing(12);
    statsGrid->addWidget(createStatCard("Registered Patients (std::vector)", "#0F766E", totalPatientsValue_), 0, 0);
    statsGrid->addWidget(createStatCard("Pending Appointments", "#D97706", pendingAppointmentsValue_), 0, 1);
    statsGrid->addWidget(createStatCard("Waiting Room Queue (std::queue)", "#2563EB", queueSizeValue_), 0, 2);
    statsGrid->addWidget(createStatCard("Emergency / High Priority", "#DC2626", emergencyCountValue_), 0, 3);
    rootLayout->addLayout(statsGrid);

    // Quick Action Bar
    auto* quickActionsGroup = new QGroupBox("Quick Clinic Actions");
    auto* quickLayout = new QHBoxLayout(quickActionsGroup);
    auto* btnPatients = new QPushButton("Register / Manage Patients");
    btnPatients->setToolTip("Open the Patient Records panel to register, edit, or delete patients.");
    auto* btnAppts = new QPushButton("Schedule Appointment");
    btnAppts->setToolTip("Open the Appointments panel to schedule, complete, or cancel visits.");
    auto* btnQueue = new QPushButton("Manage Waiting Queue");
    btnQueue->setToolTip("Open the Waiting Queue panel to enqueue or call the next patient.");
    auto* btnSearch = new QPushButton("Search && Sort Records");
    btnSearch->setToolTip("Open the Search & Sort panel to test Linear, Binary, Recursive Search & Sorting.");
    auto* btnReports = new QPushButton("View Clinic Reports");
    btnReports->setToolTip("Open Reports & Admin panel for clinic analytics and Undo stack.");

    connect(btnPatients, &QPushButton::clicked, this, [this]() { emit navigateToTab(1); });
    connect(btnAppts, &QPushButton::clicked, this, [this]() { emit navigateToTab(2); });
    connect(btnQueue, &QPushButton::clicked, this, [this]() { emit navigateToTab(3); });
    connect(btnSearch, &QPushButton::clicked, this, [this]() { emit navigateToTab(4); });
    connect(btnReports, &QPushButton::clicked, this, [this]() { emit navigateToTab(5); });

    quickLayout->addWidget(btnPatients);
    quickLayout->addWidget(btnAppts);
    quickLayout->addWidget(btnQueue);
    quickLayout->addWidget(btnSearch);
    quickLayout->addWidget(btnReports);
    rootLayout->addWidget(quickActionsGroup);

    // Bottom split: Cache Array + Polymorphic Assessment Log
    auto* bottomSplit = new QHBoxLayout();
    bottomSplit->setSpacing(12);

    auto* cacheGroup = new QGroupBox("Frequently Accessed Patients (Fixed Array Cache[10])");
    auto* cacheLayout = new QVBoxLayout(cacheGroup);
    cacheTable_ = new QTableWidget(0, 5);
    cacheTable_->setHorizontalHeaderLabels({"Patient ID", "Name", "Category", "Priority", "Lookups"});
    cacheTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    cacheTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cacheTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    cacheTable_->setAlternatingRowColors(true);
    cacheLayout->addWidget(cacheTable_);

    auto* polyGroup = new QGroupBox("Runtime Polymorphism Dispatch Log (PatientRecord&& + Appointable*)");
    auto* polyLayout = new QVBoxLayout(polyGroup);
    polymorphicLogList_ = new QListWidget();
    polymorphicLogList_->setAlternatingRowColors(true);
    polymorphicLogList_->setWordWrap(true);
    auto* refreshPolyBtn = new QPushButton("Run Polymorphic Assessment");
    refreshPolyBtn->setProperty("variant", "secondary");
    refreshPolyBtn->setToolTip("Invokes processPatientPolymorphically(const PatientRecord&) across all subclasses.");
    connect(refreshPolyBtn, &QPushButton::clicked, this, &DashboardPanel::refreshData);
    polyLayout->addWidget(polymorphicLogList_);
    polyLayout->addWidget(refreshPolyBtn);

    bottomSplit->addWidget(cacheGroup, 5);
    bottomSplit->addWidget(polyGroup, 6);
    rootLayout->addLayout(bottomSplit, 1);
}

void DashboardPanel::updateAlertBanner(const QString& alertMessage) {
    if (alertLabel_) {
        alertLabel_->setText(alertMessage);
    }
    if (clockLabel_) {
        clockLabel_->setText(QDateTime::currentDateTime().toString("ddd, MMM dd yyyy | hh:mm:ss AP"));
    }
}

void DashboardPanel::refreshData() {
    if (clockLabel_) {
        clockLabel_->setText(QDateTime::currentDateTime().toString("ddd, MMM dd yyyy | hh:mm:ss AP"));
    }

    const auto dist = manager_.getCategoryDistribution();
    const auto summary = manager_.getDailyAppointmentSummary();

    totalPatientsValue_->setText(QString::number(dist.totalPatients));
    pendingAppointmentsValue_->setText(QString::number(summary.pendingCount));
    queueSizeValue_->setText(QString::number(manager_.getDatabase().getQueueSize()));
    emergencyCountValue_->setText(QString::number(dist.emergencyCount));

    // Populate Fixed Array Cache table
    const auto cached = manager_.getDatabase().getFrequentlyAccessedPatients();
    cacheTable_->setRowCount(static_cast<int>(cached.size()));
    for (int row = 0; row < static_cast<int>(cached.size()); ++row) {
        const auto* p = cached[static_cast<std::size_t>(row)];
        const QString cat = QString::fromStdString(p->getPatientCategory());
        const QString pri = QString::fromStdString(p->getPriorityLabel());

        cacheTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p->getPatientId())));
        cacheTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p->getName())));

        auto* catItem = new QTableWidgetItem(cat);
        catItem->setBackground(categoryBgColor(cat));
        catItem->setForeground(categoryFgColor(cat));
        cacheTable_->setItem(row, 2, catItem);

        auto* priItem = new QTableWidgetItem(pri);
        priItem->setBackground(categoryBgColor(cat));
        priItem->setForeground(categoryFgColor(cat));
        cacheTable_->setItem(row, 3, priItem);

        cacheTable_->setItem(row, 4, new QTableWidgetItem(QString::number(p->getAccessCount())));
    }

    // Populate Runtime Polymorphism Log
    polymorphicLogList_->clear();
    const auto logs = manager_.generatePolymorphicAssessmentLog();
    for (const auto& entry : logs) {
        polymorphicLogList_->addItem(QString::fromStdString(entry));
    }
}

} // namespace scapms::gui
