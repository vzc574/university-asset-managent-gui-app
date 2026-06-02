#include "lostfoundwindow.h"
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
#include <QFont>

LostFoundWindow::LostFoundWindow(int userId, const QString &role, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_role(role)
{ setupUI(); loadData(); }

void LostFoundWindow::setupUI()
{
    setStyleSheet("background-color: #1e1e2e; color: #cdd6f4;");
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24,24,24,24); mainLayout->setSpacing(16);

    auto *title = new QLabel("🔍 Lost & Found");
    title->setStyleSheet("font-size:22px; font-weight:bold; color:#cba6f7;");
    mainLayout->addWidget(title);

    auto *toolbar = new QHBoxLayout();
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search item description...");
    searchEdit->setStyleSheet("background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:6px;padding:6px 10px;");
    typeFilter = new QComboBox();
    typeFilter->addItems({"All Types","LOST","FOUND"});
    typeFilter->setStyleSheet("background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:6px;padding:6px;");

    addBtn     = new QPushButton("+ Add Report");
    claimBtn   = new QPushButton("✔ Mark Returned");
    refreshBtn = new QPushButton("↺ Refresh");
    styleButton(addBtn,"#a6e3a1"); styleButton(claimBtn,"#89b4fa"); styleButton(refreshBtn,"#585b70");

    toolbar->addWidget(searchEdit); toolbar->addWidget(typeFilter); toolbar->addStretch();
    toolbar->addWidget(addBtn); toolbar->addWidget(claimBtn); toolbar->addWidget(refreshBtn);
    mainLayout->addLayout(toolbar);

    table = new QTableWidget();
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({"ID","Description","Type","Location","Reported By","Date","Status"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet("QTableWidget{background:#181825;color:#cdd6f4;border:none;gridline-color:#313244;alternate-background-color:#252535;}"
                         "QTableWidget::item{color:#cdd6f4;}QTableWidget::item:selected{background:#45475a;color:white;}"
                         "QHeaderView::section{background:#313244;color:#cba6f7;padding:8px;border:none;font-weight:bold;}");
    mainLayout->addWidget(table);

    connect(addBtn,     &QPushButton::clicked,         this, &LostFoundWindow::addReport);
    connect(claimBtn,   &QPushButton::clicked,         this, &LostFoundWindow::markClaimed);
    connect(refreshBtn, &QPushButton::clicked,         this, &LostFoundWindow::loadData);
    connect(searchEdit, &QLineEdit::textChanged,       this, &LostFoundWindow::filterTable);
    connect(typeFilter, &QComboBox::currentTextChanged,this, &LostFoundWindow::filterTable);
}

void LostFoundWindow::loadData()
{
    // Correct column names from Oracle DESC LOST_FOUND:
    // RECORD_ID, ITEM_ID, REPORTED_BY, FOUND_BY, RECORD_TYPE,
    // ITEM_DESCRIPTION, FOUND_LOCATION, REPORTED_AT, RESOLVED_AT, STATUS, NOTES
    QSqlQuery q(QSqlDatabase::database());
    bool ok = q.exec(
        "SELECT lf.RECORD_ID, "
        "       lf.ITEM_DESCRIPTION, "
        "       lf.RECORD_TYPE, "
        "       l.BUILDING_NAME || ' - ' || NVL(l.ROOM_NUMBER,'N/A'), "
        "       u.FULL_NAME, "
        "       TO_CHAR(lf.REPORTED_AT,'YYYY-MM-DD'), "
        "       NVL(lf.STATUS,'OPEN') "
        "FROM   LOST_FOUND lf "
        "JOIN   USERS u ON lf.REPORTED_BY = u.USER_ID "
        "LEFT JOIN LOCATIONS l ON lf.FOUND_LOCATION = l.LOCATION_ID "
        "ORDER BY lf.REPORTED_AT DESC");
    if (!ok) { qDebug()<<"LostFound error:"<<q.lastError().text(); QMessageBox::critical(this,"DB Error",q.lastError().text()); return; }
    table->setRowCount(0);
    while (q.next()) {
        int row=table->rowCount(); table->insertRow(row);
        QString status = q.value(6).toString();
        bool returned  = (status == "RETURNED");

        for (int col=0;col<7;col++) {
            auto *item=new QTableWidgetItem(q.value(col).toString());
            item->setTextAlignment(Qt::AlignCenter);

            if (returned) {
                // Dim the whole row to show it is resolved
                item->setForeground(QColor("#585b70"));
                item->setFont([&]{ QFont f; f.setStrikeOut(true); return f; }());
            }
            if (col==2 && !returned)
                item->setForeground(q.value(col).toString()=="LOST"?QColor(243,139,168):QColor(166,227,161));
            if (col==6) {
                if (returned) {
                    item->setForeground(QColor("#1e1e2e"));
                    item->setBackground(QColor("#a6e3a1"));  // green badge
                    item->setFont(QFont("Segoe UI", 8, QFont::Bold));
                } else {
                    item->setForeground(QColor("#1e1e2e"));
                    item->setBackground(QColor("#f9e2af"));  // yellow badge = open
                    item->setFont(QFont("Segoe UI", 8, QFont::Bold));
                }
            }
            table->setItem(row,col,item);
        }
    }
}

void LostFoundWindow::addReport()
{
    QDialog dlg(this); dlg.setWindowTitle("Add Lost/Found Report");
    dlg.setStyleSheet("background:#1e1e2e;color:#cdd6f4;"); dlg.setMinimumWidth(440);
    auto *form=new QFormLayout(&dlg); form->setSpacing(12); form->setContentsMargins(20,20,20,20);
    QString s="background:#313244;color:#cdd6f4;border:1px solid #45475a;border-radius:6px;padding:6px;";
    auto *typeCombo=new QComboBox(); typeCombo->addItems({"LOST","FOUND"}); typeCombo->setStyleSheet(s);
    auto *descEdit=new QTextEdit(); descEdit->setStyleSheet(s); descEdit->setMaximumHeight(80);
    descEdit->setPlaceholderText("Describe the item in detail...");
    auto *locCombo=new QComboBox(); locCombo->setStyleSheet(s);
    QSqlQuery lq(QSqlDatabase::database());
    lq.exec("SELECT LOCATION_ID,BUILDING_NAME||' - '||NVL(ROOM_NUMBER,'N/A') FROM LOCATIONS ORDER BY BUILDING_NAME");
    while(lq.next()) locCombo->addItem(lq.value(1).toString(),lq.value(0).toInt());
    form->addRow("Type:",typeCombo); form->addRow("Description:",descEdit); form->addRow("Location:",locCombo);
    auto *btns=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    btns->setStyleSheet("QPushButton{background:#313244;color:#cdd6f4;border-radius:6px;padding:6px 14px;}");
    form->addRow(btns);
    connect(btns,&QDialogButtonBox::accepted,&dlg,&QDialog::accept);
    connect(btns,&QDialogButtonBox::rejected,&dlg,&QDialog::reject);
    if (dlg.exec()!=QDialog::Accepted) return;
    QString desc=descEdit->toPlainText().trimmed();
    if (desc.isEmpty()){QMessageBox::warning(this,"Error","Description is required."); return;}
    QSqlQuery q(QSqlDatabase::database());
    q.prepare("INSERT INTO LOST_FOUND(RECORD_ID,REPORTED_BY,RECORD_TYPE,ITEM_DESCRIPTION,FOUND_LOCATION,REPORTED_AT) "
              "VALUES(SEQ_LOST_FOUND.NEXTVAL,:by,:type,:desc,:loc,SYSDATE)");
    q.bindValue(":by",  m_userId);
    q.bindValue(":type",typeCombo->currentText());
    q.bindValue(":desc",desc);
    q.bindValue(":loc", locCombo->currentData().toInt());
    if (q.exec()){QMessageBox::information(this,"Success","Report added."); loadData();}
    else{qDebug()<<"LF insert:"<<q.lastError().text(); QMessageBox::critical(this,"Error",q.lastError().text());}
}

void LostFoundWindow::markClaimed()
{
    int row=table->currentRow();
    if (row<0){QMessageBox::warning(this,"Select Row","Select a record first."); return;}
    int id=table->item(row,0)->text().toInt();
    if (QMessageBox::question(this,"Mark Returned","Mark as RETURNED?",QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes) return;
    QSqlQuery q(QSqlDatabase::database());
    q.prepare("UPDATE LOST_FOUND SET STATUS='RETURNED',RESOLVED_AT=SYSDATE WHERE RECORD_ID=:id");
    q.bindValue(":id",id);
    if (q.exec()) {
        QSqlQuery(QSqlDatabase::database()).exec("COMMIT");
        loadData();
    } else {
        QMessageBox::critical(this,"Error",q.lastError().text());
    }
}

void LostFoundWindow::filterTable()
{
    QString search=searchEdit->text().toLower();
    QString type=typeFilter->currentText();
    for(int row=0;row<table->rowCount();row++){
        bool ms=search.isEmpty()||table->item(row,1)->text().toLower().contains(search);
        bool mt=(type=="All Types")||table->item(row,2)->text()==type;
        table->setRowHidden(row,!(ms&&mt));
    }
}

void LostFoundWindow::styleButton(QPushButton *btn,const QString &color)
{ btn->setStyleSheet(QString("QPushButton{background:%1;color:#1e1e2e;border-radius:6px;padding:7px 16px;font-weight:bold;}").arg(color)); }