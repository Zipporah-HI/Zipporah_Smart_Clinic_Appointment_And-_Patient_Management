#include "gui/SearchPanel.h"

#include "gui/ClinicTheme.h"
#include "utils/ClinicExceptions.h"

#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace scapms::gui {

SearchPanel::SearchPanel(controller::ClinicManager& manager, QWidget* parent)
    : QWidget(parent), manager_(manager) {
    setupUi();
    refreshAllRecords();
}

void SearchPanel::setupUi() {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto* topGrid = new QGridLayout();
    topGrid->setSpacing(12);

    // Search Card
    auto* searchGroup = new QGroupBox("Patient Search (Manual Linear, Binary && Recursive Search)");
    auto* searchLayout = new QVBoxLayout(searchGroup);

    auto* searchRow1 = new QHBoxLayout();
    searchQueryEdit_ = new QLineEdit();
    searchQueryEdit_->setPlaceholderText("Enter Patient ID (e.g., PAT-1003), Name, or Category (Emergency)...");
    searchQueryEdit_->setToolTip("Type Patient ID, partial name, or category; press Enter or click Search.");

    searchAlgoCombo_ = new QComboBox();
    searchAlgoCombo_->addItem("Auto-Adaptive (Sorted/Unsorted)", static_cast<int>(controller::SearchAlgorithm::AutoAdaptive));
    searchAlgoCombo_->addItem("Linear Search (by ID)", static_cast<int>(controller::SearchAlgorithm::LinearSearchId));
    searchAlgoCombo_->addItem("Binary Search (by ID)", static_cast<int>(controller::SearchAlgorithm::BinarySearchId));
    searchAlgoCombo_->addItem("Recursive Search (by Name)", static_cast<int>(controller::SearchAlgorithm::RecursiveNameSearch));
    searchAlgoCombo_->addItem("Linear Scan (by Category)", static_cast<int>(controller::SearchAlgorithm::CategoryLinearSearch));
    searchAlgoCombo_->setToolTip("Choose a manual searching algorithm or let SCAPMS adapt to the data's sorted state.");

    auto* searchBtn = new QPushButton("Search");
    searchBtn->setToolTip("Execute the selected manual search algorithm.");
    auto* resetBtn = new QPushButton("Show All");
    resetBtn->setProperty("variant", "secondary");
    resetBtn->setToolTip("Clear search filter and show all patient records.");

    searchRow1->addWidget(searchQueryEdit_, 2);
    searchRow1->addWidget(searchAlgoCombo_, 1);
    searchRow1->addWidget(searchBtn);
    searchRow1->addWidget(resetBtn);

    dataStateLabel_ = new QLabel();
    dataStateLabel_->setStyleSheet("color: #0F766E; font-weight: 600; font-size: 12px;");

    searchResultBanner_ = new QLabel("Enter a keyword or select an algorithm to search patient records.");
    searchResultBanner_->setWordWrap(true);
    searchResultBanner_->setStyleSheet("color: #334155; font-size: 12px;");

    searchLayout->addLayout(searchRow1);
    searchLayout->addWidget(dataStateLabel_);
    searchLayout->addWidget(searchResultBanner_);

    // Sort Card
    auto* sortGroup = new QGroupBox("Record Sorting (Manual Merge, Insertion && Selection Sort)");
    auto* sortLayout = new QVBoxLayout(sortGroup);

    auto* sortControls = new QHBoxLayout();
    sortAlgoCombo_ = new QComboBox();
    sortAlgoCombo_->addItem("Merge Sort", static_cast<int>(controller::SortAlgorithm::MergeSort));
    sortAlgoCombo_->addItem("Insertion Sort", static_cast<int>(controller::SortAlgorithm::InsertionSort));
    sortAlgoCombo_->addItem("Selection Sort", static_cast<int>(controller::SortAlgorithm::SelectionSort));
    sortAlgoCombo_->setToolTip("Select a student-implemented sorting algorithm.");

    sortFieldCombo_ = new QComboBox();
    sortFieldCombo_->addItem("Patient Name", static_cast<int>(model::SortField::Name));
    sortFieldCombo_->addItem("Patient ID", static_cast<int>(model::SortField::PatientId));
    sortFieldCombo_->addItem("Age", static_cast<int>(model::SortField::Age));
    sortFieldCombo_->addItem("Appointment Date", static_cast<int>(model::SortField::AppointmentDate));
    sortFieldCombo_->setToolTip("Select the patient attribute to sort by.");

    sortOrderCombo_ = new QComboBox();
    sortOrderCombo_->addItem("Ascending", true);
    sortOrderCombo_->addItem("Descending", false);

    auto* sortBtn = new QPushButton("SORT");
    sortBtn->setToolTip("Sort patient records in-place using the selected manual algorithm.");

    sortControls->addWidget(new QLabel("Sort Algorithm:"));
    sortControls->addWidget(sortAlgoCombo_);
    sortControls->addWidget(new QLabel("Sort By:"));
    sortControls->addWidget(sortFieldCombo_);
    sortControls->addWidget(sortOrderCombo_);
    sortControls->addWidget(sortBtn);

    sortExplanationLabel_ = new QLabel();
    sortExplanationLabel_->setWordWrap(true);
    sortExplanationLabel_->setStyleSheet(
        "color: #1E3A8A; background-color: #EFF6FF; border: 1px solid #BFDBFE; "
        "border-radius: 6px; padding: 8px; font-size: 12px;");

    sortLayout->addLayout(sortControls);
    sortLayout->addWidget(sortExplanationLabel_);

    topGrid->addWidget(searchGroup, 0, 0);
    topGrid->addWidget(sortGroup, 1, 0);
    rootLayout->addLayout(topGrid);

    // Results Table
    auto* tableGroup = new QGroupBox("Search && Sort Output View");
    auto* tableLayout = new QVBoxLayout(tableGroup);

    resultsTable_ = new QTableWidget(0, 7);
    resultsTable_->setHorizontalHeaderLabels(
        {"Patient ID", "Name", "Age", "Category", "Priority", "Next / Latest Appt", "Details"});
    resultsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    resultsTable_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    resultsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultsTable_->setAlternatingRowColors(true);
    tableLayout->addWidget(resultsTable_);
    rootLayout->addWidget(tableGroup, 1);

    connect(searchBtn, &QPushButton::clicked, this, &SearchPanel::onExecuteSearch);
    connect(searchQueryEdit_, &QLineEdit::returnPressed, this, &SearchPanel::onExecuteSearch);
    connect(searchQueryEdit_, &QLineEdit::textChanged, this, &SearchPanel::onSearchTextChanged);
    connect(resetBtn, &QPushButton::clicked, this, &SearchPanel::onResetSearchView);
    connect(sortBtn, &QPushButton::clicked, this, &SearchPanel::onExecuteSort);
    connect(sortAlgoCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SearchPanel::onSortSelectionChanged);
    connect(sortFieldCombo_, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &SearchPanel::onSortSelectionChanged);

    onSortSelectionChanged();
}

void SearchPanel::focusSearchInput() {
    if (searchQueryEdit_) {
        searchQueryEdit_->setFocus();
        searchQueryEdit_->selectAll();
    }
}

void SearchPanel::onSearchTextChanged(const QString& text) {
    if (text.trimmed().isEmpty()) {
        searchResultBanner_->setText("Showing all registered patients. Type a query to search.");
        refreshAllRecords();
    }
}

void SearchPanel::onExecuteSearch() {
    try {
        const std::string query = searchQueryEdit_->text().trimmed().toStdString();
        const auto algo = static_cast<controller::SearchAlgorithm>(
            searchAlgoCombo_->currentData().toInt());

        const auto outcome = controller::SearchEngine::executeSearch(
            manager_.getDatabase(), query, algo);

        populateTable(outcome.matches);
        updateSortedStateBadge();

        searchResultBanner_->setText(
            QString("Algorithm: %1 | Matches: %2 | Comparisons: %3 — %4")
                .arg(QString::fromStdString(outcome.algorithmUsed))
                .arg(outcome.matches.size())
                .arg(outcome.comparisons)
                .arg(QString::fromStdString(outcome.rationale)));

        emit dataChanged(QString("Search completed via %1 (%2 match(es)).")
                             .arg(QString::fromStdString(outcome.algorithmUsed))
                             .arg(outcome.matches.size()));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Search Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Search operation failed.");
    }
}

void SearchPanel::onResetSearchView() {
    searchQueryEdit_->clear();
    refreshAllRecords();
}

void SearchPanel::onSortSelectionChanged() {
    const auto algo = static_cast<controller::SortAlgorithm>(
        sortAlgoCombo_->currentData().toInt());
    const auto field = static_cast<model::SortField>(
        sortFieldCombo_->currentData().toInt());

    const std::string explanation = controller::SearchEngine::explainSortChoice(
        algo, field, manager_.getDatabase().getPatientCount());
    sortExplanationLabel_->setText(QString::fromStdString(explanation));
}

void SearchPanel::onExecuteSort() {
    try {
        const auto algo = static_cast<controller::SortAlgorithm>(
            sortAlgoCombo_->currentData().toInt());
        const auto field = static_cast<model::SortField>(
            sortFieldCombo_->currentData().toInt());
        const bool ascending = sortOrderCombo_->currentData().toBool();

        const auto outcome = controller::SearchEngine::sortPatients(
            manager_.getDatabase(), algo, field, ascending);

        refreshAllRecords();
        sortExplanationLabel_->setText(
            QString("%1 | Comparisons: %2 | Moves/Swaps: %3")
                .arg(QString::fromStdString(outcome.rationale))
                .arg(outcome.comparisons)
                .arg(outcome.swapsOrMoves));

        emit dataChanged(QString("Sorted %1 records by %2 using %3.")
                             .arg(manager_.getDatabase().getPatientCount())
                             .arg(QString::fromStdString(outcome.sortFieldLabel))
                             .arg(QString::fromStdString(outcome.algorithmUsed)));
    } catch (const utils::ClinicException& ex) {
        QMessageBox::warning(this, "Sort Error", QString::fromStdString(ex.what()));
    } catch (...) {
        QMessageBox::critical(this, "Unexpected Error", "Sort operation failed.");
    }
}

void SearchPanel::updateSortedStateBadge() {
    const std::string state = controller::SearchEngine::sortFieldToString(
        manager_.getDatabase().getSortedState());
    dataStateLabel_->setText(
        QString("Current Dataset Order State: %1").arg(QString::fromStdString(state)));
}

void SearchPanel::populateTable(const std::vector<model::PatientRecord*>& records) {
    resultsTable_->setRowCount(static_cast<int>(records.size()));
    for (int row = 0; row < static_cast<int>(records.size()); ++row) {
        const auto* p = records[static_cast<std::size_t>(row)];
        if (!p) {
            continue;
        }
        const QString cat = QString::fromStdString(p->getPatientCategory());
        const QString pri = QString::fromStdString(p->getPriorityLabel());
        const std::string nextAppt =
            manager_.getDatabase().getEarliestPendingOrLatestAppointmentDate(p->getPatientId());
        const QString apptDisplay = (nextAppt == "9999-99-99 99:99")
                                        ? "None"
                                        : QString::fromStdString(nextAppt);

        resultsTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p->getPatientId())));
        resultsTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p->getName())));
        resultsTable_->setItem(row, 2, new QTableWidgetItem(QString::number(p->getAge())));

        auto* catItem = new QTableWidgetItem(cat);
        catItem->setBackground(categoryBgColor(cat));
        catItem->setForeground(categoryFgColor(cat));
        QFont boldFont = catItem->font();
        boldFont.setBold(true);
        catItem->setFont(boldFont);
        resultsTable_->setItem(row, 3, catItem);

        auto* priItem = new QTableWidgetItem(pri);
        priItem->setBackground(categoryBgColor(cat));
        priItem->setForeground(categoryFgColor(cat));
        priItem->setFont(boldFont);
        resultsTable_->setItem(row, 4, priItem);

        resultsTable_->setItem(row, 5, new QTableWidgetItem(apptDisplay));
        resultsTable_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(p->getDetailsSummary())));
    }
}

void SearchPanel::refreshAllRecords() {
    std::vector<model::PatientRecord*> allPtrs;
    const auto& patients = manager_.getDatabase().getPatients();
    allPtrs.reserve(patients.size());
    for (const auto& p : patients) {
        if (p) {
            allPtrs.push_back(p.get());
        }
    }
    populateTable(allPtrs);
    updateSortedStateBadge();
    onSortSelectionChanged();
}

} // namespace scapms::gui
