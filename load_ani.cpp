#include "load_ani.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>

QDialog* LoadingDialog::show(QWidget* parent, const QString& message)
{
    // TEST: Try different types of Qt dialogs to see if ANY work
    // Try QFileDialog first - this is a completely different dialog class
    static bool tested = false;
    if (!tested) {
        tested = true;
        // This should popup a file dialog - what color is it?
        QFileDialog::getOpenFileName(nullptr, "TEST - What color is this dialog?", "", "All Files (*)");
    }
    
    // If that doesn't work, try QColorDialog
    QColorDialog::getColor(Qt::white, nullptr, "TEST - What color is this color picker?");
    
    // Return a dummy dialog (this will be black as we've established)
    QDialog* dialog = new QDialog();
    dialog->setWindowTitle("This will be black");
    dialog->resize(300, 100);
    dialog->show();
    QApplication::processEvents();
    return dialog;
}

void LoadingDialog::hide(QDialog* dialog)
{
    if (dialog) {
        dialog->close();
        delete dialog;
    }
}