#include "campusgraphwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QString>

CampusGraphWindow::CampusGraphWindow(QWidget *parent) : QWidget(parent)
{
    m_dsa.graphInit();
    setupUI();
}

void CampusGraphWindow::setupUI()
{
    setStyleSheet(R"(
        QWidget        { background:#1e1e2e; color:#cdd6f4; font-family:Segoe UI; font-size:13px; }
        QGroupBox      { border:1px solid #45475a; border-radius:8px; margin-top:10px;
                         color:#cba6f7; font-weight:bold; padding:10px; }
        QGroupBox::title { subcontrol-origin:margin; left:10px; }
        QComboBox      { background:#313244; color:#cdd6f4; border:1px solid #45475a;
                         border-radius:6px; padding:6px 10px; }
        QPushButton    { border:none; border-radius:8px; padding:9px 20px;
                         font-weight:bold; font-size:13px; }
        QTextEdit      { background:#181825; color:#cdd6f4; border:1px solid #313244;
                         border-radius:6px; font-family:Consolas,monospace; font-size:12px; }
        QLabel#algo    { background:#181825; color:#89dceb; font-size:12px;
                         padding:6px 12px; border-radius:6px; border:1px solid #313244; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(14);

    // Title
    auto *title = new QLabel("🗺  Campus Building Graph — BFS & DFS (Ch. 7)");
    title->setStyleSheet("font-size:20px; font-weight:bold; color:#cba6f7;");
    root->addWidget(title);

    // Algorithm info bar
    algoLabel = new QLabel(
        "Graph: 10 nodes (buildings), 12 edges (paths) — undirected, unweighted  |"
        "  BFS = shortest path O(V+E)   DFS = full traversal O(V+E)");
    algoLabel->setObjectName("algo");
    algoLabel->setWordWrap(true);
    root->addWidget(algoLabel);

    auto *mid = new QHBoxLayout();
    mid->setSpacing(14);

    // ── Left: adjacency list display ─────────────────────
    auto *adjGroup = new QGroupBox("Adjacency List Representation");
    auto *adjLayout = new QVBoxLayout(adjGroup);
    adjText = new QTextEdit();
    adjText->setReadOnly(true);
    adjText->setFixedWidth(310);
    adjText->setText(QString::fromStdString(m_dsa.graphAdjList()));
    adjLayout->addWidget(adjText);
    mid->addWidget(adjGroup);

    // ── Right: BFS + DFS controls ────────────────────────
    auto *rightCol = new QVBoxLayout();
    rightCol->setSpacing(12);

    // BFS group
    auto *bfsGroup = new QGroupBox("BFS — Shortest Path between two buildings");
    auto *bfsLayout = new QVBoxLayout(bfsGroup);

    auto *bfsRow = new QHBoxLayout();
    startCombo = new QComboBox();
    endCombo   = new QComboBox();
    for (const std::string &n : m_dsa.graphNodes()) {
        startCombo->addItem(QString::fromStdString(n));
        endCombo->addItem(QString::fromStdString(n));
    }
    endCombo->setCurrentIndex(endCombo->count() > 5 ? 5 : endCombo->count() - 1);

    bfsBtn = new QPushButton("▶  Run BFS");
    bfsBtn->setStyleSheet("background:#89b4fa; color:#1e1e2e;");

    bfsRow->addWidget(new QLabel("From:"));
    bfsRow->addWidget(startCombo, 1);
    bfsRow->addWidget(new QLabel("To:"));
    bfsRow->addWidget(endCombo, 1);
    bfsRow->addWidget(bfsBtn);
    bfsLayout->addLayout(bfsRow);
    rightCol->addWidget(bfsGroup);

    // DFS group
    auto *dfsGroup = new QGroupBox("DFS — Full Traversal from a building");
    auto *dfsLayout = new QVBoxLayout(dfsGroup);

    auto *dfsRow = new QHBoxLayout();
    dfsStartCombo = new QComboBox();
    for (const std::string &n : m_dsa.graphNodes())
        dfsStartCombo->addItem(QString::fromStdString(n));

    dfsBtn = new QPushButton("▶  Run DFS");
    dfsBtn->setStyleSheet("background:#a6e3a1; color:#1e1e2e;");

    dfsRow->addWidget(new QLabel("Start from:"));
    dfsRow->addWidget(dfsStartCombo, 1);
    dfsRow->addWidget(dfsBtn);
    dfsLayout->addLayout(dfsRow);
    rightCol->addWidget(dfsGroup);

    // Result area
    auto *resGroup = new QGroupBox("Output");
    auto *resLayout = new QVBoxLayout(resGroup);
    resultText = new QTextEdit();
    resultText->setReadOnly(true);
    resultText->setPlainText("Run BFS or DFS to see results here.");
    resLayout->addWidget(resultText);
    rightCol->addWidget(resGroup, 1);

    mid->addLayout(rightCol, 1);
    root->addLayout(mid, 1);

    connect(bfsBtn, &QPushButton::clicked, this, &CampusGraphWindow::runBFS);
    connect(dfsBtn, &QPushButton::clicked, this, &CampusGraphWindow::runDFS);
}

void CampusGraphWindow::runBFS()
{
    std::string start = startCombo->currentText().toStdString();
    std::string end   = endCombo->currentText().toStdString();

    if (start == end) {
        resultText->setPlainText("Start and end are the same building.");
        return;
    }

    std::vector<std::string> path = m_dsa.graphBFS(start, end);

    QString out;
    out += "═══ BFS: Shortest Path ═══\n";
    out += QString("From: %1\nTo:   %2\n\n").arg(QString::fromStdString(start),
                                                  QString::fromStdString(end));
    if (path.empty()) {
        out += "❌ No path found.\n";
    } else {
        out += QString("✅ Path found (%1 hops):\n\n").arg(path.size() - 1);
        for (int i = 0; i < (int)path.size(); i++) {
            if (i) out += "  →  ";
            out += QString::fromStdString(path[i]);
        }
        out += "\n\n";
        out += "How BFS works:\n";
        out += "  1. Enqueue start node, mark visited\n";
        out += "  2. Dequeue node, check if it's the target\n";
        out += "  3. Enqueue all unvisited neighbours\n";
        out += "  4. Repeat until target found or queue empty\n";
        out += QString("\nComplexity: O(V + E) = O(%1 + 12)\n").arg(m_dsa.graphNodes().size());
    }

    resultText->setPlainText(out);
    algoLabel->setText(
        QString("BFS result: shortest path from '%1' to '%2' = %3 hops | O(V+E)")
            .arg(QString::fromStdString(start),
                 QString::fromStdString(end))
            .arg(path.empty() ? 0 : (int)path.size() - 1));
}

void CampusGraphWindow::runDFS()
{
    std::string start = dfsStartCombo->currentText().toStdString();

    std::vector<std::string> order = m_dsa.graphDFS(start);

    QString out;
    out += "═══ DFS: Full Traversal ═══\n";
    out += QString("Starting from: %1\n\n").arg(QString::fromStdString(start));
    out += QString("Visited %1 buildings in order:\n\n").arg(order.size());

    for (int i = 0; i < (int)order.size(); i++) {
        out += QString("  %1. %2\n").arg(i + 1).arg(QString::fromStdString(order[i]));
    }

    out += "\nHow DFS works:\n";
    out += "  1. Visit current node, mark visited\n";
    out += "  2. Recursively visit each unvisited neighbour\n";
    out += "  3. Backtrack when no unvisited neighbours remain\n";
    out += QString("\nComplexity: O(V + E) = O(%1 + 12)\n").arg(m_dsa.graphNodes().size());

    resultText->setPlainText(out);
    algoLabel->setText(
        QString("DFS from '%1': visited %2/%3 buildings | O(V+E)")
            .arg(QString::fromStdString(start))
            .arg(order.size())
            .arg(m_dsa.graphNodes().size()));
}
