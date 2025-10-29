#include "network_geek.h"
#include "network.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QProcess>
#include <QGuiApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

NetworkGeekDialog::NetworkGeekDialog(QWidget* parent)
    : QDialog(parent), te(new QTextEdit(this)), timer(new QTimer(this))
{
    setWindowTitle("Network - Geek Mode");
    resize(800, 480);

    te->setReadOnly(true);

    QVBoxLayout* ml = new QVBoxLayout(this);
    ml->addWidget(te);

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Close, this);

    QPushButton* copyBtn = new QPushButton("Copy");
    QPushButton* saveBtn = new QPushButton("Save");
    box->addButton(copyBtn, QDialogButtonBox::ActionRole);
    box->addButton(saveBtn, QDialogButtonBox::ActionRole);

    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(copyBtn, &QPushButton::clicked, this, &NetworkGeekDialog::copyToClipboard);
    connect(saveBtn, &QPushButton::clicked, this, &NetworkGeekDialog::saveToFile);

    ml->addWidget(box);

    connect(timer, &QTimer::timeout, this, &NetworkGeekDialog::refresh);
    timer->start(3000); // refresh every 3s

    refresh();
}

void NetworkGeekDialog::fillText()
{
    // Prefer structured helper output and then append raw commands
    QString out = getNetworkInfo();

    QProcess p;
    p.start("sh", QStringList() << "-c" << "ip addr && echo --- && ip route && echo --- && cat /proc/net/dev && echo --- && cat /proc/net/route");
    p.waitForFinished(3000);
    out += "\n\n" + QString::fromLocal8Bit(p.readAllStandardOutput());
    out += "\n" + QString::fromLocal8Bit(p.readAllStandardError());

    te->setPlainText(out);
}

void NetworkGeekDialog::refresh()
{
    fillText();
}

void NetworkGeekDialog::copyToClipboard()
{
    QClipboard* cb = QGuiApplication::clipboard();
    cb->setText(te->toPlainText());
}

void NetworkGeekDialog::saveToFile()
{
    QString fn = QFileDialog::getSaveFileName(this, "Save network info", QString(), "Text files (*.txt);;All files (*)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QTextStream ts(&f);
    ts << te->toPlainText();
    f.close();
}
