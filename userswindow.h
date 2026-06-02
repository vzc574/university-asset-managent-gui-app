#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QMap>
class UsersWindow : public QWidget {
    Q_OBJECT
public:
    explicit UsersWindow(int userId, const QString &role, QWidget *parent = nullptr);

private slots:
    void loadData();
    void addUser();
    void toggleActive();

private:
    QTableWidget *table;
    QLineEdit *searchEdit;
    QPushButton *addBtn, *toggleBtn, *refreshBtn;
    int m_userId;
    QString m_role;
    QTabWidget *roleTabs;
    QMap<QString, QTableWidget*> roleTables;

    void setupUserTable(QTableWidget *t);
    void setupUI();
    void styleButton(QPushButton *btn, const QString &color);
};