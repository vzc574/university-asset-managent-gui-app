#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QMessageBox>
#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QCheckBox>
LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent), m_userId(-1)
{
    setWindowTitle("Smart Campus Asset Manager");
    setFixedSize(420, 500);

    setStyleSheet(R"(
        QDialog {
            background-color: #f0f2f5;
        }
        QLineEdit {
            border: 1.5px solid #ddd;
            border-radius: 8px;
            padding: 10px 14px;
            font-size: 14px;
            background: #fafafa;
            color: #1a1a2e;
            min-height: 20px;
        }
        QLineEdit:focus {
            border-color: #0f6e56;
            background: white;
        }
        QPushButton#loginBtn {
            background-color: #0f6e56;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 15px;
            font-weight: bold;
            padding: 12px;
            min-height: 44px;
        }
        QPushButton#loginBtn:hover {
            background-color: #1D9E75;
        }
        QPushButton#loginBtn:pressed {
            background-color: #085041;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(12);

    // University label
    QLabel *uniLabel = new QLabel("DEBRE BERHAN UNIVERSITY");
    uniLabel->setAlignment(Qt::AlignCenter);
    uniLabel->setStyleSheet("color: #0f6e56; font-size: 11px; font-weight: bold;");

    // Title
    QLabel *titleLabel = new QLabel("Smart Campus");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #1a1a2e; font-size: 26px; font-weight: bold;");

    // Subtitle
    QLabel *subtitleLabel = new QLabel("Asset Manager");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("color: #666; font-size: 14px;");

    // Spacer
    QLabel *spacer = new QLabel("");
    spacer->setFixedHeight(10);

    // Username
    QLabel *userLabel = new QLabel("USERNAME");
    userLabel->setStyleSheet("color: #444; font-size: 11px; font-weight: bold;");
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Enter your username");

    // Password
    QLabel *passLabel = new QLabel("PASSWORD");
    passLabel->setStyleSheet("color: #444; font-size: 11px; font-weight: bold;");
    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Enter your password");
    passwordEdit->setEchoMode(QLineEdit::Password);

    // Error label
    errorLabel = new QLabel("");
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setStyleSheet("color: #c0392b; font-size: 12px;");

    // Login button
    loginBtn = new QPushButton("Login");
    loginBtn->setObjectName("loginBtn");

    mainLayout->addWidget(uniLabel);
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(spacer);
    mainLayout->addWidget(userLabel);
    mainLayout->addWidget(usernameEdit);
    mainLayout->addWidget(passLabel);
    mainLayout->addWidget(passwordEdit);
    mainLayout->addWidget(errorLabel);
    mainLayout->addWidget(loginBtn);
    QCheckBox *showPass = new QCheckBox("Show Password");
    showPass->setStyleSheet("color:#444; font-size:12px;");
    connect(showPass, &QCheckBox::toggled, this, [=](bool checked){

        passwordEdit->setEchoMode(
            checked
                ? QLineEdit::Normal
                : QLineEdit::Password
            );

    });

    mainLayout->addWidget(showPass);

    connect(loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
connect(usernameEdit, &QLineEdit::returnPressed, passwordEdit, [this](){ passwordEdit->setFocus(); });
}

void LoginDialog::onLoginClicked()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        errorLabel->setText("Please enter username and password.");
        return;
    }

    QString hashedPw = QCryptographicHash::hash(
                           password.toUtf8(),
                           QCryptographicHash::Sha256
                           ).toHex();

    QSqlQuery query(QSqlDatabase::database());
    query.prepare(R"(
        SELECT user_id, role, is_active
        FROM USERS
        WHERE username = :u
          AND (password_hash = :hashed OR password_hash = :plain)
    )");

    query.bindValue(":u", username);
    query.bindValue(":hashed", hashedPw);
    query.bindValue(":plain", password);

    if (!query.exec()) {
        errorLabel->setText("Database login error.");
        return;
    }

    if (query.next()) {
        if (query.value(2).toString() != "Y") {
            errorLabel->setText("Your account is inactive.");
            return;
        }

        m_userId = query.value(0).toInt();
        m_role   = query.value(1).toString();
        accept();
    } else {
        errorLabel->setText("Invalid username or password.");
        passwordEdit->clear();
        passwordEdit->setFocus();
    }
}