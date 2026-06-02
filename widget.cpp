#include "widget.h"
#include "ui_widget.h"
#include "logindialog.h"
#include "assetwindow.h"
#include "maintenancewindow.h"
#include "lostfoundwindow.h"
#include "studentitemswindow.h"
#include "userswindow.h"
#include "securitycheckwindow.h"
#include "campusgraphwindow.h"
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
// ── Role permissions ─────────────────────────────────────
struct RolePolicy {
    bool canViewAllAssets;
    bool canAddAsset;
    bool canDeleteAsset;
    bool canViewMaintenance;
    bool canReportIssue;
    bool canResolveMaintenance;
    bool canViewLostFound;
    bool canViewAllStudentItems;
    bool canManageUsers;

    static RolePolicy forRole(const QString &role) {
        if (role == "ADMIN") {
            return {true, true, true, true, true, true, true, true, true};
        }

        if (role == "TECHNICIAN") {
            return {true, false, false, true, true, true, true, true, false};
        }

        if (role == "AUDITOR") {
            return {true, false, false, true, false, false, true, true, false};
        }

        if (role == "SECURITY") {
            return {false, false, false, false, false, false, true, true, false};
        }

        // STUDENT default
        return {false, false, false, true, true, false, true, false, false};
    }
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , m_userId(-1)
{
    ui->setupUi(this);


    QSqlDatabase db;

    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        db = QSqlDatabase::database("qt_sql_default_connection");
    } else {
        db = QSqlDatabase::addDatabase("QODBC");
    }

    db.setDatabaseName(
        "Driver={Oracle in XE};"
        "DBQ=127.0.0.1:1522/xe;"
        );
    db.setUserName("campus_admin");
    db.setPassword("campus123");

    if (!db.open()) {
        QMessageBox::critical(this, "DB Error", db.lastError().text());
        return;
    }
    if (!db.open()) {
        QMessageBox::critical(this, "DB Error", db.lastError().text());
        return;
    }

    LoginDialog login;

    if (login.exec() != QDialog::Accepted) {
        QApplication::quit();
        return;
    }

    m_role = login.getRole();
    m_userId = login.getUserId();

    buildDashboard(m_role, m_userId);

}

void Widget::applyRoleAccess(const QString &role,
                             QPushButton *btnAssets,
                             QPushButton *btnMaint,
                             QPushButton *btnLostFound,
                             QPushButton *btnStudents,
                             QPushButton *btnUsers)
{
    RolePolicy p = RolePolicy::forRole(role);

    btnAssets->setVisible(p.canViewAllAssets);
    btnMaint->setVisible(p.canViewMaintenance || role == "STUDENT");
    btnLostFound->setVisible(p.canViewLostFound);
    btnStudents->setVisible(true);
    btnUsers->setVisible(p.canManageUsers);
}

void Widget::buildDashboard(const QString &role, int userId)
{
    setWindowTitle("Smart Campus Asset Manager — " + role);
    setMinimumSize(980, 660);

    setStyleSheet(R"(

/* ===== GLOBAL ===== */

QWidget {
    background-color: #11111b;
    color: #cdd6f4;
    font-family: "Segoe UI";
    font-size: 13px;
}

/* ===== SIDEBAR ===== */

QFrame#sidebar {
    background-color: #181825;
    border-right: 1px solid #313244;
}

/* ===== NAVIGATION BUTTON ===== */

QPushButton#navBtn {
    background-color: transparent;
    color: #bac2de;
    border: none;
    text-align: left;
    padding: 12px 18px;
    font-size: 13px;
    border-radius: 10px;
    font-weight: 500;
}

QPushButton#navBtn:hover {
    background-color: #313244;
    color: white;
}

QPushButton#navBtnActive {
    background-color: #0f6e56;
    color: white;
    border: none;
    text-align: left;
    padding: 12px 18px;
    font-size: 13px;
    border-radius: 10px;
    font-weight: bold;
}

/* ===== HOME BUTTON ===== */

QPushButton#homeBtn {
    background-color: #0f6e56;
    color: white;
    border: none;
    border-radius: 8px;
    padding: 8px 16px;
    font-size: 12px;
    font-weight: bold;
}

QPushButton#homeBtn:hover {
    background-color: #1D9E75;
}

/* ===== LOGOUT ===== */

QPushButton#logoutBtn {
    background-color: #c0392b;
    color: white;
    border: none;
    border-radius: 8px;
    padding: 8px 16px;
    font-size: 12px;
    font-weight: bold;
}

QPushButton#logoutBtn:hover {
    background-color: #e74c3c;
}

/* ===== STAT CARDS ===== */

QFrame#statCard {
    background-color: #181825;
    border-radius: 14px;
    border: 1px solid #313244;
}

QFrame#statCard:hover {
    border: 1px solid #0f6e56;
}

/* ===== STAT TEXT ===== */

QLabel#statNum {
    font-size: 30px;
    font-weight: bold;
    color: #a6e3a1;
}

QLabel#statTitle {
    font-size: 12px;
    color: #bac2de;
}

/* ===== PAGE TITLE ===== */

QLabel#pageTitle {
    font-size: 24px;
    font-weight: bold;
    color: #cba6f7;
}

/* ===== ROLE BADGE ===== */

QLabel#roleBadge {
    background-color: #0f6e56;
    color: white;
    border-radius: 12px;
    padding: 5px 12px;
    font-size: 11px;
    font-weight: bold;
}

/* ===== ACCESS CARDS ===== */

QFrame#accessCard {
    background-color: #181825;
    border-radius: 12px;
    border-left: 5px solid #0f6e56;
    padding: 10px;
}

QFrame#accessCardDenied {
    background-color: #181825;
    border-radius: 12px;
    border-left: 5px solid #c0392b;
    padding: 10px;
}

/* ===== INPUTS ===== */

QLineEdit,
QComboBox,
QTextEdit {
    background-color: #313244;
    color: #cdd6f4;
    border: 1px solid #45475a;
    border-radius: 8px;
    padding: 8px;
}

QLineEdit:focus,
QComboBox:focus,
QTextEdit:focus {
    border: 2px solid #0f6e56;
}

/* ===== TABLE ===== */

QTableWidget {
    background-color: #181825;
    color: #cdd6f4;
    border: none;
    gridline-color: #313244;
    alternate-background-color: #1e1e2e;
    selection-background-color: #45475a;
}

QTableWidget::item {
    padding: 8px;
}

QHeaderView::section {
    background-color: #313244;
    color: #cba6f7;
    padding: 10px;
    border: none;
    font-weight: bold;
}

/* ===== TREE ===== */

QTreeWidget {
    background-color: #181825;
    color: #cdd6f4;
    border: none;
    border-radius: 10px;
}

QTreeWidget::item {
    padding: 8px;
    border-radius: 6px;
}

QTreeWidget::item:hover {
    background-color: #313244;
}

QTreeWidget::item:selected {
    background-color: #0f6e56;
    color: white;
}

/* ===== TABS ===== */

QTabWidget::pane {
    border: 1px solid #313244;
    background: #181825;
    border-radius: 10px;
}

QTabBar::tab {
    background: #313244;
    color: #cdd6f4;
    padding: 10px 18px;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
}

QTabBar::tab:selected {
    background: #0f6e56;
    color: white;
    font-weight: bold;
}

/* ===== SCROLLBAR ===== */

QScrollBar:vertical {
    background: #11111b;
    width: 10px;
}

QScrollBar::handle:vertical {
    background: #45475a;
    border-radius: 5px;
}

QScrollBar::handle:vertical:hover {
    background: #0f6e56;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0px;
}

/* ===== TOOLTIP ===== */

QToolTip {
    background-color: #313244;
    color: white;
    border: 1px solid #0f6e56;
    padding: 6px;
}

)");

    // Build inside a container so doLogout can delete and recreate cleanly
    m_dashContainer = new QWidget(this);

    // Ensure the Widget itself has exactly one permanent layout
    if (!layout()) {
        QHBoxLayout *permanent = new QHBoxLayout(this);
        permanent->setContentsMargins(0, 0, 0, 0);
        permanent->setSpacing(0);
    }
    static_cast<QHBoxLayout *>(layout())->addWidget(m_dashContainer);

    QHBoxLayout *rootLayout = new QHBoxLayout(m_dashContainer);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── SIDEBAR ──────────────────────────────────────────
    // ── SIDEBAR ──────────────────────────────────────────
    QFrame *sidebar = new QFrame(m_dashContainer);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(270);

    QVBoxLayout *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(10, 20, 10, 20);
    sideLayout->setSpacing(8);

    // Logo box
QFrame *logoBox = new QFrame(sidebar);


logoBox->setStyleSheet(R"(
    QFrame {
        background-color: #0f1020;
        border: 1px solid #313244;
        border-radius: 22px;
    }
)");

logoBox->setFixedHeight(190);

QVBoxLayout *logoLayout = new QVBoxLayout(logoBox);
logoLayout->setContentsMargins(15, 15, 15, 15);
logoLayout->setSpacing(0);

QLabel *logoLabel = new QLabel(logoBox);

QPixmap logo(":/dbu_logo.png");

if (logo.isNull()) {

    logoLabel->setText("LOGO");

    logoLabel->setStyleSheet(R"(
        color:white;
        font-size:24px;
        font-weight:bold;
        border:none;
        background:transparent;
    )");

} else {

    logoLabel->setPixmap(
        logo.scaled(
            210,
            210,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        )
    );
}

logoLabel->setAlignment(Qt::AlignCenter);
logoLabel->setStyleSheet(
    "background:transparent;border:none;"
);

logoLayout->addStretch();

logoLayout->addWidget(
    logoLabel,
    0,
    Qt::AlignCenter
);

logoLayout->addStretch();

sideLayout->addWidget(logoBox);

    // Navigation buttons
    QPushButton *btnDashboard = new QPushButton("  🏠  Home", sidebar);
    QPushButton *btnAssets    = new QPushButton("  🗂  Assets", sidebar);
    QPushButton *btnMaint     = new QPushButton("  🔧  Maintenance", sidebar);
    QPushButton *btnLostFound = new QPushButton("  🔍  Lost & Found", sidebar);
    QPushButton *btnStudents  = new QPushButton("  🎒  My Items", sidebar);
    QPushButton *btnUsers     = new QPushButton("  👥  Users", sidebar);
    QPushButton *btnSecurity  = new QPushButton("  🛡  Security Check", sidebar);
    QPushButton *btnGraph     = new QPushButton("  🗺  Campus Graph", sidebar);

    QList<QPushButton*> navBtns = {
        btnDashboard,
        btnAssets,
        btnMaint,
        btnLostFound,
        btnStudents,
        btnUsers,
        btnSecurity,
        btnGraph
    };

    for (QPushButton *btn : navBtns) {
        btn->setObjectName("navBtn");
        btn->setCursor(Qt::PointingHandCursor);
        sideLayout->addWidget(btn);
    }

    applyRoleAccess(role, btnAssets, btnMaint, btnLostFound, btnStudents, btnUsers);
    btnSecurity->setVisible(role == "SECURITY" || role == "ADMIN");
    btnGraph->setVisible(true);   // available to all roles — pure DSA demo

    sideLayout->addStretch();

    QLabel *roleBadge = new QLabel(role, sidebar);
    roleBadge->setObjectName("roleBadge");
    roleBadge->setAlignment(Qt::AlignCenter);
    roleBadge->setFixedHeight(24);
    sideLayout->addWidget(roleBadge);

    QLabel *userLabel = new QLabel(QString("ID: %1").arg(userId), sidebar);
    userLabel->setStyleSheet("color:#666;font-size:10px;");
    userLabel->setAlignment(Qt::AlignCenter);
    sideLayout->addWidget(userLabel);

    // ── CONTENT AREA ─────────────────────────────────────
    QWidget *content = new QWidget(m_dashContainer);
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(28, 20, 28, 28);
    contentLayout->setSpacing(16);

    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *breadcrumb = new QLabel("Home", content);
    breadcrumb->setStyleSheet("color:#888;font-size:12px;");

    QPushButton *homeBtn = new QPushButton("🏠 Home", content);
    QPushButton *logoutBtn = new QPushButton("⏏ Logout", content);
    homeBtn->setObjectName("homeBtn");
    logoutBtn->setObjectName("logoutBtn");
    homeBtn->setFixedHeight(30);
    logoutBtn->setFixedHeight(30);

    topBar->addWidget(breadcrumb, 1);
    topBar->addWidget(homeBtn);
    topBar->addWidget(logoutBtn);
    contentLayout->addLayout(topBar);

    QWidget *pageArea = new QWidget(content);
    QVBoxLayout *pageLayout = new QVBoxLayout(pageArea);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);
    contentLayout->addWidget(pageArea, 1);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(content, 1);

    // ── HELPERS ──────────────────────────────────────────
    auto setActive = [navBtns](QPushButton *active) {
        for (QPushButton *btn : navBtns) {
            btn->setObjectName(btn == active ? "navBtnActive" : "navBtn");
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
            btn->update();
        }
    };

    auto clearPage = [pageLayout]() {
        while (QLayoutItem *child = pageLayout->takeAt(0)) {
            if (QWidget *w = child->widget()) {
                w->setParent(nullptr);
                w->deleteLater();
            }
            delete child;
        }
    };

    auto getCount = [](const QString &tableName, const QString &where = QString()) -> int {
        QSqlQuery q(QSqlDatabase::database());
        QString sql = "SELECT COUNT(*) FROM " + tableName;
        if (!where.isEmpty()) {
            sql += " WHERE " + where;
        }

        if (!q.exec(sql)) {
            return 0;
        }

        return q.next() ? q.value(0).toInt() : 0;
    };

    // ── HOME PAGE ────────────────────────────────────────
    auto showHome = [=]() {
        clearPage();
        setActive(btnDashboard);
        breadcrumb->setText("Home");

        RolePolicy policy = RolePolicy::forRole(role);

        QLabel *title = new QLabel("Welcome, " + role + " 👋", pageArea);
        title->setObjectName("pageTitle");

        QLabel *subtitle = new QLabel("Smart Campus Asset Manager — Debre Berhan University", pageArea);
        subtitle->setStyleSheet("color:#888;font-size:13px;");

        QHBoxLayout *cardsRow = new QHBoxLayout();
        cardsRow->setSpacing(14);

        QList<QPair<QString, int>> stats;
        if (role == "ADMIN") {
            stats = {
                {"Total Assets",  getCount("ASSETS")},
                {"Student Items", getCount("STUDENT_ITEMS")},
                {"Maintenance",   getCount("MAINTENANCE_LOG")},
                {"Lost & Found",  getCount("LOST_FOUND")}
            };
        } else if (role == "TECHNICIAN") {
            stats = {
                {"Total Assets", getCount("ASSETS")},
                {"Pending Jobs", getCount("MAINTENANCE_LOG", "STATUS='PENDING'")},
                {"In Progress",  getCount("MAINTENANCE_LOG", "STATUS='IN_PROGRESS'")},
                {"Lost & Found", getCount("LOST_FOUND")}
            };
        } else {
            stats = {
                {"My Items",     getCount("STUDENT_ITEMS", QString("STUDENT_ID=%1").arg(userId))},
                {"Lost & Found", getCount("LOST_FOUND")},
                {"My Reports",   getCount("MAINTENANCE_LOG", QString("REPORTED_BY=%1").arg(userId))},
                {"Open Issues",  getCount("MAINTENANCE_LOG", "STATUS='PENDING'")}
            };
        }

        for (const auto &s : stats) {
            QFrame *card = new QFrame(pageArea);
            card->setObjectName("statCard");
            card->setFixedHeight(110);

            QVBoxLayout *cl = new QVBoxLayout(card);
            cl->setAlignment(Qt::AlignCenter);

            QLabel *num = new QLabel(QString::number(s.second), card);
            num->setObjectName("statNum");
            num->setAlignment(Qt::AlignCenter);

            QLabel *lbl = new QLabel(s.first, card);
            lbl->setObjectName("statTitle");
            lbl->setAlignment(Qt::AlignCenter);

            cl->addWidget(num);
            cl->addWidget(lbl);
            cardsRow->addWidget(card);
        }

        QWidget *cardsWidget = new QWidget(pageArea);
        cardsWidget->setLayout(cardsRow);

        QLabel *permTitle = new QLabel("🔐 Your Access Permissions", pageArea);
        permTitle->setStyleSheet("font-size:14px;font-weight:bold;color:#cba6f7;margin-top:8px;");

        QHBoxLayout *permsRow = new QHBoxLayout();
        permsRow->setSpacing(10);

        struct PermItem {
            QString label;
            bool allowed;
        };

        QList<PermItem> perms = {
            {"View Assets",       policy.canViewAllAssets},
            {"Add/Delete Assets", policy.canAddAsset},
            {"Maintenance",       policy.canViewMaintenance},
            {"Report Issue",      policy.canReportIssue},
            {"Resolve Issue",     policy.canResolveMaintenance},
            {"Lost & Found",      policy.canViewLostFound},
            {"Manage Users",      policy.canManageUsers}
        };

        for (const auto &p : perms) {
            QFrame *pcard = new QFrame(pageArea);
            pcard->setObjectName(p.allowed ? "accessCard" : "accessCardDenied");
            pcard->setFixedHeight(60);

            QVBoxLayout *pl = new QVBoxLayout(pcard);
            pl->setContentsMargins(10, 6, 10, 6);

            QLabel *icon = new QLabel(p.allowed ? "✔" : "✘", pcard);
            icon->setStyleSheet(
                p.allowed
                    ? "color:#0f6e56;font-size:16px;font-weight:bold;"
                    : "color:#c0392b;font-size:16px;font-weight:bold;"
                );
            icon->setAlignment(Qt::AlignCenter);

            QLabel *lbl = new QLabel(p.label, pcard);
            lbl->setStyleSheet("font-size:10px;color:#bac2de;");
            lbl->setAlignment(Qt::AlignCenter);

            pl->addWidget(icon);
            pl->addWidget(lbl);
            permsRow->addWidget(pcard);
        }

        QWidget *permsWidget = new QWidget(pageArea);
        permsWidget->setLayout(permsRow);

        pageLayout->addWidget(title);
        pageLayout->addWidget(subtitle);
        pageLayout->addWidget(cardsWidget);
        pageLayout->addWidget(permTitle);
        pageLayout->addWidget(permsWidget);
        pageLayout->addStretch();
    };

    auto showAccessDenied = [=](const QString &screen) {
        clearPage();
        breadcrumb->setText("Access Denied");

        QLabel *icon = new QLabel("🔒", pageArea);
        icon->setStyleSheet("font-size:48px;");
        icon->setAlignment(Qt::AlignCenter);

        QLabel *msg = new QLabel("Access Denied", pageArea);
        msg->setStyleSheet("font-size:22px;font-weight:bold;color:#c0392b;");
        msg->setAlignment(Qt::AlignCenter);

        QLabel *sub = new QLabel("Your role (" + role + ") does not have permission to access: " + screen, pageArea);
        sub->setStyleSheet("font-size:13px;color:#888;");
        sub->setAlignment(Qt::AlignCenter);

        pageLayout->addStretch();
        pageLayout->addWidget(icon);
        pageLayout->addWidget(msg);
        pageLayout->addWidget(sub);
        pageLayout->addStretch();
    };

    // ── BUTTON CONNECTIONS ───────────────────────────────
    connect(btnDashboard, &QPushButton::clicked, this, showHome);
    connect(homeBtn, &QPushButton::clicked, this, showHome);

    connect(logoutBtn, &QPushButton::clicked, this, &Widget::doLogout);

    connect(btnAssets, &QPushButton::clicked, this, [=]() {
        if (!RolePolicy::forRole(role).canViewAllAssets) {
            showAccessDenied("Assets");
            return;
        }

        clearPage();
        setActive(btnAssets);
        breadcrumb->setText("Home > Assets");
        pageLayout->addWidget(new AssetWindow(pageArea));
    });

    connect(btnMaint, &QPushButton::clicked, this, [=]() {
        if (!(RolePolicy::forRole(role).canViewMaintenance || role == "STUDENT")) {
            showAccessDenied("Maintenance");
            return;
        }

        clearPage();
        setActive(btnMaint);
        breadcrumb->setText("Home > Maintenance");
        pageLayout->addWidget(new MaintenanceWindow(userId, role, pageArea));
    });

    connect(btnLostFound, &QPushButton::clicked, this, [=]() {
        if (!RolePolicy::forRole(role).canViewLostFound) {
            showAccessDenied("Lost & Found");
            return;
        }

        clearPage();
        setActive(btnLostFound);
        breadcrumb->setText("Home > Lost & Found");
        pageLayout->addWidget(new LostFoundWindow(userId, role, pageArea));
    });

    connect(btnStudents, &QPushButton::clicked, this, [=]() {
        clearPage();
        setActive(btnStudents);
        breadcrumb->setText("Home > Student Items");
        pageLayout->addWidget(new StudentItemsWindow(userId, role, pageArea));
    });

    connect(btnUsers, &QPushButton::clicked, this, [=]() {
        if (!RolePolicy::forRole(role).canManageUsers) {
            showAccessDenied("User Management");
            return;
        }

        clearPage();
        setActive(btnUsers);
        breadcrumb->setText("Home > Users");
        pageLayout->addWidget(new UsersWindow(userId, role, pageArea));
    });
    connect(btnSecurity, &QPushButton::clicked, this, [=]() {
        SecurityCheckWindow *w = new SecurityCheckWindow(userId, role, this);
        w->show();
    });

    connect(btnGraph, &QPushButton::clicked, this, [=]() {
        clearPage();
        setActive(btnGraph);
        breadcrumb->setText("Home > Campus Graph (BFS / DFS)");
        pageLayout->addWidget(new CampusGraphWindow(pageArea));
    });

    showHome();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::doLogout()
{
    hide();

    // Delete the entire dashboard — container owns all sidebar/content children
    if (m_dashContainer) {
        delete m_dashContainer;
        m_dashContainer = nullptr;
    }

    // Fresh standalone login (nullptr = no parent → no dark stylesheet inherited)
    LoginDialog login(nullptr);
    if (login.exec() != QDialog::Accepted) {
        QApplication::quit();
        return;
    }

    m_role   = login.getRole();
    m_userId = login.getUserId();
    buildDashboard(m_role, m_userId);
    show();
}
