#include "network_geek.h"
#include "network.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QProcess>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

NetworkGeekDialog::NetworkGeekDialog(QWidget* parent)
    : QDialog(parent), te(new QTextEdit(this)), timer(new QTimer(this))
{
    setWindowTitle(QCoreApplication::translate("NetworkGeekDialog", "Network - Geek Mode"));
    resize(800, 480);

    te->setReadOnly(true);

    QVBoxLayout* ml = new QVBoxLayout(this);
    ml->addWidget(te);

    QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Close, this);

    QPushButton* copyBtn = new QPushButton(QCoreApplication::translate("NetworkGeekDialog", "Copy"));
    QPushButton* saveBtn = new QPushButton(QCoreApplication::translate("NetworkGeekDialog", "Save"));
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
    QString fn = QFileDialog::getSaveFileName(this, QCoreApplication::translate("NetworkGeekDialog", "Save network info"), "network-info.csv", QCoreApplication::translate("NetworkGeekDialog", "CSV files (*.csv);;All files (*)"));
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    QTextStream ts(&f);

    auto esc = [](const QString &s)->QString{
        QString out = s;
        out.replace('"', "\"");
        if (out.contains(',') || out.contains('\n') || out.contains('"')) out = '"' + out + '"';
        return out;
    };

    // Split into sections by blank line; use first line as section title
    const QString content = te->toPlainText();
    QStringList parts = content.split("\n\n", Qt::SkipEmptyParts);

    ts << "Section,Text\n";
    if (parts.isEmpty()) {
        ts << esc(QString("All")) << ',' << esc(content) << '\n';
    } else {
        for (const QString &p : parts) {
            QString title;
            QString body;
            int nl = p.indexOf('\n');
            if (nl >= 0) {
                title = p.left(nl).trimmed();
                body = p.mid(nl+1).trimmed();
            } else {
                title = p.trimmed();
                body.clear();
            }
            ts << esc(title.isEmpty() ? QString("Section") : title) << ',' << esc(body.isEmpty() ? title : body) << '\n';
        }
    }
    f.close();
}
