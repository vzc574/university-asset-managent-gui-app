#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void doLogout();

private:
    void buildDashboard(const QString &role, int userId);
    void applyRoleAccess(const QString &role,
                         QPushButton *btnAssets,
                         QPushButton *btnMaint,
                         QPushButton *btnLostFound,
                         QPushButton *btnStudents,
                         QPushButton *btnUsers);
    Ui::Widget *ui;
    QString     m_role;
    int         m_userId;
    QWidget    *m_dashContainer = nullptr;
};

#endif