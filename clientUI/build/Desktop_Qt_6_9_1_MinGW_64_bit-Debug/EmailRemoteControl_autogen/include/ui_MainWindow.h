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
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
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
    QPushButton *FileTransfer;
    QPushButton *StartKeyLogger;
    QPushButton *ScreenshotWebshot;
    QPushButton *MacAddress;
    QPushButton *DirectoryTree;
    QPushButton *StartApp;
    QPushButton *ListApp;
    QPushButton *ListProce;
    QPushButton *StopAppPro;
    QPushButton *ShutdownLock;
    QSpacerItem *verticalSpacer;
    QFrame *mainFrame;
    QFormLayout *formLayout;
    QLabel *IPText;
    QLineEdit *IPLine;
    QHBoxLayout *connectButtonsLayout;
    QPushButton *Connect;
    QPushButton *Disconnect;
    QPushButton *Clear;
    QHBoxLayout *sendLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *Download;
    QStackedWidget *stackedWidget;
    QTextEdit *ContentText;
    QWidget *imagePage;
    QVBoxLayout *verticalLayout_2;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout;
    QLabel *imageLabel;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(567, 436);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        sidebar = new QFrame(centralwidget);
        sidebar->setObjectName("sidebar");
        sidebar->setFrameShape(QFrame::Shape::StyledPanel);
        sidebarLayout = new QVBoxLayout(sidebar);
        sidebarLayout->setObjectName("sidebarLayout");
        FileTransfer = new QPushButton(sidebar);
        FileTransfer->setObjectName("FileTransfer");

        sidebarLayout->addWidget(FileTransfer);

        StartKeyLogger = new QPushButton(sidebar);
        StartKeyLogger->setObjectName("StartKeyLogger");

        sidebarLayout->addWidget(StartKeyLogger);

        ScreenshotWebshot = new QPushButton(sidebar);
        ScreenshotWebshot->setObjectName("ScreenshotWebshot");

        sidebarLayout->addWidget(ScreenshotWebshot);

        MacAddress = new QPushButton(sidebar);
        MacAddress->setObjectName("MacAddress");

        sidebarLayout->addWidget(MacAddress);

        DirectoryTree = new QPushButton(sidebar);
        DirectoryTree->setObjectName("DirectoryTree");

        sidebarLayout->addWidget(DirectoryTree);

        StartApp = new QPushButton(sidebar);
        StartApp->setObjectName("StartApp");

        sidebarLayout->addWidget(StartApp);

        ListApp = new QPushButton(sidebar);
        ListApp->setObjectName("ListApp");

        sidebarLayout->addWidget(ListApp);

        ListProce = new QPushButton(sidebar);
        ListProce->setObjectName("ListProce");

        sidebarLayout->addWidget(ListProce);

        StopAppPro = new QPushButton(sidebar);
        StopAppPro->setObjectName("StopAppPro");

        sidebarLayout->addWidget(StopAppPro);

        ShutdownLock = new QPushButton(sidebar);
        ShutdownLock->setObjectName("ShutdownLock");

        sidebarLayout->addWidget(ShutdownLock);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        sidebarLayout->addItem(verticalSpacer);


        horizontalLayout->addWidget(sidebar);

        mainFrame = new QFrame(centralwidget);
        mainFrame->setObjectName("mainFrame");
        sizePolicy.setHeightForWidth(mainFrame->sizePolicy().hasHeightForWidth());
        mainFrame->setSizePolicy(sizePolicy);
        formLayout = new QFormLayout(mainFrame);
        formLayout->setObjectName("formLayout");
        IPText = new QLabel(mainFrame);
        IPText->setObjectName("IPText");

        formLayout->setWidget(0, QFormLayout::ItemRole::LabelRole, IPText);

        IPLine = new QLineEdit(mainFrame);
        IPLine->setObjectName("IPLine");

        formLayout->setWidget(0, QFormLayout::ItemRole::FieldRole, IPLine);

        connectButtonsLayout = new QHBoxLayout();
        connectButtonsLayout->setObjectName("connectButtonsLayout");
        Connect = new QPushButton(mainFrame);
        Connect->setObjectName("Connect");

        connectButtonsLayout->addWidget(Connect);

        Disconnect = new QPushButton(mainFrame);
        Disconnect->setObjectName("Disconnect");

        connectButtonsLayout->addWidget(Disconnect);


        formLayout->setLayout(1, QFormLayout::ItemRole::SpanningRole, connectButtonsLayout);

        Clear = new QPushButton(mainFrame);
        Clear->setObjectName("Clear");

        formLayout->setWidget(3, QFormLayout::ItemRole::LabelRole, Clear);

        sendLayout = new QHBoxLayout();
        sendLayout->setObjectName("sendLayout");
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        sendLayout->addItem(horizontalSpacer);

        Download = new QPushButton(mainFrame);
        Download->setObjectName("Download");

        sendLayout->addWidget(Download);


        formLayout->setLayout(3, QFormLayout::ItemRole::FieldRole, sendLayout);

        stackedWidget = new QStackedWidget(mainFrame);
        stackedWidget->setObjectName("stackedWidget");
        sizePolicy.setHeightForWidth(stackedWidget->sizePolicy().hasHeightForWidth());
        stackedWidget->setSizePolicy(sizePolicy);
        ContentText = new QTextEdit();
        ContentText->setObjectName("ContentText");
        ContentText->setReadOnly(true);
        stackedWidget->addWidget(ContentText);
        imagePage = new QWidget();
        imagePage->setObjectName("imagePage");
        sizePolicy.setHeightForWidth(imagePage->sizePolicy().hasHeightForWidth());
        imagePage->setSizePolicy(sizePolicy);
        verticalLayout_2 = new QVBoxLayout(imagePage);
        verticalLayout_2->setObjectName("verticalLayout_2");
        scrollArea = new QScrollArea(imagePage);
        scrollArea->setObjectName("scrollArea");
        sizePolicy.setHeightForWidth(scrollArea->sizePolicy().hasHeightForWidth());
        scrollArea->setSizePolicy(sizePolicy);
        scrollArea->setWidgetResizable(false);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 0, 0));
        verticalLayout = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout->setObjectName("verticalLayout");
        imageLabel = new QLabel(scrollAreaWidgetContents);
        imageLabel->setObjectName("imageLabel");
        imageLabel->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(imageLabel->sizePolicy().hasHeightForWidth());
        imageLabel->setSizePolicy(sizePolicy1);
        imageLabel->setScaledContents(false);
        imageLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(imageLabel);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_2->addWidget(scrollArea);

        stackedWidget->addWidget(imagePage);

        formLayout->setWidget(2, QFormLayout::ItemRole::SpanningRole, stackedWidget);


        horizontalLayout->addWidget(mainFrame);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);

        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Email Remote Control", nullptr));
        FileTransfer->setText(QCoreApplication::translate("MainWindow", "File Transfer", nullptr));
        StartKeyLogger->setText(QCoreApplication::translate("MainWindow", "Start KeyLogger", nullptr));
        ScreenshotWebshot->setText(QCoreApplication::translate("MainWindow", "Screenshot/Webcam", nullptr));
        MacAddress->setText(QCoreApplication::translate("MainWindow", "MAC address", nullptr));
        DirectoryTree->setText(QCoreApplication::translate("MainWindow", "Directory tree", nullptr));
        StartApp->setText(QCoreApplication::translate("MainWindow", "Start App", nullptr));
        ListApp->setText(QCoreApplication::translate("MainWindow", "List Apps", nullptr));
        ListProce->setText(QCoreApplication::translate("MainWindow", "List Processes", nullptr));
        StopAppPro->setText(QCoreApplication::translate("MainWindow", "Stop App/Process", nullptr));
        ShutdownLock->setText(QCoreApplication::translate("MainWindow", "Shutdown/Lock", nullptr));
        IPText->setText(QCoreApplication::translate("MainWindow", "IP:", nullptr));
        Connect->setText(QCoreApplication::translate("MainWindow", "Connect", nullptr));
        Disconnect->setText(QCoreApplication::translate("MainWindow", "Disconnect", nullptr));
        Clear->setText(QCoreApplication::translate("MainWindow", "Clear", nullptr));
        Download->setText(QCoreApplication::translate("MainWindow", "Download", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
