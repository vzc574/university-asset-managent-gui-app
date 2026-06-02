#include "studentitemswindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFont>
#include <QRegularExpressionValidator>
StudentItemsWindow::StudentItemsWindow(int userId, const QString &role, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_role(role)
{
    setupUI();
    loadData();
}

void StudentItemsWindow::setupUI()
{
    setStyleSheet(R"(
        QWidget          { background:#1e1e2e; color:#cdd6f4; font-family:'Segoe UI'; font-size:13px; }
        QLineEdit        { background:#313244; color:#cdd6f4; border:1px solid #45475a;
                           border-radius:6px; padding:7px 12px; }
        QLineEdit:focus  { border:1px solid #89b4fa; }
        QScrollBar:vertical   { background:#181825; width:8px; border-radius:4px; }
        QScrollBar::handle:vertical { background:#45475a; border-radius:4px; }
    )");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 16);
    mainLayout->setSpacing(14);

    // ── Title row ─────────────────────────────────────────
    auto *titleRow = new QHBoxLayout();
    auto *title = new QLabel("Student Items");
    title->setStyleSheet("font-size:22px; font-weight:bold; color:#cba6f7;");

    statsLabel = new QLabel("0 items");
    statsLabel->setStyleSheet(
        "background:#313244; color:#89b4fa; border-radius:10px;"
        "padding:4px 14px; font-size:11px; font-weight:bold;");
    statsLabel->setAlignment(Qt::AlignCenter);

    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(statsLabel);
    mainLayout->addLayout(titleRow);

    // ── Toolbar ───────────────────────────────────────────
    auto *toolbar = new QHBoxLayout();
    toolbar->setSpacing(10);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("  Search by name, owner, serial, brand...");
    searchEdit->setMinimumWidth(280);
    searchEdit->setFixedHeight(36);

    registerBtn = new QPushButton("＋  Register Item");
    deleteBtn   = new QPushButton("✕  Remove");
    refreshBtn  = new QPushButton("↻  Refresh");

    styleButton(registerBtn, "#a6e3a1");
    styleButton(deleteBtn,   "#f38ba8");
    styleButton(refreshBtn,  "#585b70");

    if (m_role == "STUDENT") deleteBtn->setVisible(false);

    toolbar->addWidget(searchEdit, 1);
    toolbar->addStretch();
    toolbar->addWidget(registerBtn);
    toolbar->addWidget(deleteBtn);
    toolbar->addWidget(refreshBtn);
    mainLayout->addLayout(toolbar);

    // ── Table ─────────────────────────────────────────────
    table = new QTableWidget();
    table->setColumnCount(9);
    table->setHorizontalHeaderLabels({
        "ID", "Owner", "Item Name", "Category",
        "Brand", "Model", "Serial No.", "Status", "Location"
    });

    // Fixed widths for known columns, stretch for Location
    auto *hdr = table->horizontalHeader();
    hdr->setSectionResizeMode(QHeaderView::Interactive);
    hdr->setSectionResizeMode(8, QHeaderView::Stretch);   // Location fills rest
    table->setColumnWidth(0, 50);   // ID
    table->setColumnWidth(1, 130);  // Owner
    table->setColumnWidth(2, 140);  // Item Name
    table->setColumnWidth(3, 100);  // Category
    table->setColumnWidth(4, 90);   // Brand
    table->setColumnWidth(5, 90);   // Model
    table->setColumnWidth(6, 120);  // Serial No.
    table->setColumnWidth(7, 120);  // Status

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(false);
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setWordWrap(false);
    table->verticalHeader()->setDefaultSectionSize(38);

    table->setStyleSheet(R"(
        QTableWidget {
            background:#181825; color:#cdd6f4; border:1px solid #313244;
            border-radius:8px; gridline-color:#2a2a3e;
            outline:0;
        }
        QTableWidget::item {
            padding:0px 8px; border-bottom:1px solid #2a2a3e;
        }
        QTableWidget::item:selected {
            background:#313244; color:#cdd6f4;
        }
        QTableWidget::item:hover {
            background:#2a2a40;
        }
        QHeaderView::section {
            background:#252535; color:#cba6f7; padding:10px 8px;
            border:none; border-bottom:2px solid #45475a;
            font-weight:bold; font-size:12px;
        }
        QHeaderView::section:first { border-top-left-radius:8px; }
        QHeaderView::section:last  { border-top-right-radius:8px; }
    )");

    mainLayout->addWidget(table, 1);

    // ── Connections ───────────────────────────────────────
    connect(registerBtn, &QPushButton::clicked, this, &StudentItemsWindow::registerItem);
    connect(deleteBtn,   &QPushButton::clicked, this, &StudentItemsWindow::deleteItem);
    connect(refreshBtn,  &QPushButton::clicked, this, &StudentItemsWindow::loadData);
    connect(searchEdit,  &QLineEdit::textChanged, this, [this](const QString &txt) {
        int visible = 0;
        QString t = txt.toLower();
        for (int row = 0; row < table->rowCount(); row++) {
            bool match = false;
            for (int col : {1, 2, 4, 5, 6}) {  // Owner, Name, Brand, Model, Serial
                if (table->item(row, col) &&
                    table->item(row, col)->text().toLower().contains(t)) {
                    match = true; break;
                }
            }
            table->setRowHidden(row, !match);
            if (match) ++visible;
        }
        statsLabel->setText(QString("%1 item%2").arg(visible).arg(visible==1?"":"s"));
    });
}

void StudentItemsWindow::loadData()
{
    table->setRowCount(0);
    QSqlQuery q(QSqlDatabase::database());

    // 9 columns: id, owner, item_name, category, brand, model, serial, status, location
    QString sql = R"(
        SELECT si.item_id, u.full_name, si.item_name,
               si.category,
               NVL(si.brand, '-'),
               NVL(si.model, '-'),
               NVL(si.serial_number, '-'),
               si.status,
               NVL(l.building_name || ' — ' || l.room_number, 'N/A')
        FROM STUDENT_ITEMS si
        JOIN  USERS     u ON si.student_id  = u.user_id
        LEFT JOIN LOCATIONS l ON si.location_id = l.location_id
    )";

    if (m_role == "STUDENT")
        sql += QString(" WHERE si.student_id = %1").arg(m_userId);
    sql += " ORDER BY si.item_id";

    if (!q.exec(sql)) {
        QMessageBox::critical(this, "DB Error", q.lastError().text());
        return;
    }

    table->setSortingEnabled(false);
    while (q.next()) {
        int row = table->rowCount();
        table->insertRow(row);

        for (int col = 0; col < 9; col++) {
            auto *cell = new QTableWidgetItem(q.value(col).toString());
            cell->setTextAlignment(col == 0 ? Qt::AlignCenter : Qt::AlignVCenter | Qt::AlignLeft);

            if (col == 7)  // Status column — colored badge
                applyStatusBadge(cell, q.value(col).toString());

            table->setItem(row, col, cell);
        }
    }
    table->setSortingEnabled(true);

    int total = table->rowCount();
    statsLabel->setText(QString("%1 item%2").arg(total).arg(total == 1 ? "" : "s"));
}

void StudentItemsWindow::applyStatusBadge(QTableWidgetItem *item, const QString &status)
{
    item->setTextAlignment(Qt::AlignCenter);
    item->setFont(QFont("Segoe UI", 8, QFont::Bold));

    if (status == "REGISTERED") {
        item->setForeground(QColor("#1e1e2e"));
        item->setBackground(QColor("#a6e3a1"));
    } else if (status == "OUT_OF_CAMPUS") {
        item->setForeground(QColor("#1e1e2e"));
        item->setBackground(QColor("#f9e2af"));
    } else if (status == "LOST" || status == "STOLEN") {
        item->setForeground(QColor("#1e1e2e"));
        item->setBackground(QColor("#f38ba8"));
    } else {
        item->setForeground(QColor("#cdd6f4"));
        item->setBackground(QColor("#45475a"));
    }
}

void StudentItemsWindow::registerItem()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Register Student Item");
    dlg.setStyleSheet("background:#1e1e2e; color:#cdd6f4;");
    dlg.setMinimumWidth(400);

    auto *form = new QFormLayout(&dlg);
    form->setSpacing(12);
    form->setContentsMargins(20, 20, 20, 20);
    QString inputStyle =
        "background:#313244; color:#cdd6f4; border:1px solid #45475a;"
        "border-radius:6px; padding:6px;";

    auto *nameEdit  = new QLineEdit(); nameEdit->setStyleSheet(inputStyle);
    auto *brandEdit = new QLineEdit(); brandEdit->setStyleSheet(inputStyle);
    auto *modelEdit = new QLineEdit(); modelEdit->setStyleSheet(inputStyle);
    auto *serialEdit = new QLineEdit(); serialEdit->setStyleSheet(inputStyle);

    nameEdit->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z0-9 ._-]{2,50}$"), this
            )
        );

    brandEdit->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z0-9 ._-]{2,30}$"), this
            )
        );

    modelEdit->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z0-9 ._-]{2,30}$"), this
            )
        );

    serialEdit->setValidator(
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z0-9-]{4,40}$"), this
            )
        );
    auto *catCombo = new QComboBox(); catCombo->setStyleSheet(inputStyle);
    catCombo->addItems({"LAPTOP","PHONE","CLOTHING","BOOK","INSTRUMENT","SPORTS","OTHER"});

    QComboBox *studentCombo = nullptr;
    if (m_role != "STUDENT") {
        studentCombo = new QComboBox();
        studentCombo->setStyleSheet(inputStyle);
        QSqlQuery sq;
        sq.exec("SELECT user_id, full_name FROM USERS WHERE role='STUDENT' AND is_active='Y' ORDER BY full_name");
        while (sq.next())
            studentCombo->addItem(sq.value(1).toString(), sq.value(0).toInt());
        form->addRow("Student:", studentCombo);
    }

    // Location combo — loaded from LOCATIONS table
    auto *locCombo = new QComboBox(); locCombo->setStyleSheet(inputStyle);
    locCombo->addItem("— None —", QVariant());   // nullable
    {
        QSqlQuery lq(QSqlDatabase::database());
        lq.exec("SELECT LOCATION_ID, BUILDING_NAME || ' - ' || NVL(ROOM_NUMBER,'N/A') "
                "FROM LOCATIONS ORDER BY BUILDING_NAME");
        while (lq.next())
            locCombo->addItem(lq.value(1).toString(), lq.value(0).toInt());
    }

    form->addRow("Item Name:", nameEdit);
    form->addRow("Category:",  catCombo);
    form->addRow("Brand:",     brandEdit);
    form->addRow("Model:",     modelEdit);
    form->addRow("Serial No:", serialEdit);
    form->addRow("Location:",  locCombo);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    btns->setStyleSheet(
        "QPushButton{background:#313244;color:#cdd6f4;border-radius:6px;padding:6px 14px;}");
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;
    if (nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Error", "Item name is required.");
        return;
    }

    int ownerId = (m_role == "STUDENT") ? m_userId
                                        : (studentCombo ? studentCombo->currentData().toInt() : m_userId);

    QString serial = serialEdit->text().trimmed();
    QVariant locId = locCombo->currentData();   // null variant if "— None —" selected

    QSqlQuery q;
    q.prepare(R"(
        INSERT INTO STUDENT_ITEMS
            (item_id, student_id, item_name, category, brand, model,
             serial_number, location_id, status)
        VALUES
            (SEQ_STUDENT_ITEMS.NEXTVAL, :uid, :name, :cat, :brand, :model,
             :serial, :loc, 'REGISTERED')
    )");
    q.bindValue(":uid",    ownerId);
    q.bindValue(":name",   nameEdit->text().trimmed());
    q.bindValue(":cat",    catCombo->currentText());
    q.bindValue(":brand",  brandEdit->text().trimmed());
    q.bindValue(":model",  modelEdit->text().trimmed());
    q.bindValue(":serial", serial.isEmpty() ? QVariant() : serial);
    q.bindValue(":loc",    locId);

    if (q.exec()) {
        QSqlQuery q2; q2.exec("COMMIT");
        QMessageBox::information(this, "Success", "Item registered successfully!");
        loadData();
    } else {
        QMessageBox::critical(this, "Error", q.lastError().text());
    }
}

void StudentItemsWindow::deleteItem()
{
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Select Row", "Please select an item to remove.");
        return;
    }

    if (!table->item(row, 0) || !table->item(row, 2)) {
        QMessageBox::warning(this, "Error", "Invalid selected row.");
        return;
    }

    int itemId = table->item(row, 0)->text().toInt();
    QString itemName = table->item(row, 2)->text();

    auto reply = QMessageBox::question(
        this,
        "Remove Item",
        "Remove item: " + itemName + "?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) return;

    QSqlQuery q(QSqlDatabase::database());
    q.prepare("DELETE FROM STUDENT_ITEMS WHERE ITEM_ID = :id");
    q.bindValue(":id", itemId);

    if (!q.exec()) {
        QMessageBox::critical(this, "Delete Error", q.lastError().text());
        return;
    }

    QSqlQuery commit(QSqlDatabase::database());
    commit.exec("COMMIT");

    QMessageBox::information(this, "Deleted", "Item removed successfully.");
    loadData();
}

void StudentItemsWindow::styleButton(QPushButton *btn, const QString &color)
{
    btn->setStyleSheet(QString(
                           "QPushButton{background:%1;color:#1e1e2e;border-radius:6px;"
                           "padding:7px 16px;font-weight:bold;}"
                           ).arg(color));
}