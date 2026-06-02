#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTabWidget>
#include <QMap>
#include "dsaengine.h"

class AssetWindow : public QWidget {
    Q_OBJECT

public:
    explicit AssetWindow(QWidget *parent = nullptr);

private slots:
    void loadAssets();
    void searchAssets();
    void addAsset();
    void editAsset();
    void deleteAsset();
    void deleteBySelected();
    void deleteFirst();
    void deleteLast();
    void deleteByKey();
    void undoAction();
    void redoAction();
    void sortAssets(const QString &criteria);

private:
    std::vector<AssetRecord> fetchAllAssets();
    void refreshTable(const std::vector<AssetRecord> &records);
    void refreshAllTabs(const std::vector<AssetRecord> &records);
    QTableWidget *currentTable() const;
    QString categoryFolder(const QString &category) const;
    void setupAssetTable(QTableWidget *t);

    QTabWidget  *categoryTabs;
    QMap<QString, QTableWidget*> tabTables;

    QLineEdit    *searchEdit;
    QComboBox    *filterCombo;
    QComboBox    *sortCombo;
    QPushButton  *addBtn, *editBtn, *deleteBtn;
    QPushButton  *undoBtn, *redoBtn, *refreshBtn;
    QLabel       *stackLabel;

    std::vector<AssetRecord> m_currentRecords;
    DSAEngine m_dsa;
};
