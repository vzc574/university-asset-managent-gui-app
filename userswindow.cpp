#include "userswindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCryptographicHash>
#include <QDebug>
#include <QTabWidget>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QIntValidator>
#include <QDoubleValidator>
UsersWindow::UsersWindow(int userId, const QString &role, QWidget *parent)
    : QWidget(parent), m_userId(userId), m_role(role)
{
    setupUI();
    if (m_role == "ADMIN") loadData();
}

void UsersWindow::setupUI()
{
    setStyleSheet("background-color:#1e1e2e; color:#cdd6f4;");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24,24,24,24);
    mainLayout->setSpacing(16);

    auto *title = new QLabel("👥 User Management");
    title->setStyleSheet("font-size:22px; font-weight:bold; color:#cba6f7;");
    mainLayout->addWidget(title);

    // Non-admins see access denied message
    if (m_role != "ADMIN") {
        QLabel *denied = new QLabel("🔒 Access restricted to Administrators only.");
        denied->setStyleSheet("font-size:16px; color:#f38ba8; padding:40px;");
        denied->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(denied);
        mainLayout->addStretch();
        return;
    }

    auto *toolbar = new QHBoxLayout();

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by username, name or email...");
    searchEdit->setStyleSheet("background:#313244; color:#cdd6f4; border:1px solid #45475a; border-radius:6px; padding:6px 10px;");

    addBtn    = new QPushButton("＋ Add User");
    toggleBtn = new QPushButton("⏸ Toggle Active");
    refreshBtn= new QPushButton("↺ Refresh");

    styleButton(addBtn,     "#a6e3a1");
    styleButton(toggleBtn,  "#fab387");
    styleButton(refreshBtn, "#585b70");

    toolbar->addWidget(searchEdit);
    toolbar->addStretch();
    toolbar->addWidget(addBtn);
    toolbar->addWidget(toggleBtn);
    toolbar->addWidget(refreshBtn);
    mainLayout->addLayout(toolbar);

    roleTabs = new QTabWidget();
    roleTabs->setStyleSheet(R"(
    QTabWidget::pane {
        border:1px solid #313244;
        background:#181825;
    }
    QTabBar::tab {
        background:#313244;
        color:#cdd6f4;
        padding:8px 18px;
        border-top-left-radius:6px;
        border-top-right-radius:6px;
    }
    QTabBar::tab:selected {
        background:#0f6e56;
        color:white;
        font-weight:bold;
    }
)");

    QStringList roles = {
        "ADMIN",
        "STUDENT",
        "SECURITY",
        "TECHNICIAN",
        "AUDITOR",
        "DEPT_HEAD"
    };

    for (const QString &r : roles) {
        QTableWidget *t = new QTableWidget();
        setupUserTable(t);
        roleTables.insert(r, t);
        roleTabs->addTab(t, r);
    }

    mainLayout->addWidget(roleTabs);

    connect(addBtn,    &QPushButton::clicked, this, &UsersWindow::addUser);
    connect(toggleBtn, &QPushButton::clicked, this, &UsersWindow::toggleActive);
    connect(refreshBtn,&QPushButton::clicked, this, &UsersWindow::loadData);
    connect(searchEdit, &QLineEdit::textChanged, this, [this](const QString &text){
        QString low = text.toLower();
        for (int r = 0; r < table->rowCount(); r++) {
            bool match = table->item(r,1)->text().toLower().contains(low)
            || table->item(r,2)->text().toLower().contains(low)
                || table->item(r,3)->text().toLower().contains(low);
            table->setRowHidden(r, !match);
        }
    });
}

void UsersWindow::loadData()
{
    for (QTableWidget *t : roleTables) {
        t->setRowCount(0);
    }

    QSqlQuery q(QSqlDatabase::database());

    bool ok = q.exec(R"(
        SELECT USER_ID, USERNAME, FULL_NAME, EMAIL, ROLE, DEPARTMENT, IS_ACTIVE
        FROM   USERS
        ORDER  BY ROLE, USERNAME
    )");

    if (!ok) {
        qDebug() << "Users load error:" << q.lastError().text();
        QMessageBox::critical(this, "DB Error", q.lastError().text());
        return;
    }

    while (q.next()) {
        QString role = q.value(4).toString();

        if (!roleTables.contains(role)) {
            role = "STUDENT";
        }

        QTableWidget *t = roleTables.value(role);

        int row = t->rowCount();
        t->insertRow(row);

        for (int col = 0; col < 7; col++) {
            QString val = q.value(col).toString();

            if (col == 6) {
                val = (val == "Y") ? "✔ Active" : "✘ Inactive";
            }

            QTableWidgetItem *item = new QTableWidgetItem(val);
            item->setTextAlignment(Qt::AlignCenter);

            if (col == 6) {
                item->setForeground(val.startsWith("✔")
                                        ? QColor("#a6e3a1")
                                        : QColor("#f38ba8"));
            }

            if (col == 4) {
                if      (val == "ADMIN")      item->setForeground(QColor("#cba6f7"));
                else if (val == "TECHNICIAN") item->setForeground(QColor("#fab387"));
                else if (val == "STUDENT")    item->setForeground(QColor("#89b4fa"));
                else if (val == "AUDITOR")    item->setForeground(QColor("#a6e3a1"));
                else if (val == "SECURITY")   item->setForeground(QColor("#f38ba8"));
            }

            t->setItem(row, col, item);
        }
    }
}
void UsersWindow::addUser()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Add New User");
    dlg.setStyleSheet("background:#1e1e2e; color:#cdd6f4;");
    dlg.setMinimumWidth(440);

    auto *form = new QFormLayout(&dlg);
    form->setSpacing(12);
    form->setContentsMargins(20,20,20,20);

    QString s = "background:#313244; color:#cdd6f4; border:1px solid #45475a; border-radius:6px; padding:6px;";

    auto *usernameEdit  = new QLineEdit();
    usernameEdit->setStyleSheet(s);

    auto *fullNameEdit  = new QLineEdit();
    fullNameEdit->setStyleSheet(s);

    auto *emailEdit     = new QLineEdit();
    emailEdit->setStyleSheet(s);

    auto *phoneEdit     = new QLineEdit();
    phoneEdit->setStyleSheet(s);

    auto *deptEdit      = new QLineEdit();
    deptEdit->setStyleSheet(s);

    auto *studentIdEdit = new QLineEdit();
    studentIdEdit->setStyleSheet(s);

    QRegularExpressionValidator *userVal =
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z0-9_]{4,20}$"),
            this
            );

    usernameEdit->setValidator(userVal);

    QRegularExpressionValidator *nameVal =
        new QRegularExpressionValidator(
            QRegularExpression("^[A-Za-z ]{3,50}$"),
            this
            );

    fullNameEdit->setValidator(nameVal);

    QRegularExpressionValidator *phoneVal =
        new QRegularExpressionValidator(
            QRegularExpression("^[0-9+]{9,15}$"),
            this
            );

    phoneEdit->setValidator(phoneVal);

    QRegularExpression emailRegex(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)"
        );

    auto *passwordEdit  = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet(s);

    auto *roleCombo = new QComboBox();
    roleCombo->setStyleSheet(s);

    roleCombo->addItems({
        "STUDENT",
        "TECHNICIAN",
        "AUDITOR",
        "SECURITY",
        "DEPT_HEAD",
        "ADMIN"
    });

    auto *studentIdLabel = new QLabel("Student ID:");

    connect(roleCombo, &QComboBox::currentTextChanged,
            [=](const QString &r)
            {
                bool isStudent = (r == "STUDENT");

                studentIdEdit->setVisible(isStudent);
                studentIdLabel->setVisible(isStudent);
            });

    form->addRow("Username *:",  usernameEdit);
    form->addRow("Full Name *:", fullNameEdit);
    form->addRow("Password *:",  passwordEdit);
    form->addRow("Role:",        roleCombo);
    form->addRow("Email:",       emailEdit);
    form->addRow("Phone:",       phoneEdit);
    form->addRow("Department:",  deptEdit);
    form->addRow(studentIdLabel, studentIdEdit);

    auto *btns =
        new QDialogButtonBox(
            QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel
            );

    btns->setStyleSheet(
        "QPushButton{"
        "background:#313244;"
        "color:#cdd6f4;"
        "border-radius:6px;"
        "padding:6px 14px;"
        "}"
        );

    form->addRow(btns);

    connect(btns,
            &QDialogButtonBox::accepted,
            &dlg,
            &QDialog::accept);

    connect(btns,
            &QDialogButtonBox::rejected,
            &dlg,
            &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return;

    QString username = usernameEdit->text().trimmed();
    QString fullName = fullNameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString email    = emailEdit->text().trimmed();
    QString phone    = phoneEdit->text().trimmed();

    QRegularExpression phoneRegex(
        R"(^[0-9+ ]{9,15}$)"
        );

    if (username.isEmpty() ||
        fullName.isEmpty() ||
        password.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Validation Error",
            "Username, full name and password are required."
            );
        return;
    }

    if (password.length() < 8)
    {
        QMessageBox::warning(
            this,
            "Validation Error",
            "Password must be at least 8 characters."
            );
        return;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;

    for (QChar ch : password)
    {
        if (ch.isUpper()) hasUpper = true;
        if (ch.isLower()) hasLower = true;
        if (ch.isDigit()) hasDigit = true;
    }

    if (!hasUpper || !hasLower || !hasDigit)
    {
        QMessageBox::warning(
            this,
            "Validation Error",
            "Password must contain uppercase, lowercase and number."
            );
        return;
    }

    if (!email.isEmpty() &&
        !emailRegex.match(email).hasMatch())
    {
        QMessageBox::warning(
            this,
            "Validation Error",
            "Invalid email address."
            );
        return;
    }

    if (!phone.isEmpty() &&
        !phoneRegex.match(phone).hasMatch())
    {
        QMessageBox::warning(
            this,
            "Validation Error",
            "Invalid phone number."
            );
        return;
    }

    QString hashedPw = QCryptographicHash::hash(
                           password.toUtf8(),
                           QCryptographicHash::Sha256
                           ).toHex();

    QSqlQuery q(QSqlDatabase::database());
    q.prepare(R"(
        INSERT INTO USERS
            (USER_ID, USERNAME, FULL_NAME, PASSWORD_HASH, ROLE,
             EMAIL, PHONE, DEPARTMENT, STUDENT_ID, IS_ACTIVE, CREATED_AT)
        VALUES
            (SEQ_USERS.NEXTVAL, :username, :fullname, :pw, :role,
             :email, :phone, :dept, :stuid, 'Y', SYSDATE)
    )");

    q.bindValue(":username", username);
    q.bindValue(":fullname", fullName);
    q.bindValue(":pw",       hashedPw);
    q.bindValue(":role",     roleCombo->currentText());
    q.bindValue(":email",    email);
    q.bindValue(":phone",    phone);
    q.bindValue(":dept",     deptEdit->text().trimmed());
    q.bindValue(":stuid",    studentIdEdit->text().trimmed());

    if (q.exec()) {
        QMessageBox::information(this, "Success", "User added successfully.");
        loadData();
    } else {
        qDebug() << "Users insert error:" << q.lastError().text();
        QMessageBox::critical(this, "Error", q.lastError().text());
    }
}
void UsersWindow::toggleActive()
{
    if (!table) return;
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Select Row", "Please select a user to toggle.");
        return;
    }

    int uid = table->item(row, 0)->text().toInt();
    if (uid == m_userId) {
        QMessageBox::warning(this, "Error", "You cannot deactivate your own account.");
        return;
    }

    QString activeText = table->item(row, 6)->text();
    QString newActive  = activeText.startsWith("✔") ? "N" : "Y";
    QString action     = (newActive == "Y") ? "activate" : "deactivate";

    auto reply = QMessageBox::question(this, "Toggle User",
                                       QString("Are you sure you want to %1 this user?").arg(action),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QSqlQuery q(QSqlDatabase::database());
    q.prepare("UPDATE USERS SET IS_ACTIVE = :active WHERE USER_ID = :uid");
    q.bindValue(":active", newActive);
    q.bindValue(":uid",    uid);

    if (q.exec()) {
        loadData();
    } else {
        qDebug() << "Users toggle error:" << q.lastError().text();
        QMessageBox::critical(this, "Error", q.lastError().text());
    }
}

void UsersWindow::styleButton(QPushButton *btn, const QString &color)
{
    btn->setStyleSheet(QString(
                           "QPushButton{background:%1;color:#1e1e2e;border-radius:6px;padding:7px 16px;font-weight:bold;}"
                           ).arg(color));
}
void UsersWindow::setupUserTable(QTableWidget *t)
{
    t->setColumnCount(7);
    t->setHorizontalHeaderLabels({
        "ID", "Username", "Full Name", "Email", "Role", "Department", "Active"
    });

    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setAlternatingRowColors(true);

    t->setStyleSheet(R"(
        QTableWidget { background:#181825; color:#cdd6f4; border:none; gridline-color:#313244; }
        QTableWidget::item:selected { background:#45475a; }
        QHeaderView::section { background:#313244; color:#cba6f7; padding:8px; border:none; font-weight:bold; }
        QTableWidget { alternate-background-color:#1e1e2e; }
    )");
}