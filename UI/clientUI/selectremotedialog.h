#pragma once

#include <QDialog>
#include <QPushButton>
#include <QHBoxLayout>

class SelectRemoteDialog : public QDialog
{
    Q_OBJECT

public:
    enum RemoteType { None, IP, Gmail };
    RemoteType selectedType = None;

    explicit SelectRemoteDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Chọn loại Remote");
        resize(400, 180);

        auto *layout = new QHBoxLayout(this);

        QPushButton *ipButton = new QPushButton("IP Remote", this);
        QPushButton *gmailButton = new QPushButton("Gmail Remote", this);

        ipButton->setMinimumSize(140, 60);
        gmailButton->setMinimumSize(140, 60);

        QFont btnFont;
        btnFont.setPointSize(14);
        ipButton->setFont(btnFont);
        gmailButton->setFont(btnFont);

        layout->addWidget(ipButton);
        layout->addWidget(gmailButton);

        connect(ipButton, &QPushButton::clicked, this, [=](){
            selectedType = IP;
            accept();
        });
        connect(gmailButton, &QPushButton::clicked, this, [=](){
            selectedType = Gmail;
            accept();
        });
    }
};
