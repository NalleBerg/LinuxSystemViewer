#ifndef NETWORK_GEEK_H
#define NETWORK_GEEK_H

#include <QDialog>
#include <QTextEdit>
#include <QTimer>

class NetworkGeekDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NetworkGeekDialog(QWidget* parent = nullptr);

private slots:
    void refresh();
    void copyToClipboard();
    void saveToFile();

private:
    void fillText();
    QTextEdit* te;
    QTimer* timer;
};

#endif // NETWORK_GEEK_H
