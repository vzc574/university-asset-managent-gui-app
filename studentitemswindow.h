#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class StudentItemsWindow : public QWidget {
    Q_OBJECT
public:
    explicit StudentItemsWindow(int userId, const QString &role, QWidget *parent = nullptr);

private slots:
    void loadData();
    void registerItem();
    void deleteItem();

private:
    QTableWidget *table;
    QLineEdit    *searchEdit;
    QPushButton  *registerBtn, *deleteBtn, *refreshBtn;
    QLabel       *statsLabel;
    int          m_userId;
    QString      m_role;

    void setupUI();
    void styleButton(QPushButton *btn, const QString &color);
    void applyStatusBadge(QTableWidgetItem *item, const QString &status);
};