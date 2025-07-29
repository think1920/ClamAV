/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QFrame *sidebar;
    QVBoxLayout *sidebarLayout;
    QPushButton *btnKeyLogger;
    QPushButton *btnCaptureScreen;
    QPushButton *btnCaptureWebcam;
    QPushButton *btnMacAddress;
    QPushButton *btnDirectoryTree;
    QPushButton *btnAppProcess;
    QPushButton *btnRegistry;
    QPushButton *btnShutdownLogout;
    QSpacerItem *verticalSpacer;
    QFrame *mainFrame;
    QFormLayout *formLayout;
    QLabel *labelTo;
    QLineEdit *editTo;
    QLabel *labelSubject;
    QLineEdit *editSubject;
    QTextEdit *editContent;
    QPushButton *btnCancel;
    QHBoxLayout *sendLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnSend;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        sidebar = new QFrame(centralwidget);
        sidebar->setObjectName("sidebar");
        sidebar->setMinimumWidth(150);
        sidebar->setFrameShape(QFrame::StyledPanel);
        sidebarLayout = new QVBoxLayout(sidebar);
        sidebarLayout->setObjectName("sidebarLayout");
        btnKeyLogger = new QPushButton(sidebar);
        btnKeyLogger->setObjectName("btnKeyLogger");

        sidebarLayout->addWidget(btnKeyLogger);

        btnCaptureScreen = new QPushButton(sidebar);
        btnCaptureScreen->setObjectName("btnCaptureScreen");

        sidebarLayout->addWidget(btnCaptureScreen);

        btnCaptureWebcam = new QPushButton(sidebar);
        btnCaptureWebcam->setObjectName("btnCaptureWebcam");

        sidebarLayout->addWidget(btnCaptureWebcam);

        btnMacAddress = new QPushButton(sidebar);
        btnMacAddress->setObjectName("btnMacAddress");

        sidebarLayout->addWidget(btnMacAddress);

        btnDirectoryTree = new QPushButton(sidebar);
        btnDirectoryTree->setObjectName("btnDirectoryTree");

        sidebarLayout->addWidget(btnDirectoryTree);

        btnAppProcess = new QPushButton(sidebar);
        btnAppProcess->setObjectName("btnAppProcess");

        sidebarLayout->addWidget(btnAppProcess);

        btnRegistry = new QPushButton(sidebar);
        btnRegistry->setObjectName("btnRegistry");

        sidebarLayout->addWidget(btnRegistry);

        btnShutdownLogout = new QPushButton(sidebar);
        btnShutdownLogout->setObjectName("btnShutdownLogout");

        sidebarLayout->addWidget(btnShutdownLogout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        sidebarLayout->addItem(verticalSpacer);


        horizontalLayout->addWidget(sidebar);

        mainFrame = new QFrame(centralwidget);
        mainFrame->setObjectName("mainFrame");
        formLayout = new QFormLayout(mainFrame);
        formLayout->setObjectName("formLayout");
        labelTo = new QLabel(mainFrame);
        labelTo->setObjectName("labelTo");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, labelTo);

        editTo = new QLineEdit(mainFrame);
        editTo->setObjectName("editTo");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, editTo);

        labelSubject = new QLabel(mainFrame);
        labelSubject->setObjectName("labelSubject");

        formLayout->setWidget(1, QFormLayout::ItemRole::LabelRole, labelSubject);

        editSubject = new QLineEdit(mainFrame);
        editSubject->setObjectName("editSubject");

        formLayout->setWidget(1, QFormLayout::ItemRole::FieldRole, editSubject);

        editContent = new QTextEdit(mainFrame);
        editContent->setObjectName("editContent");

        formLayout->setWidget(2, QFormLayout::ItemRole::SpanningRole, editContent);

        btnCancel = new QPushButton(mainFrame);
        btnCancel->setObjectName("btnCancel");

        formLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, btnCancel);

        sendLayout = new QHBoxLayout();
        sendLayout->setObjectName("sendLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        sendLayout->addItem(horizontalSpacer);

        btnSend = new QPushButton(mainFrame);
        btnSend->setObjectName("btnSend");

        sendLayout->addWidget(btnSend);


        formLayout->setLayout(3, QFormLayout::ItemRole::FieldRole, sendLayout);


        horizontalLayout->addWidget(mainFrame);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Email Remote Control", nullptr));
        btnKeyLogger->setText(QCoreApplication::translate("MainWindow", "Key logger", nullptr));
        btnCaptureScreen->setText(QCoreApplication::translate("MainWindow", "Capture screen", nullptr));
        btnCaptureWebcam->setText(QCoreApplication::translate("MainWindow", "Capture webcam", nullptr));
        btnMacAddress->setText(QCoreApplication::translate("MainWindow", "MAC address", nullptr));
        btnDirectoryTree->setText(QCoreApplication::translate("MainWindow", "Directory tree", nullptr));
        btnAppProcess->setText(QCoreApplication::translate("MainWindow", "Application/Process", nullptr));
        btnRegistry->setText(QCoreApplication::translate("MainWindow", "Registry", nullptr));
        btnShutdownLogout->setText(QCoreApplication::translate("MainWindow", "Shutdown/Logout", nullptr));
        labelTo->setText(QCoreApplication::translate("MainWindow", "To:", nullptr));
        labelSubject->setText(QCoreApplication::translate("MainWindow", "Subject:", nullptr));
        btnCancel->setText(QCoreApplication::translate("MainWindow", "Cancel", nullptr));
        btnSend->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
