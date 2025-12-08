#ifndef LOAD_ANI_H
#define LOAD_ANI_H

#include <QDialog>
#include <QString>

class LoadingDialog
{
public:
    static QDialog* show(QWidget* parent, const QString& message);
    static void hide(QDialog* dialog);
};

#endif // LOAD_ANI_H