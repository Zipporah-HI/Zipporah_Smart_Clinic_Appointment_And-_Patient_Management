#ifndef SCAPMS_GUI_SEARCH_PANEL_H
#define SCAPMS_GUI_SEARCH_PANEL_H

#include "controller/ClinicManager.h"
#include "controller/SearchEngine.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace scapms::gui {

class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(controller::ClinicManager& manager, QWidget* parent = nullptr);

    void refreshAllRecords();
    void focusSearchInput();

signals:
    void dataChanged(const QString& statusMessage);

private slots:
    void onExecuteSearch();
    void onResetSearchView();
    void onExecuteSort();
    void onSortSelectionChanged();
    void onSearchTextChanged(const QString& text);

private:
    void setupUi();
    void populateTable(const std::vector<model::PatientRecord*>& records);
    void updateSortedStateBadge();

    controller::ClinicManager& manager_;
    QLineEdit* searchQueryEdit_ = nullptr;
    QComboBox* searchAlgoCombo_ = nullptr;
    QLabel* dataStateLabel_ = nullptr;
    QLabel* searchResultBanner_ = nullptr;

    QComboBox* sortAlgoCombo_ = nullptr;
    QComboBox* sortFieldCombo_ = nullptr;
    QComboBox* sortOrderCombo_ = nullptr;
    QLabel* sortExplanationLabel_ = nullptr;

    QTableWidget* resultsTable_ = nullptr;
};

} // namespace scapms::gui

#endif // SCAPMS_GUI_SEARCH_PANEL_H
