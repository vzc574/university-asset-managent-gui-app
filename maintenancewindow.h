#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class MaintenanceWindow : public QWidget {
    Q_OBJECT
public:
    explicit MaintenanceWindow(int userId, const QString &role, QWidget *parent = nullptr);

private slots:
    void loadData();
    void reportIssue();
    void resolveIssue();
    void filterTable();
    void greedyPrioritize();   // Greedy scheduling: highest urgency first (Ch. 9)

private:
    QTableWidget *table;
    QLineEdit    *searchEdit;
    QComboBox    *statusFilter;
    QPushButton  *reportBtn, *resolveBtn, *refreshBtn, *prioritizeBtn;
    QLabel       *greedyLabel;
    int     m_userId;
    QString m_role;

    void setupUI();
    void styleButton(QPushButton *btn, const QString &color);
};