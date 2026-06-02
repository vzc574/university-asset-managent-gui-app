#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
        QWidget {
            color: #1a1a2e;
            background-color: #f0f2f5;
        }

        QLabel {
            color: #1a1a2e;
        }

        QLineEdit, QTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            color: #1a1a2e;
            background-color: white;
            border: 1px solid #ccc;
            border-radius: 6px;
            padding: 6px;
        }

        QTableWidget {
            color: #1a1a2e;
            background-color: white;
        }

        QTableWidget::item {
            color: #1a1a2e;
            background-color: white;
        }

        QTableWidget::item:selected {
            color: white;
            background-color: #0f6e56;
        }

        QHeaderView::section {
            color: white;
            background-color: #1a1a2e;
        }

        QPushButton {
            color: white;
            background-color: #0f6e56;
            border-radius: 6px;
            padding: 8px 14px;
        }
    )");

    Widget w;
    w.show();
    return a.exec();
}