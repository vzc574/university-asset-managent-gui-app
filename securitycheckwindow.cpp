#include "securitycheckwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <cmath>
#include <QFont>

SecurityCheckWindow::SecurityCheckWindow(int userId,
                                         const QString &role,
                                         QWidget *parent)
    : QWidget(parent), m_userId(userId), m_role(role)
{
    setWindowFlags(
        Qt::Window |
        Qt::WindowMinimizeButtonHint |
        Qt::WindowMaximizeButtonHint |
        Qt::WindowCloseButtonHint
        );

    setupUI();
    loadSerialsBST();
    loadOutsidePanel();
}

void SecurityCheckWindow::setupUI()
{
    setWindowTitle("Security Exit Check");
    setMinimumSize(800, 500);

    setStyleSheet(R"(
    QWidget {
        background-color:#11111b;
        color:#cdd6f4;
        font-family:Segoe UI;
    }

    QLabel#title {
        font-size:26px;
        font-weight:bold;
        color:#cba6f7;
    }

    QLineEdit {
        background:#313244;
        color:#cdd6f4;
        border:1px solid #45475a;
        border-radius:8px;
        padding:10px;
        font-size:14px;
    }

    QLineEdit:focus {
        border:2px solid #0f6e56;
    }

    QPushButton {
        background:#0f6e56;
        color:white;
        border:none;
        border-radius:8px;
        padding:10px 18px;
        font-weight:bold;
        font-size:13px;
    }

    QPushButton:hover {
        background:#1D9E75;
    }

    QTableWidget {
        background:#181825;
        alternate-background-color:#1e1e2e;
        color:#cdd6f4;
        border:none;
        gridline-color:#313244;
        selection-background-color:#45475a;
    }

    QTableWidget::item {
        padding:8px;
        background:#181825;
    }

    QHeaderView::section {
        background:#313244;
        color:#cba6f7;
        padding:10px;
        border:none;
        font-weight:bold;
    }

    QScrollBar:vertical {
        background:#11111b;
        width:10px;
    }

    QScrollBar::handle:vertical {
        background:#45475a;
        border-radius:5px;
    }

    QMessageBox {
        background:#1e1e2e;
    }
)");

    // ── Two-column layout: left = outside panel, right = search ──
    auto *mainLayout  = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto *title = new QLabel("🛡  Security Exit Check");
    title->setObjectName("title");
    mainLayout->addWidget(title);

    auto *columns = new QHBoxLayout();
    columns->setSpacing(14);

    // ══ LEFT: Currently Outside Campus ═══════════════════════
    auto *leftCol    = new QVBoxLayout();
    outsideLabel     = new QLabel("🚫  Currently Outside Campus  (0)");
    outsideLabel->setStyleSheet(
        "font-size:13px; font-weight:bold; color:#f38ba8; padding:4px 0;");
    leftCol->addWidget(outsideLabel);

    outsideTable = new QTableWidget();
    outsideTable->setColumnCount(4);
    outsideTable->setHorizontalHeaderLabels({"ID","Owner","Item Name","Serial No."});
    outsideTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    outsideTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    outsideTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    outsideTable->verticalHeader()->setVisible(false);
    outsideTable->setFixedWidth(370);
    outsideTable->setStyleSheet(R"(
        QTableWidget { background:#2d0f0f; color:#ffffff; border:2px solid #f38ba8;
                       border-radius:6px; gridline-color:#5a2020; }
        QTableWidget::item { padding:6px 8px; color:#ffffff; }
        QTableWidget::item:selected { background:#f38ba8; color:#1e1e2e; }
        QHeaderView::section { background:#f38ba8; color:#1e1e2e; padding:7px;
                                border:none; font-weight:bold; font-size:11px; }
    )");
    leftCol->addWidget(outsideTable, 1);

    auto *refreshOutsideBtn = new QPushButton("↻  Refresh List");
    refreshOutsideBtn->setStyleSheet(
        "background:#2a0f0f; color:#f38ba8; border:1px solid #f38ba8;"
        "border-radius:6px; padding:5px 12px; font-size:11px;");
    leftCol->addWidget(refreshOutsideBtn);

    columns->addLayout(leftCol);

    // ══ RIGHT: Search panel ═══════════════════════════════════
    auto *rightCol = new QVBoxLayout();
    rightCol->setSpacing(10);

    auto *info = new QLabel("Enter serial number to verify owner before exit:");
    info->setStyleSheet("font-size:12px; color:#a6adc8;");
    rightCol->addWidget(info);

    auto *searchRow = new QHBoxLayout();
    serialEdit = new QLineEdit();
    serialEdit->setPlaceholderText("Enter serial number...");
    serialEdit->setFixedHeight(36);
    searchBtn  = new QPushButton("Search");
    searchBtn->setFixedHeight(36);
    searchRow->addWidget(serialEdit, 1);
    searchRow->addWidget(searchBtn);
    rightCol->addLayout(searchRow);

    // BST info bar + reload
    auto *bstRow = new QHBoxLayout();
    bstLabel = new QLabel("BST Index: not loaded yet");
    bstLabel->setStyleSheet(
        "background:#1e1e2e; color:#89b4fa; font-size:11px;"
        "padding:5px 10px; border-radius:6px; border:1px solid #313244;");
    reloadBstBtn = new QPushButton("↻  Reload BST");
    reloadBstBtn->setStyleSheet(
        "background:#313244; color:#a6e3a1; border:1px solid #45475a;"
        "border-radius:6px; padding:5px 12px; font-size:11px; font-weight:bold;");
    reloadBstBtn->setFixedHeight(30);
    bstRow->addWidget(bstLabel, 1);
    bstRow->addWidget(reloadBstBtn);
    rightCol->addLayout(bstRow);

    // Action buttons
    auto *actionLayout = new QHBoxLayout();
    allowExitBtn    = new QPushButton("✅ Allow Exit");
    denyExitBtn     = new QPushButton("❌ Deny Exit");
    markReturnedBtn = new QPushButton("↩ Mark Returned");
    clearBtn        = new QPushButton("🧹 Clear");

    allowExitBtn->setStyleSheet(
        "background:#16a085;color:white;border-radius:8px;padding:9px;font-weight:bold;");
    denyExitBtn->setStyleSheet(
        "background:#c0392b;color:white;border-radius:8px;padding:9px;font-weight:bold;");
    markReturnedBtn->setStyleSheet(
        "background:#8e44ad;color:white;border-radius:8px;padding:9px;font-weight:bold;");
    clearBtn->setStyleSheet(
        "background:#34495e;color:white;border-radius:8px;padding:9px;font-weight:bold;");

    allowExitBtn->setEnabled(false);
    denyExitBtn->setEnabled(false);
    markReturnedBtn->setEnabled(false);

    actionLayout->addWidget(allowExitBtn);
    actionLayout->addWidget(denyExitBtn);
    actionLayout->addWidget(markReturnedBtn);
    actionLayout->addWidget(clearBtn);
    rightCol->addLayout(actionLayout);

    resultLabel = new QLabel("");
    rightCol->addWidget(resultLabel);

    // Search result table
    table = new QTableWidget();
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels(
        {"Item ID","Owner","Student ID","Item Name","Category","Brand","Status"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet(R"(
        QTableWidget { background:#181825; color:#cdd6f4; border:1px solid #313244;
                       border-radius:6px; gridline-color:#313244; }
        QTableWidget::item { padding:6px; }
        QTableWidget::item:selected { background:#45475a; }
        QHeaderView::section { background:#313244; color:#cba6f7; padding:8px;
                                border:none; font-weight:bold; }
    )");
    rightCol->addWidget(table, 1);

    columns->addLayout(rightCol, 1);
    mainLayout->addLayout(columns, 1);

    connect(refreshOutsideBtn, &QPushButton::clicked,
            this, &SecurityCheckWindow::loadOutsidePanel);

    connect(searchBtn,    &QPushButton::clicked,  this, &SecurityCheckWindow::searchItem);
    connect(serialEdit,   &QLineEdit::returnPressed, this, &SecurityCheckWindow::searchItem);
    connect(reloadBstBtn, &QPushButton::clicked,  this, &SecurityCheckWindow::loadSerialsBST);
    connect(allowExitBtn, &QPushButton::clicked, this, [=]() {
        QString serial = serialEdit->text().trimmed();
        if (serial.isEmpty()) return;

        QSqlQuery q(QSqlDatabase::database());
        // Update by serial number directly — does not depend on table rows
        q.prepare("UPDATE STUDENT_ITEMS SET STATUS='OUT_OF_CAMPUS' "
                  "WHERE LOWER(SERIAL_NUMBER) = LOWER(:s)");
        q.bindValue(":s", serial);

        if (!q.exec()) {
            QMessageBox::critical(this, "DB Error", q.lastError().text());
            return;
        }
        QSqlQuery(QSqlDatabase::database()).exec("COMMIT");

        int affected = q.numRowsAffected();
        if (affected > 0) {
            QMessageBox::information(this, "Exit Allowed",
                QString("✅ %1 item(s) marked OUT OF CAMPUS. Exit approved.").arg(affected));
        } else {
            QMessageBox::warning(this, "Not Found",
                "Serial number not found in STUDENT_ITEMS. Cannot allow exit.");
        }
        searchItem();
        loadOutsidePanel();
    });

    connect(denyExitBtn, &QPushButton::clicked, this, [=]() {
        QMessageBox::warning(this, "Exit Denied",
                             "Exit denied. Item is not verified or not registered.");
    });

    connect(markReturnedBtn, &QPushButton::clicked, this, [=]() {
        QString serial = serialEdit->text().trimmed();
        if (serial.isEmpty()) return;

        QSqlQuery q(QSqlDatabase::database());
        q.prepare("UPDATE STUDENT_ITEMS SET STATUS='REGISTERED' "
                  "WHERE LOWER(SERIAL_NUMBER) = LOWER(:s)");
        q.bindValue(":s", serial);

        if (!q.exec()) {
            QMessageBox::critical(this, "DB Error", q.lastError().text());
            return;
        }
        QSqlQuery(QSqlDatabase::database()).exec("COMMIT");

        int affected = q.numRowsAffected();
        markReturnedBtn->setEnabled(false);
        if (affected > 0) {
            QMessageBox::information(this, "Returned",
                QString("↩ %1 item(s) marked REGISTERED (returned to campus).").arg(affected));
            searchItem();
            loadOutsidePanel();
        } else {
            QMessageBox::warning(this, "Not Found",
                "Serial number not found. Could not mark as returned.");
        }
    });

    connect(clearBtn, &QPushButton::clicked, this, [=]() {
        serialEdit->clear();
        table->setRowCount(0);
        resultLabel->clear();
        allowExitBtn->setEnabled(false);
        denyExitBtn->setEnabled(false);
        markReturnedBtn->setEnabled(false);
    });
}

void SecurityCheckWindow::loadSerialsBST()
{
    // Load every STUDENT_ITEMS serial number into the BST — O(n log n)
    m_dsa.bstClear();

    QSqlQuery q(QSqlDatabase::database());
    q.exec("SELECT ITEM_ID, SERIAL_NUMBER FROM STUDENT_ITEMS WHERE SERIAL_NUMBER IS NOT NULL");

    int loaded = 0;
    while (q.next()) {
        int     id     = q.value(0).toInt();
        QString serial = q.value(1).toString().trimmed().toLower();
        if (!serial.isEmpty()) {
            m_dsa.bstInsert(serial.toStdString(), id);
            ++loaded;
        }
    }

    int h = m_dsa.bstHeight();
    // Expected comparisons for a balanced BST: ceil(log2(n+1))
    int expected = (loaded > 0) ? (int)std::ceil(std::log2(loaded + 1)) : 0;
    bstLabel->setText(
        QString("BST Index: %1 serials loaded | Height: %2 | Search cost: O(log n) ≈ %3 comparisons")
            .arg(loaded).arg(h).arg(expected));
}

void SecurityCheckWindow::searchItem()
{
    QString serial = serialEdit->text().trimmed();

    if (serial.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter serial number.");
        return;
    }

    // ── Step 1: BST lookup O(log n) ───────────────────────
    int bstItemId = -1;
    bool bstHit   = m_dsa.bstSearch(serial.toLower().toStdString(), bstItemId);

    int h = m_dsa.bstHeight();
    int n = m_dsa.bstSize();
    int logN = (n > 0) ? (int)std::ceil(std::log2(n + 1)) : 0;

    if (bstHit)
        bstLabel->setText(
            QString("BST: FOUND '%1' in O(log %2) ≈ %3 comparisons | linear scan would be O(%2)")
                .arg(serial).arg(n).arg(logN));
    else
        bstLabel->setText(
            QString("BST: '%1' NOT in index (unregistered) — O(log %2) ≈ %3 comparisons")
                .arg(serial).arg(n).arg(logN));

    // ── Step 2: DB query for full record details ───────────
    table->setRowCount(0);
    allowExitBtn->setEnabled(false);
    denyExitBtn->setEnabled(false);
    markReturnedBtn->setEnabled(false);
    QSqlQuery q(QSqlDatabase::database());

    // LEFT JOIN so result shows even if student user record is missing
    // Use separate bind names for each :likeX because Oracle ODBC
    // requires unique placeholder names per position
    q.prepare(R"(
        SELECT si.ITEM_ID,
               NVL(u.FULL_NAME, 'Unknown'),
               NVL(u.STUDENT_ID, '-'),
               si.ITEM_NAME,
               si.CATEGORY,
               NVL(si.BRAND, '-'),
               si.STATUS
        FROM   STUDENT_ITEMS si
        LEFT JOIN USERS u ON si.STUDENT_ID = u.USER_ID
        WHERE  LOWER(si.SERIAL_NUMBER) = LOWER(:serial)
           OR  LOWER(si.ITEM_NAME)     LIKE LOWER(:like1)
           OR  LOWER(si.BRAND)         LIKE LOWER(:like2)
    )");

    QString likeVal = "%" + serial + "%";
    q.bindValue(":serial", serial);
    q.bindValue(":like1",  likeVal);
    q.bindValue(":like2",  likeVal);

    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error", q.lastError().text());
        return;
    }

    int count = 0;
    bool hasOutOfCampus = false;
    bool hasPresent     = false;

    while (q.next()) {
        int row = table->rowCount();
        table->insertRow(row);

        for (int col = 0; col < 7; col++) {
            QTableWidgetItem *item = new QTableWidgetItem(q.value(col).toString());
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, col, item);
        }

        QString status = q.value(6).toString();
        if (status == "OUT_OF_CAMPUS") {
            hasOutOfCampus = true;
            // Pink background + dark text — clearly visible
            for (int col = 0; col < 7; col++) {
                table->item(row, col)->setBackground(QColor("#5a1a1a"));
                table->item(row, col)->setForeground(QColor("#ffffff"));
            }
            // Status badge
            table->item(row, 6)->setBackground(QColor("#f38ba8"));
            table->item(row, 6)->setForeground(QColor("#1e1e2e"));
            table->item(row, 6)->setFont(QFont("Segoe UI", 8, QFont::Bold));
            table->setRowHeight(row, 42);
        } else {
            hasPresent = true;
            for (int col = 0; col < 7; col++)
                table->item(row, col)->setForeground(QColor("#cdd6f4"));
            table->item(row, 6)->setBackground(QColor("#a6e3a1"));
            table->item(row, 6)->setForeground(QColor("#1e1e2e"));
            table->item(row, 6)->setFont(QFont("Segoe UI", 8, QFont::Bold));
        }

        count++;
    }

    if (count == 0) {
        resultLabel->setText("❌ No registered item found. Deny exit until verified.");
        resultLabel->setStyleSheet("color:#f38ba8; font-size:15px; font-weight:bold;");
        denyExitBtn->setEnabled(true);
        allowExitBtn->setEnabled(false);
        markReturnedBtn->setEnabled(false);
    } else if (hasOutOfCampus) {
        resultLabel->setText("⚠  Item is OUT OF CAMPUS — click 'Mark Returned' when student comes back.");
        resultLabel->setStyleSheet(
            "color:#1e1e2e; background:#f38ba8; font-size:13px; font-weight:bold;"
            "padding:8px 12px; border-radius:6px;");
        markReturnedBtn->setEnabled(true);
        allowExitBtn->setEnabled(false);
        denyExitBtn->setEnabled(true);
    } else {
        resultLabel->setText("✅ Item is on campus — verify owner ID then allow exit.");
        resultLabel->setStyleSheet(
            "color:#1e1e2e; background:#a6e3a1; font-size:13px; font-weight:bold;"
            "padding:8px 12px; border-radius:6px;");
        allowExitBtn->setEnabled(true);
        markReturnedBtn->setEnabled(false);
        denyExitBtn->setEnabled(true);
    }
}

void SecurityCheckWindow::loadOutsidePanel()
{
    outsideTable->setRowCount(0);
    QSqlQuery q(QSqlDatabase::database());
    bool ok = q.exec(R"(
        SELECT si.ITEM_ID, u.FULL_NAME, si.ITEM_NAME,
               NVL(si.SERIAL_NUMBER, '-')
        FROM   STUDENT_ITEMS si
        LEFT JOIN USERS u ON si.STUDENT_ID = u.USER_ID
        WHERE  si.STATUS = 'OUT_OF_CAMPUS'
        ORDER  BY si.ITEM_ID
    )");

    if (!ok) return;

    while (q.next()) {
        int row = outsideTable->rowCount();
        outsideTable->insertRow(row);
        for (int col = 0; col < 4; col++) {
            auto *cell = new QTableWidgetItem(q.value(col).toString());
            cell->setTextAlignment(Qt::AlignCenter);
            cell->setForeground(QColor("#ffffff"));   // white text — clearly visible on dark red
            outsideTable->setItem(row, col, cell);
        }
    }

    int n = outsideTable->rowCount();
    outsideLabel->setText(QString("🚫  Currently Outside Campus  (%1)").arg(n));
    outsideLabel->setStyleSheet(
        n > 0
        ? "font-size:13px; font-weight:bold; color:#f38ba8; padding:4px 0;"
        : "font-size:13px; font-weight:bold; color:#a6e3a1; padding:4px 0;");
}