#pragma once
#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QInputDialog>
#include <QTimer>

class GmailAccountDialog : public QDialog {
    Q_OBJECT
public:
    QString selectedEmail;
    bool addNew = false;
    bool deleteSelected = false;
    void reloadList() {
        list->clear();
        QFile file("tmptokens.json");
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonArray arr = doc.array();
            for (auto v : arr)
                list->addItem(v.toObject()["email"].toString().trimmed());
            file.close();
        }
    }

    explicit GmailAccountDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Quản lý tài khoản Gmail");
        QVBoxLayout *layout = new QVBoxLayout(this);
        list = new QListWidget(this);
        layout->addWidget(list);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        addBtn = new QPushButton("Thêm Gmail...", this);
        delBtn = new QPushButton("Xoá Gmail", this);
        btnLayout->addWidget(addBtn);
        btnLayout->addWidget(delBtn);
        layout->addLayout(btnLayout);

        connect(addBtn, &QPushButton::clicked, this, &GmailAccountDialog::onAddClicked);
        connect(delBtn, &QPushButton::clicked, this, &GmailAccountDialog::onDeleteClicked);
        connect(list, &QListWidget::itemDoubleClicked, this, [=](QListWidgetItem* item){
            selectedEmail = item->text().trimmed();
            accept();
        });

        reloadList();
    }
private slots:
    void onAddClicked() {
        addNew = true;
        accept();
    }
    void onDeleteClicked() {
        auto item = list->currentItem();
        if (!item) {
            QMessageBox::warning(this, "Chưa chọn", "Chọn tài khoản cần xoá trước");
            return;
        }
        QString email = item->text().trimmed();
        if (QMessageBox::question(this, "Xác nhận", "Xoá tài khoản này?") == QMessageBox::Yes) {
            QFile file("tmptokens.json");
            QJsonArray arr;
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                arr = doc.array();
                file.close();
            }
            QJsonArray newArr;
            for (auto v : arr) {
                if (v.toObject()["email"].toString().trimmed() != email)
                    newArr.append(v);
            }
            file.open(QIODevice::WriteOnly | QIODevice::Truncate);
            file.write(QJsonDocument(newArr).toJson());
            file.close();

            reloadList();
        }
    }
private:
    QListWidget* list;
    QPushButton* addBtn;
    QPushButton* delBtn;
};
