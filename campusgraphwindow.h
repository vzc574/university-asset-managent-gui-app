#pragma once
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include "dsaengine.h"

class CampusGraphWindow : public QWidget {
    Q_OBJECT
public:
    explicit CampusGraphWindow(QWidget *parent = nullptr);

private slots:
    void runBFS();
    void runDFS();

private:
    void setupUI();

    QComboBox   *startCombo, *endCombo, *dfsStartCombo;
    QPushButton *bfsBtn, *dfsBtn;
    QLabel      *algoLabel;
    QTextEdit   *adjText;
    QTextEdit   *resultText;

    DSAEngine m_dsa;
};
