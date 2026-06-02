#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class LostFoundWindow : public QWidget {
    Q_OBJECT
public:
    explicit LostFoundWindow(int userId, const QString &role, QWidget *parent = nullptr);

private slots:
    void loadData();
    void addReport();
    void markClaimed();
    void filterTable();

private:
    void setupUI();
    void styleButton(QPushButton *btn, const QString &color);

    QTableWidget *table;
    QLineEdit    *searchEdit;
    QComboBox    *typeFilter;
    QPushButton  *addBtn;
    QPushButton  *claimBtn;
    QPushButton  *refreshBtn;

    int     m_userId;
    QString m_role;
};