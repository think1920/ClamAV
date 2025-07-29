#include "MainWindow.h"
#include <QApplication>
#include "selectremotedialog.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SelectRemoteDialog dialog;
    SelectRemoteDialog::RemoteType chosenType = SelectRemoteDialog::None;
    if (dialog.exec() == QDialog::Accepted)
        chosenType = dialog.selectedType;
    else
        return 0;

    MainWindow w(chosenType);
    w.show();
    return a.exec();
}


