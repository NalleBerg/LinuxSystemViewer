#ifndef CTRLW_H
#define CTRLW_H

#include <QObject>
#include <QEvent>

class QMainWindow;

class CtrlWHandler : public QObject
{
    Q_OBJECT

public:
    explicit CtrlWHandler(QMainWindow* mainWindow, QObject* parent = nullptr);
    ~CtrlWHandler();

public slots:
    void showQuitDialogFromMenu();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void showQuitDialog();
    
    QMainWindow* m_mainWindow;
};

#endif // CTRLW_H