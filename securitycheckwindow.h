#ifndef SECURITYCHECKWINDOW_H
#define SECURITYCHECKWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include "dsaengine.h"

class SecurityCheckWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SecurityCheckWindow(int userId, const QString &role, QWidget *parent = nullptr);

private slots:
    void searchItem();

private:
    int m_userId;
    QString m_role;

    QLineEdit    *serialEdit;
    QPushButton  *searchBtn;
    QTableWidget *table;
    QLabel       *resultLabel;
    QLabel       *bstLabel;
    QPushButton  *allowExitBtn;
    QPushButton  *denyExitBtn;
    QPushButton  *markReturnedBtn;
    QPushButton  *clearBtn;
    QPushButton  *reloadBstBtn;

    // "Currently Outside Campus" panel
    QTableWidget *outsideTable;
    QLabel       *outsideLabel;

    DSAEngine    m_dsa;
    int          m_bstSearchSteps = 0;

    void setupUI();
    void loadSerialsBST();
    void loadOutsidePanel();
};

#endif
