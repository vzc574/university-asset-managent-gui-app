#include "assetwindow.h"
#include <QRegularExpression>
#include <QApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

// ── Helper for undo/redo label ───────────────────────────
static void updateStackLabel(QLabel *lbl, DSAEngine &dsa,
                             QPushButton *undoBtn, QPushButton *redoBtn)
{
    lbl->setText(QString("Undo: %1  |  Redo: %2")
                     .arg(dsa.undoSize()).arg(dsa.redoSize()));
    undoBtn->setEnabled(dsa.canUndo());
    redoBtn->setEnabled(dsa.canRedo());
}

AssetWindow::AssetWindow(QWidget *parent) : QWidget(parent)
{
    setStyleSheet(R"(
        QTabWidget::pane {
            border: 1px solid #ddd;
            background: white;
            border-radius: 8px;
        }
        QTabBar::tab {
            background: #eef1f5;
            color: #1a1a2e;
            padding: 9px 18px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            margin-right: 3px;
            font-weight: bold;
        }
        QTabBar::tab:selected {
            background: #0f6e56;
            color: white;
        }
        QTableWidget {
            background: white;
            border: none;
            gridline-color: #f0f0f0;
            font-size: 13px;
            color: #1a1a2e;
        }
        QTableWidget::item { padding: 8px; color: #1a1a2e; background: white; }
        QTableWidget::item:selected { background-color: #e8f5f1; color: #1a1a2e; }
        QHeaderView::section {
            background-color: #1a1a2e; color: white;
            padding: 10px; font-size: 13px; font-weight: bold; border: none;
        }
        QLineEdit {
            border: 1.5px solid #ddd; border-radius: 8px;
            padding: 8px 14px; font-size: 13px;
            background: white; color: #1a1a2e;
        }
        QLineEdit:focus { border-color: #0f6e56; }
        QComboBox {
            border: 1.5px solid #ddd; border-radius: 8px;
            padding: 8px 14px; font-size: 13px;
            background: white; color: #1a1a2e; min-width: 130px;
        }
        QPushButton#addBtn    { background:#0f6e56; color:white; border:none; border-radius:8px; padding:8px 16px; font-size:13px; font-weight:bold; }
        QPushButton#addBtn:hover { background:#1D9E75; }
        QPushButton#editBtn   { background:#2980b9; color:white; border:none; border-radius:8px; padding:8px 16px; font-size:13px; font-weight:bold; }
        QPushButton#editBtn:hover { background:#3498db; }
        QPushButton#deleteBtn { background:#c0392b; color:white; border:none; border-radius:8px; padding:8px 16px; font-size:13px; font-weight:bold; }
        QPushButton#deleteBtn:hover { background:#e74c3c; }
        QPushButton#undoBtn   { background:#8e44ad; color:white; border:none; border-radius:8px; padding:8px 16px; font-size:13px; font-weight:bold; }
        QPushButton#undoBtn:hover { background:#9b59b6; }
        QPushButton#redoBtn   { background:#d35400; color:white; border:none; border-radius:8px; padding:8px 16px; font-size:13px; font-weight:bold; }
        QPushButton#redoBtn:hover { background:#e67e22; }
        QPushButton#refreshBtn { background:#f0f2f5; color:#1a1a2e; border:1px solid #ddd; border-radius:8px; padding:8px 16px; font-size:13px; }
        QPushButton#refreshBtn:hover { background:#e0e0e0; }
        QPushButton#undoBtn:disabled, QPushButton#redoBtn:disabled { background:#ccc; color:white; }
        QInputDialog { background-color: #f0f2f5; color: #1a1a2e; }
        QInputDialog QLabel { color: #1a1a2e; background: transparent; }
        QInputDialog QLineEdit, QInputDialog QComboBox, QInputDialog QDoubleSpinBox {
            color: #1a1a2e; background-color: white;
            border: 1px solid #0f6e56; border-radius: 6px; padding: 6px;
        }
        QInputDialog QPushButton {
            color: white; background-color: #0f6e56;
            border-radius: 6px; padding: 6px 14px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    QHBoxLayout *titleRow = new QHBoxLayout();
    QLabel *title = new QLabel("Asset Inventory by Folder");
    title->setStyleSheet("font-size:20px;font-weight:bold;color:#1a1a2e;");

    stackLabel = new QLabel("Undo: 0  |  Redo: 0");
    stackLabel->setStyleSheet("font-size:12px;color:#888;padding:4px 10px;"
                              "background:#f0f2f5;border-radius:6px;border:1px solid #ddd;");
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(stackLabel);

    QHBoxLayout *toolbar1 = new QHBoxLayout();
    toolbar1->setSpacing(8);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by name or serial...");
    searchEdit->setFixedHeight(36);

    filterCombo = new QComboBox();
    filterCombo->setFixedHeight(36);
    filterCombo->addItems({
        "All Folders",
        "IT_EQUIPMENT",
        "LAB_EQUIPMENT",
        "FURNITURE",
        "VEHICLES",
        "BUILDING_INFRASTRUCTURE",
        "SECURITY_SAFETY",
        "OFFICE_ADMIN",
        "SPORTS_OTHER",
        "OTHER"
    });

    sortCombo = new QComboBox();
    sortCombo->setFixedHeight(36);
    sortCombo->addItems({"Sort by...",
                         "ID (Bubble Sort)","Value (Bubble Sort)",
                         "Name (Selection Sort)","Location (Selection Sort)",
                         "Value (Merge Sort - D&C)","Name (Merge Sort - D&C)"});

    toolbar1->addWidget(searchEdit, 1);
    toolbar1->addWidget(filterCombo);
    toolbar1->addWidget(sortCombo);

    QHBoxLayout *toolbar2 = new QHBoxLayout();
    toolbar2->setSpacing(8);

    refreshBtn = new QPushButton("Refresh");
    refreshBtn->setObjectName("refreshBtn");
    refreshBtn->setFixedHeight(36);

    addBtn = new QPushButton("+ Add");
    addBtn->setObjectName("addBtn");
    addBtn->setFixedHeight(36);

    editBtn = new QPushButton("Edit");
    editBtn->setObjectName("editBtn");
    editBtn->setFixedHeight(36);

    deleteBtn = new QPushButton("Delete");
    deleteBtn->setObjectName("deleteBtn");
    deleteBtn->setFixedHeight(36);

    undoBtn = new QPushButton("Undo");
    undoBtn->setObjectName("undoBtn");
    undoBtn->setFixedHeight(36);
    undoBtn->setEnabled(false);

    redoBtn = new QPushButton("Redo");
    redoBtn->setObjectName("redoBtn");
    redoBtn->setFixedHeight(36);
    redoBtn->setEnabled(false);

    toolbar2->addWidget(refreshBtn);
    toolbar2->addStretch();
    toolbar2->addWidget(undoBtn);
    toolbar2->addWidget(redoBtn);
    toolbar2->addWidget(addBtn);
    toolbar2->addWidget(editBtn);
    toolbar2->addWidget(deleteBtn);

    categoryTabs = new QTabWidget(this);
    QStringList folders = {
        "IT_EQUIPMENT",
        "LAB_EQUIPMENT",
        "FURNITURE",
        "VEHICLES",
        "BUILDING_INFRASTRUCTURE",
        "SECURITY_SAFETY",
        "OFFICE_ADMIN",
        "SPORTS_OTHER",
        "OTHER"
    };
    for (const QString &folder : folders) {
        QTableWidget *t = new QTableWidget(categoryTabs);
        setupAssetTable(t);
        tabTables.insert(folder, t);
        categoryTabs->addTab(t, folder);
    }

    layout->addLayout(titleRow);
    layout->addLayout(toolbar1);
    layout->addLayout(toolbar2);
    layout->addWidget(categoryTabs);

    connect(refreshBtn, &QPushButton::clicked, this, &AssetWindow::loadAssets);
    connect(addBtn,     &QPushButton::clicked, this, &AssetWindow::addAsset);
    connect(editBtn,    &QPushButton::clicked, this, &AssetWindow::editAsset);
    connect(deleteBtn,  &QPushButton::clicked, this, &AssetWindow::deleteAsset);
    connect(undoBtn,    &QPushButton::clicked, this, &AssetWindow::undoAction);
    connect(redoBtn,    &QPushButton::clicked, this, &AssetWindow::redoAction);
    connect(searchEdit, &QLineEdit::textChanged, this, &AssetWindow::searchAssets);
    connect(filterCombo,&QComboBox::currentTextChanged, this, &AssetWindow::searchAssets);
    connect(sortCombo,  &QComboBox::currentTextChanged, this, &AssetWindow::sortAssets);

    loadAssets();
}

void AssetWindow::setupAssetTable(QTableWidget *t)
{
    t->setColumnCount(7);
    t->setHorizontalHeaderLabels({
        "ID","Name","Folder","Condition",
        "Serial Number","Value (ETB)","Location"
    });
    t->horizontalHeader()->setStretchLastSection(true);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->setColumnWidth(0, 50);
    t->setColumnWidth(1, 170);
    t->setColumnWidth(2, 100);
    t->setColumnWidth(3, 130);
    t->setColumnWidth(4, 130);
    t->setColumnWidth(5, 100);
}

QTableWidget *AssetWindow::currentTable() const
{
    return qobject_cast<QTableWidget*>(categoryTabs->currentWidget());
}

QString AssetWindow::categoryFolder(const QString &category) const
{
    QString c = category.trimmed().toUpper();

    if (c == "IT_EQUIPMENT" || c == "ELECTRONICS" || c == "COMPUTER" || c == "PC")
        return "IT_EQUIPMENT";

    if (c == "LAB_EQUIPMENT" || c == "LAB" || c == "SCIENCE")
        return "LAB_EQUIPMENT";

    if (c == "FURNITURE" || c == "CHAIR" || c == "DESK" || c == "TABLE")
        return "FURNITURE";

    if (c == "VEHICLES" || c == "VEHICLE" || c == "CAR" || c == "BUS")
        return "VEHICLES";

    if (c == "BUILDING_INFRASTRUCTURE" || c == "INFRASTRUCTURE")
        return "BUILDING_INFRASTRUCTURE";

    if (c == "SECURITY_SAFETY" || c == "SECURITY" || c == "SAFETY")
        return "SECURITY_SAFETY";

    if (c == "OFFICE_ADMIN" || c == "OFFICE")
        return "OFFICE_ADMIN";

    if (c == "SPORTS_OTHER" || c == "SPORTS")
        return "SPORTS_OTHER";

    return "OTHER";
}

std::vector<AssetRecord> AssetWindow::fetchAllAssets()
{
    std::vector<AssetRecord> records;

    QSqlQuery q(QSqlDatabase::database());
    bool ok = q.exec(R"(
        SELECT a.asset_id,
               a.asset_name,
               a.category,
               NVL(a.condition,'GOOD'),
               NVL(a.serial_number,''),
               NVL(a.purchase_value,0),
               NVL(l.building_name || ' - ' || NVL(l.room_number,'N/A'), 'N/A')
        FROM ASSETS a
        LEFT JOIN LOCATIONS l ON a.location_id = l.location_id
        ORDER BY a.asset_id
    )");

    if (!ok) {
        QMessageBox::critical(this, "DB Error", q.lastError().text());
        return records;
    }

    while (q.next()) {
        AssetRecord r;
        r.id           = q.value(0).toInt();
        r.name         = q.value(1).toString().toStdString();
        r.category     = categoryFolder(q.value(2).toString()).toStdString();
        r.condition    = q.value(3).toString().toStdString();
        r.serialNumber = q.value(4).toString().toStdString();
        r.value        = q.value(5).toDouble();
        r.location     = q.value(6).toString().toStdString();
        records.push_back(r);
    }

    return records;
}

void AssetWindow::refreshAllTabs(const std::vector<AssetRecord> &records)
{
    for (QTableWidget *t : tabTables)
        t->setRowCount(0);

    QString keyword = searchEdit->text().trimmed().toLower();
    QString filter  = filterCombo->currentText();

    for (const auto &r : records) {
        QString folder = QString::fromStdString(r.category);
        if (!tabTables.contains(folder))
            folder = "OTHER";

        if (filter != "All Folders" && folder != filter)
            continue;

        QString name   = QString::fromStdString(r.name);
        QString serial = QString::fromStdString(r.serialNumber);

        if (!keyword.isEmpty()
            && !name.toLower().contains(keyword)
            && !serial.toLower().contains(keyword)) {
            continue;
        }

        QTableWidget *t = tabTables.value(folder);
        int row = t->rowCount();
        t->insertRow(row);

        QStringList vals = {
            QString::number(r.id),
            name,
            folder,
            QString::fromStdString(r.condition),
            serial,
            QString::number(r.value, 'f', 2),
            QString::fromStdString(r.location)
        };

        for (int col = 0; col < 7; col++) {
            QTableWidgetItem *item = new QTableWidgetItem(vals[col]);
            item->setTextAlignment(Qt::AlignCenter);

            if (col == 3) {
                QString cond = vals[col];
                if      (cond == "BROKEN")            item->setForeground(QColor("#c0392b"));
                else if (cond == "GOOD")              item->setForeground(QColor("#0f6e56"));
                else if (cond == "UNDER_MAINTENANCE") item->setForeground(QColor("#e67e22"));
            }

            t->setItem(row, col, item);
        }
    }

    // If filter is a folder, jump directly to that folder tab.
    if (filter != "All Folders" && tabTables.contains(filter)) {
        categoryTabs->setCurrentWidget(tabTables.value(filter));
    }
}

void AssetWindow::refreshTable(const std::vector<AssetRecord> &records)
{
    refreshAllTabs(records);
}

void AssetWindow::loadAssets()
{
    m_currentRecords = fetchAllAssets();
    refreshAllTabs(m_currentRecords);
    sortCombo->setCurrentIndex(0);
}

void AssetWindow::searchAssets()
{
    refreshAllTabs(m_currentRecords);
}

void AssetWindow::sortAssets(const QString &criteria)
{
    if (criteria == "Sort by...") return;

    auto records = fetchAllAssets();

    if (criteria == "ID (Bubble Sort)")
        m_dsa.bubbleSortById(records);
    else if (criteria == "Value (Bubble Sort)")
        m_dsa.bubbleSortByValue(records);
    else if (criteria == "Name (Selection Sort)")
        m_dsa.selectionSortByName(records);
    else if (criteria == "Location (Selection Sort)")
        m_dsa.selectionSortByLocation(records);
    else if (criteria == "Value (Merge Sort - D&C)")
        m_dsa.mergeSortByValue(records);
    else if (criteria == "Name (Merge Sort - D&C)")
        m_dsa.mergeSortByName(records);

    m_currentRecords = records;
    refreshAllTabs(m_currentRecords);
}
void AssetWindow::addAsset()
{
    bool ok;

    QString name = QInputDialog::getText(this, "Add Asset", "Asset name:",
                                         QLineEdit::Normal, "", &ok);
    if (!ok) return;

    name = name.trimmed();

    QRegularExpression nameRegex("^[A-Za-z0-9 ._-]{2,50}$");
    if (!nameRegex.match(name).hasMatch()) {
        QMessageBox::warning(this, "Validation Error",
                             "Asset name must be 2-50 characters.\nUse letters, numbers, space, dot, underscore or dash.");
        return;
    }

    QString serial = QInputDialog::getText(this, "Add Asset", "Serial number:",
                                           QLineEdit::Normal, "", &ok);
    if (!ok) return;

    serial = serial.trimmed();

    QRegularExpression serialRegex("^[A-Za-z0-9-]{4,40}$");
    if (!serial.isEmpty() && !serialRegex.match(serial).hasMatch()) {
        QMessageBox::warning(this, "Validation Error",
                             "Serial number must be 4-40 characters.\nUse only letters, numbers or dash.");
        return;
    }

    QStringList cats = {
        "IT_EQUIPMENT",
        "LAB_EQUIPMENT",
        "FURNITURE",
        "VEHICLES",
        "BUILDING_INFRASTRUCTURE",
        "SECURITY_SAFETY",
        "OFFICE_ADMIN",
        "SPORTS_OTHER",
        "OTHER"
    };

    QString cat = QInputDialog::getItem(this, "Add Asset", "Folder / Category:",
                                        cats, 0, false, &ok);
    if (!ok) return;

    double value = QInputDialog::getDouble(this, "Add Asset", "Purchase value (ETB):",
                                           0, 0, 9999999, 2, &ok);
    if (!ok) return;

    QSqlQuery q(QSqlDatabase::database());
    q.prepare(R"(
        INSERT INTO ASSETS (asset_id, asset_name, category, serial_number, purchase_value, condition, created_by)
        VALUES (SEQ_ASSETS.NEXTVAL, :name, :cat, :serial, :val, 'GOOD', 1)
    )");

    q.bindValue(":name",   name);
    q.bindValue(":cat",    cat);
    q.bindValue(":serial", serial);
    q.bindValue(":val",    value);

    if (q.exec()) {
        QSqlQuery q2(QSqlDatabase::database());
        q2.exec("COMMIT");

        QSqlQuery idQ(QSqlDatabase::database());
        idQ.exec("SELECT MAX(asset_id) FROM ASSETS");
        int newId = idQ.next() ? idQ.value(0).toInt() : 0;

        AssetSnapshot snap;
        snap.assetId       = newId;
        snap.assetName     = name.toStdString();
        snap.category      = cat.toStdString();
        snap.condition     = "GOOD";
        snap.serialNumber  = serial.toStdString();
        snap.purchaseValue = value;
        snap.action        = ActionType::ADD_ASSET;

        m_dsa.pushUndo(snap);
        m_dsa.clearRedo();
        m_dsa.enqueueAction("ADD: " + snap.assetName);

        updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
        QMessageBox::information(this, "Success", "Asset added.");
        loadAssets();
    } else {
        QMessageBox::critical(this, "Error", q.lastError().text());
    }
}
void AssetWindow::editAsset()
{
    QTableWidget *t = currentTable();
    if (!t) return;

    int row = t->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Warning", "Please select an asset to edit.");
        return;
    }

    if (!t->item(row,0) || !t->item(row,1) || !t->item(row,3) || !t->item(row,5)) {
        QMessageBox::warning(this, "Error", "Invalid selected row.");
        return;
    }

    int assetId      = t->item(row,0)->text().toInt();
    QString oldName  = t->item(row,1)->text();
    QString oldCat   = t->item(row,2)->text();
    QString oldCond  = t->item(row,3)->text();
    double oldValue  = t->item(row,5)->text().toDouble();

    bool ok;
    QString newName = QInputDialog::getText(this, "Edit Asset", "Asset name:",
                                            QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.trimmed().isEmpty()) return;

    QStringList cats = {
        "IT_EQUIPMENT",
        "LAB_EQUIPMENT",
        "FURNITURE",
        "VEHICLES",
        "BUILDING_INFRASTRUCTURE",
        "SECURITY_SAFETY",
        "OFFICE_ADMIN",
        "SPORTS_OTHER",
        "OTHER"
    };
    QString newCat = QInputDialog::getItem(this, "Edit Asset", "Folder / Category:",
                                           cats, cats.indexOf(oldCat), false, &ok);
    if (!ok) return;

    QStringList conds = {"GOOD","BROKEN","UNDER_MAINTENANCE","DECOMMISSIONED"};
    QString newCond = QInputDialog::getItem(this, "Edit Asset", "Condition:",
                                            conds, qMax(0, conds.indexOf(oldCond)), false, &ok);
    if (!ok) return;

    double newValue = QInputDialog::getDouble(this, "Edit Asset", "Purchase value (ETB):",
                                              oldValue, 0, 9999999, 2, &ok);
    if (!ok) return;

    AssetSnapshot snap;
    snap.assetId       = assetId;
    snap.assetName     = oldName.toStdString();
    snap.category      = oldCat.toStdString();
    snap.condition     = oldCond.toStdString();
    snap.purchaseValue = oldValue;
    snap.action        = ActionType::EDIT_ASSET;
    m_dsa.pushUndo(snap);
    m_dsa.clearRedo();

    QSqlQuery q(QSqlDatabase::database());
    q.prepare(R"(
        UPDATE ASSETS
        SET asset_name=:name,
            category=:cat,
            condition=:cond,
            purchase_value=:val,
            updated_at=SYSDATE
        WHERE asset_id=:id
    )");
    q.bindValue(":name", newName.trimmed());
    q.bindValue(":cat",  newCat);
    q.bindValue(":cond", newCond);
    q.bindValue(":val",  newValue);
    q.bindValue(":id",   assetId);

    if (q.exec()) {
        QSqlQuery q2(QSqlDatabase::database());
        q2.exec("COMMIT");

        m_dsa.enqueueAction("EDIT: " + oldName.toStdString());
        updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
        loadAssets();
    } else {
        QMessageBox::critical(this, "Error", q.lastError().text());
    }
}

void AssetWindow::deleteAsset()
{
    QStringList options = {
        "Delete Selected Row",
        "Delete First (Lowest ID)",
        "Delete Last (Highest ID)",
        "Delete by Name (Key Search)"
    };

    bool ok;
    QString choice = QInputDialog::getItem(this, "Delete Asset",
                                           "Choose delete method:", options, 0, false, &ok);
    if (!ok) return;

    if      (choice == options[0]) deleteBySelected();
    else if (choice == options[1]) deleteFirst();
    else if (choice == options[2]) deleteLast();
    else if (choice == options[3]) deleteByKey();
}

void AssetWindow::deleteBySelected()
{
    QTableWidget *t = currentTable();
    if (!t) return;

    int row = t->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Warning", "Please select an asset to delete.");
        return;
    }

    if (!t->item(row,0) || !t->item(row,1) || !t->item(row,2) || !t->item(row,5)) {
        QMessageBox::warning(this, "Error", "Invalid selected row.");
        return;
    }

    int id = t->item(row,0)->text().toInt();
    QString name = t->item(row,1)->text();

    QSqlQuery check(QSqlDatabase::database());
    check.prepare("SELECT COUNT(*) FROM MAINTENANCE_LOG WHERE ASSET_ID = :id");
    check.bindValue(":id", id);

    if (check.exec() && check.next() && check.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Cannot Delete",
                             "This asset has maintenance records. Delete or resolve those records first.");
        return;
    }

    auto reply = QMessageBox::question(this, "Confirm Delete",
                                       "Delete asset: " + name + "?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    AssetSnapshot snap;
    snap.assetId       = id;
    snap.assetName     = name.toStdString();
    snap.category      = t->item(row,2)->text().toStdString();
    snap.condition     = t->item(row,3) ? t->item(row,3)->text().toStdString() : "GOOD";
    snap.serialNumber  = t->item(row,4) ? t->item(row,4)->text().toStdString() : "";
    snap.purchaseValue = t->item(row,5)->text().toDouble();
    snap.action        = ActionType::DELETE_ASSET;

    QSqlQuery q(QSqlDatabase::database());
    q.prepare("DELETE FROM ASSETS WHERE ASSET_ID = :id");
    q.bindValue(":id", id);

    if (!q.exec()) {
        QMessageBox::critical(this, "Delete Error", q.lastError().text());
        return;
    }

    QSqlQuery commit(QSqlDatabase::database());
    commit.exec("COMMIT");

    m_dsa.pushUndo(snap);
    m_dsa.clearRedo();
    updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);

    QMessageBox::information(this, "Deleted", "Asset deleted. You can Undo it.");
    loadAssets();
}
void AssetWindow::deleteFirst()

{
    QSqlQuery q(QSqlDatabase::database());
    q.exec(R"(
        SELECT asset_id, asset_name, category, NVL(condition,'GOOD'), NVL(serial_number,''), NVL(purchase_value,0)
        FROM ASSETS
        WHERE asset_id = (SELECT MIN(asset_id) FROM ASSETS)
    )");

    if (!q.next()) {
        QMessageBox::information(this, "Info", "No assets found.");
        return;
    }

    int id = q.value(0).toInt();
    QString name = q.value(1).toString();

    auto reply = QMessageBox::question(this, "Delete First",
                                       "Delete FIRST asset: " + name + "?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    AssetSnapshot snap;
    snap.assetId       = id;
    snap.assetName     = name.toStdString();
    snap.category      = categoryFolder(q.value(2).toString()).toStdString();
    snap.condition     = q.value(3).toString().toStdString();
    snap.serialNumber  = q.value(4).toString().toStdString();
    snap.purchaseValue = q.value(5).toDouble();
    snap.action        = ActionType::DELETE_ASSET;

    QSqlQuery del(QSqlDatabase::database());
    del.prepare("DELETE FROM ASSETS WHERE asset_id=:id");
    del.bindValue(":id", id);

    if (del.exec()) {
        QSqlQuery c(QSqlDatabase::database());
        c.exec("COMMIT");
        m_dsa.pushUndo(snap);
        m_dsa.clearRedo();
        updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
        loadAssets();
    } else {
        QMessageBox::critical(this, "Error", del.lastError().text());
    }
}

void AssetWindow::deleteLast()
{
    QSqlQuery q(QSqlDatabase::database());
    q.exec(R"(
        SELECT asset_id, asset_name, category, NVL(condition,'GOOD'), NVL(serial_number,''), NVL(purchase_value,0)
        FROM ASSETS
        WHERE asset_id = (SELECT MAX(asset_id) FROM ASSETS)
    )");

    if (!q.next()) {
        QMessageBox::information(this, "Info", "No assets found.");
        return;
    }

    int id = q.value(0).toInt();
    QString name = q.value(1).toString();

    auto reply = QMessageBox::question(this, "Delete Last",
                                       "Delete LAST asset: " + name + "?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    AssetSnapshot snap;
    snap.assetId       = id;
    snap.assetName     = name.toStdString();
    snap.category      = categoryFolder(q.value(2).toString()).toStdString();
    snap.condition     = q.value(3).toString().toStdString();
    snap.serialNumber  = q.value(4).toString().toStdString();
    snap.purchaseValue = q.value(5).toDouble();
    snap.action        = ActionType::DELETE_ASSET;

    QSqlQuery del(QSqlDatabase::database());
    del.prepare("DELETE FROM ASSETS WHERE asset_id=:id");
    del.bindValue(":id", id);

    if (del.exec()) {
        QSqlQuery c(QSqlDatabase::database());
        c.exec("COMMIT");
        m_dsa.pushUndo(snap);
        m_dsa.clearRedo();
        updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
        loadAssets();
    } else {
        QMessageBox::critical(this, "Error", del.lastError().text());
    }
}

void AssetWindow::deleteByKey()
{
    bool ok;
    QString key = QInputDialog::getText(this, "Delete by Name",
                                        "Enter asset name to search:",
                                        QLineEdit::Normal, "", &ok);
    if (!ok || key.trimmed().isEmpty()) return;

    QSqlQuery q(QSqlDatabase::database());
    q.prepare(R"(
        SELECT asset_id, asset_name, category, NVL(condition,'GOOD'), NVL(serial_number,''), NVL(purchase_value,0)
        FROM ASSETS
        WHERE UPPER(asset_name) LIKE UPPER(:k)
    )");
    q.bindValue(":k", "%" + key.trimmed() + "%");

    if (!q.exec() || !q.next()) {
        QMessageBox::information(this, "Not Found", "No asset matching: " + key);
        return;
    }

    int id = q.value(0).toInt();
    QString name = q.value(1).toString();

    auto reply = QMessageBox::question(this, "Delete by Key",
                                       "Delete asset: " + name + "?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    AssetSnapshot snap;
    snap.assetId       = id;
    snap.assetName     = name.toStdString();
    snap.category      = categoryFolder(q.value(2).toString()).toStdString();
    snap.condition     = q.value(3).toString().toStdString();
    snap.serialNumber  = q.value(4).toString().toStdString();
    snap.purchaseValue = q.value(5).toDouble();
    snap.action        = ActionType::DELETE_ASSET;

    QSqlQuery del(QSqlDatabase::database());
    del.prepare("DELETE FROM ASSETS WHERE asset_id=:id");
    del.bindValue(":id", id);

    if (del.exec()) {
        QSqlQuery c(QSqlDatabase::database());
        c.exec("COMMIT");
        m_dsa.pushUndo(snap);
        m_dsa.clearRedo();
        updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
        loadAssets();
    } else {
        QMessageBox::critical(this, "Error", del.lastError().text());
    }
}

void AssetWindow::undoAction()
{
    if (!m_dsa.canUndo()) return;

    AssetSnapshot snap = m_dsa.popUndo();

    if (snap.action == ActionType::ADD_ASSET) {
        QSqlQuery q(QSqlDatabase::database());
        q.prepare("DELETE FROM ASSETS WHERE asset_id=:id");
        q.bindValue(":id", snap.assetId);

        if (q.exec()) {
            QSqlQuery c(QSqlDatabase::database());
            c.exec("COMMIT");
            m_dsa.pushRedo(snap);
        } else {
            QMessageBox::critical(this, "Undo Error", q.lastError().text());
            m_dsa.pushUndo(snap);
        }

    } else if (snap.action == ActionType::DELETE_ASSET) {
        QSqlQuery q(QSqlDatabase::database());
        q.prepare(R"(
            INSERT INTO ASSETS
                (asset_id, asset_name, category, condition, serial_number, purchase_value, created_by)
            VALUES
                (:id, :name, :cat, :cond, :serial, :val, 1)
        )");
        q.bindValue(":id",     snap.assetId);
        q.bindValue(":name",   QString::fromStdString(snap.assetName));
        q.bindValue(":cat",    QString::fromStdString(snap.category));
        q.bindValue(":cond",   QString::fromStdString(snap.condition.empty() ? "GOOD" : snap.condition));
        q.bindValue(":serial", QString::fromStdString(snap.serialNumber));
        q.bindValue(":val",    snap.purchaseValue);

        if (q.exec()) {
            QSqlQuery c(QSqlDatabase::database());
            c.exec("COMMIT");
            m_dsa.pushRedo(snap);
            QMessageBox::information(this, "Undo", "Asset restored.");
        } else {
            QMessageBox::critical(this, "Undo Error", q.lastError().text());
            m_dsa.pushUndo(snap);
        }

    } else if (snap.action == ActionType::EDIT_ASSET) {
        QSqlQuery q(QSqlDatabase::database());
        q.prepare(R"(
            UPDATE ASSETS
            SET asset_name=:name,
                category=:cat,
                condition=:cond,
                purchase_value=:val,
                updated_at=SYSDATE
            WHERE asset_id=:id
        )");
        q.bindValue(":name", QString::fromStdString(snap.assetName));
       q.bindValue(":cat", categoryFolder(QString::fromStdString(snap.category)));
        q.bindValue(":cond", QString::fromStdString(snap.condition));
        q.bindValue(":val",  snap.purchaseValue);
        q.bindValue(":id",   snap.assetId);

        if (q.exec()) {
            QSqlQuery c(QSqlDatabase::database());
            c.exec("COMMIT");
            m_dsa.pushRedo(snap);
        } else {
            QMessageBox::critical(this, "Undo Error", q.lastError().text());
            m_dsa.pushUndo(snap);
        }
    }

    updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
    loadAssets();
}

void AssetWindow::redoAction()
{
    if (!m_dsa.canRedo()) return;

    AssetSnapshot snap = m_dsa.popRedo();

    if (snap.action == ActionType::ADD_ASSET) {
        QSqlQuery q(QSqlDatabase::database());
        q.prepare(R"(
            INSERT INTO ASSETS
                (asset_id, asset_name, category, condition, serial_number, purchase_value, created_by)
            VALUES
                (:id, :name, :cat, :cond, :serial, :val, 1)
        )");
        q.bindValue(":id",     snap.assetId);
        q.bindValue(":name",   QString::fromStdString(snap.assetName));
        q.bindValue(":cat",    QString::fromStdString(snap.category));
        q.bindValue(":cond",   QString::fromStdString(snap.condition.empty() ? "GOOD" : snap.condition));
        q.bindValue(":serial", QString::fromStdString(snap.serialNumber));
        q.bindValue(":val",    snap.purchaseValue);

        if (q.exec()) {
            QSqlQuery c(QSqlDatabase::database());
            c.exec("COMMIT");
            m_dsa.pushUndo(snap);
        } else {
            QMessageBox::critical(this, "Redo Error", q.lastError().text());
            m_dsa.pushRedo(snap);
        }

    } else if (snap.action == ActionType::DELETE_ASSET) {
        QSqlQuery q(QSqlDatabase::database());
        q.prepare("DELETE FROM ASSETS WHERE asset_id=:id");
        q.bindValue(":id", snap.assetId);

        if (q.exec()) {
            QSqlQuery c(QSqlDatabase::database());
            c.exec("COMMIT");
            m_dsa.pushUndo(snap);
        } else {
            QMessageBox::critical(this, "Redo Error", q.lastError().text());
            m_dsa.pushRedo(snap);
        }

    } else if (snap.action == ActionType::EDIT_ASSET) {
        // Current DSA snapshot stores only old values, so redo edit cannot safely reapply new values.
        QMessageBox::information(this, "Redo Edit",
                                 "Redo for edit needs storing both old and new values. Delete/Add redo works.");
        m_dsa.pushRedo(snap);
    }

    updateStackLabel(stackLabel, m_dsa, undoBtn, redoBtn);
    loadAssets();
}
