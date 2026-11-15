#ifndef SCAPMS_GUI_CLINIC_THEME_H
#define SCAPMS_GUI_CLINIC_THEME_H

#include <QColor>
#include <QString>

namespace scapms::gui {

inline QString clinicStylesheet() {
    return R"(
        QMainWindow, QWidget {
            background-color: #F4F7FB;
            color: #1E293B;
            font-family: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", sans-serif;
            font-size: 13px;
        }
        QLabel {
            background-color: transparent;
        }
        QMenuBar {
            background-color: #0F2942;
            color: #F8FAFC;
            padding: 4px 8px;
        }
        QMenuBar::item {
            background: transparent;
            padding: 6px 12px;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #1E4976;
        }
        QMenu {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px 6px 12px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #E0F2FE;
            color: #0F2942;
        }
        QTabWidget::pane {
            border: 1px solid #CBD5E1;
            border-radius: 8px;
            background-color: #FFFFFF;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #E2E8F0;
            color: #334155;
            padding: 10px 18px;
            margin-right: 4px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background-color: #FFFFFF;
            color: #0F766E;
            border: 1px solid #CBD5E1;
            border-bottom: 2px solid #0F766E;
        }
        QTabBar::tab:hover:!selected {
            background-color: #CBD5E1;
        }
        QGroupBox {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            margin-top: 12px;
            padding: 14px 12px 12px 12px;
            font-weight: 600;
            color: #0F2942;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 6px;
            background-color: #FFFFFF;
        }
        QLineEdit, QSpinBox, QComboBox, QDateEdit, QTextEdit {
            background-color: #FFFFFF;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            padding: 6px 10px;
            min-height: 22px;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QDateEdit:focus, QTextEdit:focus {
            border: 1.5px solid #0F766E;
        }
        QPushButton {
            background-color: #0F766E;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: #0D9488;
        }
        QPushButton:pressed {
            background-color: #115E59;
        }
        QPushButton:disabled {
            background-color: #94A3B8;
            color: #F1F5F9;
        }
        QPushButton[variant="secondary"] {
            background-color: #E2E8F0;
            color: #1E293B;
        }
        QPushButton[variant="secondary"]:hover {
            background-color: #CBD5E1;
        }
        QPushButton[variant="danger"] {
            background-color: #DC2626;
            color: #FFFFFF;
        }
        QPushButton[variant="danger"]:hover {
            background-color: #B91C1C;
        }
        QPushButton[variant="warning"] {
            background-color: #D97706;
            color: #FFFFFF;
        }
        QPushButton[variant="warning"]:hover {
            background-color: #B45309;
        }
        QTableWidget {
            background-color: #FFFFFF;
            alternate-background-color: #F8FAFC;
            gridline-color: #E2E8F0;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            selection-background-color: #CCFBF1;
            selection-color: #0F2942;
        }
        QHeaderView::section {
            background-color: #F1F5F9;
            color: #0F2942;
            padding: 7px 10px;
            border: none;
            border-bottom: 1px solid #CBD5E1;
            border-right: 1px solid #E2E8F0;
            font-weight: 600;
        }
        QFrame#AlertBanner {
            background-color: #EFF6FF;
            border: 1px solid #BFDBFE;
            border-radius: 8px;
            padding: 8px 12px;
        }
        QFrame#StatCard {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 12px;
        }
        QStatusBar {
            background-color: #0F2942;
            color: #E2E8F0;
        }
    )";
}

inline QColor categoryBgColor(const QString& category) {
    if (category == "Emergency") {
        return QColor("#FEE2E2");
    }
    if (category == "Follow-Up") {
        return QColor("#FEF3C7");
    }
    return QColor("#DCFCE7");
}

inline QColor categoryFgColor(const QString& category) {
    if (category == "Emergency") {
        return QColor("#991B1B");
    }
    if (category == "Follow-Up") {
        return QColor("#92400E");
    }
    return QColor("#166534");
}

inline QColor statusBgColor(const QString& status) {
    if (status == "Completed") {
        return QColor("#DCFCE7");
    }
    if (status == "Cancelled") {
        return QColor("#F1F5F9");
    }
    return QColor("#FEF3C7");
}

inline QColor statusFgColor(const QString& status) {
    if (status == "Completed") {
        return QColor("#166534");
    }
    if (status == "Cancelled") {
        return QColor("#64748B");
    }
    return QColor("#92400E");
}

} // namespace scapms::gui

#endif // SCAPMS_GUI_CLINIC_THEME_H
