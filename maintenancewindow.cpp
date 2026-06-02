#include "maintenancewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDebug>
#include <algorithm>
#include <QFont>

MaintenanceWindow::MaintenanceWindow(int userId, const QString &role, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_role(role)
{ setupUI(); loadData(); }

void MaintenanceWindow::setupUI()
{
    setStyleSheet("background-color: #1e1e2e; color: #cdd6f4;");
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24,24,24,24); mainLayout->setSpacing(16);

    auto *title = new QLabel("🔧 Maintenance Log");
    title->setStyleSheet("font-size:22px; font-weight:bold; color:#cba6f7;");
    mainLayout->addWidget(title);

    auto *toolbar = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by asset or description...");
    searchEdit->setStyleSheet("background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:6px;padding:6px 10px;");
    statusFilter = new QComboBox();
    statusFilter->addItems({"All Status","PENDING","IN_PROGRESS","RESOLVED","CANCELLED"});
    statusFilter->setStyleSheet("background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:6px;padding:6px;");

    reportBtn     = new QPushButton("+ Report Issue");
    resolveBtn    = new QPushButton("✔ Resolve Issue");
    refreshBtn    = new QPushButton("↺ Refresh");
    prioritizeBtn = new QPushButton("⚡ Auto-Prioritize");
    styleButton(reportBtn,"#a6e3a1"); styleButton(resolveBtn,"#89b4fa");
    styleButton(refreshBtn,"#585b70"); styleButton(prioritizeBtn,"#f9e2af");
    bool restricted = (m_role == "STUDENT" || m_role == "SECURITY");
    if (restricted) {
        resolveBtn->setVisible(false);
        prioritizeBtn->setVisible(false);
    }

    toolbar->addWidget(searchEdit); toolbar->addWidget(statusFilter); toolbar->addStretch();
    toolbar->addWidget(prioritizeBtn);
    toolbar->addWidget(reportBtn); toolbar->addWidget(resolveBtn); toolbar->addWidget(refreshBtn);
    mainLayout->addLayout(toolbar);

    // Greedy info label
    greedyLabel = new QLabel("Greedy Scheduler: click ⚡ Auto-Prioritize to rank tasks by urgency");
    greedyLabel->setStyleSheet(
        "background:#1e1e2e; color:#f9e2af; font-size:12px;"
        "padding:5px 12px; border-radius:6px; border:1px solid #45475a;");
    mainLayout->addWidget(greedyLabel);

    table = new QTableWidget();
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID","Asset","Reported By","Description","Priority","Status","Date"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet("QTableWidget{background:#181825;color:#cdd6f4;border:none;gridline-color:#313244;}"
                         "QTableWidget::item{color:#cdd6f4;}QTableWidget::item:selected{background:#45475a;}"
                         "QHeaderView::section{background:#313244;color:#cba6f7;padding:8px;border:none;font-weight:bold;}");
    mainLayout->addWidget(table);

    connect(reportBtn,    &QPushButton::clicked,           this, &MaintenanceWindow::reportIssue);
    connect(resolveBtn,   &QPushButton::clicked,           this, &MaintenanceWindow::resolveIssue);
    connect(refreshBtn,   &QPushButton::clicked,           this, &MaintenanceWindow::loadData);
    connect(prioritizeBtn,&QPushButton::clicked,           this, &MaintenanceWindow::greedyPrioritize);
    connect(searchEdit,   &QLineEdit::textChanged,         this, &MaintenanceWindow::filterTable);
    connect(statusFilter, &QComboBox::currentTextChanged,  this, &MaintenanceWindow::filterTable);
}

void MaintenanceWindow::loadData()
{
    bool restricted = (m_role == "STUDENT" || m_role == "SECURITY");

    QString sql =
        "SELECT m.MAINTENANCE_ID, a.ASSET_NAME, u.FULL_NAME, m.DESCRIPTION,"
        "       m.PRIORITY, m.STATUS, TO_CHAR(m.REPORTED_AT,'YYYY-MM-DD') "
        "FROM   MAINTENANCE_LOG m "
        "JOIN   ASSETS a ON m.ASSET_ID   = a.ASSET_ID "
        "JOIN   USERS  u ON m.REPORTED_BY = u.USER_ID ";

    // STUDENT and SECURITY only see their own reports
    if (restricted)
        sql += QString("WHERE m.REPORTED_BY = %1 ").arg(m_userId);

    sql += "ORDER BY m.REPORTED_AT DESC";

    QSqlQuery q(QSqlDatabase::database());
    if (!q.exec(sql)) {
        qDebug() << "Maint error:" << q.lastError().text();
        QMessageBox::critical(this, "DB Error", q.lastError().text());
        return;
    }

    table->setRowCount(0);
    while (q.next()) {
        int row = table->rowCount();
        table->insertRow(row);
        for (int col = 0; col < 7; col++) {
            auto *item = new QTableWidgetItem(q.value(col).toString());
            item->setTextAlignment(Qt::AlignCenter);

            // Priority badge (col 4)
            if (col == 4) {
                QString pr = q.value(col).toString();
                if      (pr == "URGENT") { item->setBackground(QColor("#f38ba8")); item->setForeground(QColor("#1e1e2e")); }
                else if (pr == "HIGH")   { item->setBackground(QColor("#fab387")); item->setForeground(QColor("#1e1e2e")); }
                else if (pr == "NORMAL") { item->setBackground(QColor("#89b4fa")); item->setForeground(QColor("#1e1e2e")); }
                else                     { item->setBackground(QColor("#45475a")); item->setForeground(QColor("#cdd6f4")); }
                item->setFont(QFont("Segoe UI", 8, QFont::Bold));
            }

            // Status badge (col 5)
            if (col == 5) {
                QString st = q.value(col).toString();
                if      (st == "PENDING")     { item->setBackground(QColor("#f38ba8")); item->setForeground(QColor("#1e1e2e")); }
                else if (st == "IN_PROGRESS") { item->setBackground(QColor("#f9e2af")); item->setForeground(QColor("#1e1e2e")); }
                else if (st == "RESOLVED")    { item->setBackground(QColor("#a6e3a1")); item->setForeground(QColor("#1e1e2e")); }
                else if (st == "CANCELLED")   { item->setBackground(QColor("#45475a")); item->setForeground(QColor("#cdd6f4")); }
                item->setFont(QFont("Segoe UI", 8, QFont::Bold));
            }

            table->setItem(row, col, item);
        }
    }

    // Update greedy label to show scope
    if (restricted)
        greedyLabel->setText(
            QString("Showing your reports only — %1 record(s) found")
                .arg(table->rowCount()));
}

void MaintenanceWindow::reportIssue()
{
    QDialog dlg(this); dlg.setWindowTitle("Report Issue");
    dlg.setStyleSheet("background:#1e1e2e;color:#cdd6f4;"); dlg.setMinimumWidth(440);
    auto *form=new QFormLayout(&dlg); form->setSpacing(12); form->setContentsMargins(20,20,20,20);
    QString s="background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:6px;padding:6px;";
    auto *assetCombo=new QComboBox(); assetCombo->setStyleSheet(s);
    auto *typeCombo=new QComboBox(); typeCombo->addItems({"BROKEN","MISSING","SCHEDULED","OTHER"}); typeCombo->setStyleSheet(s);
    auto *priCombo=new QComboBox(); priCombo->addItems({"NORMAL","LOW","HIGH","URGENT"}); priCombo->setStyleSheet(s);
    auto *descEdit=new QTextEdit(); descEdit->setStyleSheet(s); descEdit->setMaximumHeight(80);
    QSqlQuery aq(QSqlDatabase::database());
    aq.exec("SELECT ASSET_ID,ASSET_NAME FROM ASSETS ORDER BY ASSET_NAME");
    while(aq.next()) assetCombo->addItem(aq.value(1).toString(),aq.value(0).toInt());
    form->addRow("Asset:",assetCombo); form->addRow("Issue Type:",typeCombo);
    form->addRow("Priority:",priCombo); form->addRow("Description:",descEdit);
    auto *btns=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    btns->setStyleSheet("QPushButton{background:#313244;color:#cdd6f4;border-radius:6px;padding:6px 14px;}");
    form->addRow(btns);
    connect(btns,&QDialogButtonBox::accepted,&dlg,&QDialog::accept);
    connect(btns,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    if (dlg.exec()!=QDialog::Accepted) return;
    QString desc=descEdit->toPlainText().trimmed();
    if (desc.isEmpty()){QMessageBox::warning(this,"Error","Description required."); return;}
    QSqlQuery q(QSqlDatabase::database());
    q.prepare("BEGIN SP_REPORT_ISSUE(:aid,:by,:type,:desc,:pri); END;");
    q.bindValue(":aid", assetCombo->currentData().toInt());
    q.bindValue(":by",  m_userId);
    q.bindValue(":type",typeCombo->currentText());
    q.bindValue(":desc",desc);
    q.bindValue(":pri", priCombo->currentText());
    if (q.exec()){QMessageBox::information(this,"Success","Issue reported."); loadData();}
    else{qDebug()<<"SP_REPORT_ISSUE:"<<q.lastError().text(); QMessageBox::critical(this,"Error",q.lastError().text());}
}

void MaintenanceWindow::resolveIssue()
{
    int row=table->currentRow();
    if (row<0){QMessageBox::warning(this,"Select Row","Select a record first."); return;}
    if (table->item(row,5)->text()=="RESOLVED"){QMessageBox::information(this,"Info","Already resolved."); return;}
    int id=table->item(row,0)->text().toInt();
    if (QMessageBox::question(this,"Resolve","Mark as RESOLVED?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes) return;
    QSqlQuery q(QSqlDatabase::database());
    q.prepare("BEGIN SP_RESOLVE_MAINTENANCE(:id,:by,:note); END;");
    q.bindValue(":id",  id);
    q.bindValue(":by",  m_userId);
    q.bindValue(":note","Resolved via Smart Campus system");
    if (q.exec()){QMessageBox::information(this,"Done","Issue resolved."); loadData();}
    else{qDebug()<<"SP_RESOLVE:"<<q.lastError().text(); QMessageBox::critical(this,"Error",q.lastError().text());}
}

void MaintenanceWindow::filterTable()
{
    QString search=searchEdit->text().toLower();
    QString status=statusFilter->currentText();
    for(int row=0;row<table->rowCount();row++){
        bool ms=search.isEmpty()||table->item(row,1)->text().toLower().contains(search)||table->item(row,3)->text().toLower().contains(search);
        bool mt=(status=="All Status")||table->item(row,5)->text()==status;
        table->setRowHidden(row,!(ms&&mt));
    }
}

void MaintenanceWindow::styleButton(QPushButton *btn,const QString &color)
{ btn->setStyleSheet(QString("QPushButton{background:%1;color:#1e1e2e;border-radius:6px;padding:7px 16px;font-weight:bold;}").arg(color)); }

// ── Greedy Scheduling Algorithm (Ch. 9) ───────────────────
// Greedy choice property: always pick the task with the highest
// urgency score next — locally optimal = globally optimal for
// minimizing maximum waiting time of urgent tasks.
//
// Score = priority weight + status weight
//   URGENT=40, HIGH=30, NORMAL=20, LOW=10
//   PENDING +20, IN_PROGRESS +10  (unresolved tasks rank higher)
//
// Time: O(n log n) for the sort   Space: O(n) for the row buffer
void MaintenanceWindow::greedyPrioritize()
{
    struct Row {
        int      score;
        QStringList cols;
        QVector<QColor> fgColors;
    };

    QVector<Row> rows;
    for (int r = 0; r < table->rowCount(); r++) {
        Row row;
        row.score = 0;
        for (int c = 0; c < table->columnCount(); c++) {
            auto *it = table->item(r, c);
            row.cols     << (it ? it->text()                 : "");
            row.fgColors << (it ? it->foreground().color()   : QColor("#cdd6f4"));
        }
        // Priority weight (greedy key)
        const QString &pri = row.cols[4];
        if      (pri == "URGENT") row.score += 40;
        else if (pri == "HIGH")   row.score += 30;
        else if (pri == "NORMAL") row.score += 20;
        else                      row.score += 10;  // LOW

        // Status modifier — unresolved work ranks above resolved
        const QString &st = row.cols[5];
        if      (st == "PENDING")     row.score += 20;
        else if (st == "IN_PROGRESS") row.score += 10;

        rows.append(row);
    }

    // Greedy selection: highest score first — O(n log n)
    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b){ return a.score > b.score; });

    // Repopulate the table in prioritized order
    table->setRowCount(0);
    for (const Row &row : rows) {
        int r = table->rowCount();
        table->insertRow(r);
        for (int c = 0; c < table->columnCount(); c++) {
            auto *it = new QTableWidgetItem(row.cols[c]);
            it->setTextAlignment(Qt::AlignCenter);
            it->setForeground(row.fgColors[c]);
            table->setItem(r, c, it);
        }
    }

    greedyLabel->setText(
        QString("Greedy: %1 tasks ranked — URGENT PENDING first, RESOLVED NORMAL last | O(n log n)")
            .arg(rows.size()));
}