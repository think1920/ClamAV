#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <glob.h>
#include <thread>
#include <atomic>
#include <sys/stat.h>
#include <dirent.h>
#include <climits>

#define endl '\n'

using namespace std;

class FTPClient {
private:
    int control_socket, data_socket, ftp_server_port, clamav_port;
    string ftp_server_ip, clamav_ip, current_dir;
    bool connected, binary_mode, lang, passive_mode;
    atomic<bool> running;

public:
    FTPClient(const string& ftp_ip = "127.0.0.1", int ftp_port = 21,
              const string& clam_ip = "127.0.0.1", int clam_port = 9000)
        : control_socket(-1), data_socket(-1),
          ftp_server_port(ftp_port),
          clamav_port(clam_port),
          ftp_server_ip(ftp_ip),
          clamav_ip(clam_ip),
          current_dir("."),
          connected(0),
          binary_mode(1),
          lang(1),
          passive_mode(1),
          running(1) {}

    ~FTPClient() { disconnect(); }

    bool connectToFTP() {
        control_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (control_socket < 0) {
            cerr << "Error creating socket" << endl;
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(ftp_server_port);
        inet_pton(AF_INET, ftp_server_ip.c_str(), &server_addr.sin_addr);

        if (connect(control_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Cannot connect to FTP server at " << ftp_server_ip
                 << ":" << ftp_server_port << endl;
            close(control_socket);
            return false;
        }

        // Đọc welcome message
        string welcome = readFTPResponse();
        cout << "Server: " << welcome << endl;

        
        // Login anonymous (hoặc login với ftpuser)
        if (!sendFTPCommand("USER ftpuser")) {
            disconnect();
            return false;
        }

        // Gửi mật khẩu (nếu mật khẩu chỉ là một dấu cách)
        if (!sendFTPCommand("PASS  ")) {  // PASS + two spaces + \r\n bên trong sendFTPCommand
            disconnect();
            return false;
        }

        // Set binary mode
        sendFTPCommand("TYPE I");

        connected = true;
        cout << "Connected and logged in to FTP server" << endl;
        return true;
    }

    void disconnect() {
        if (connected && control_socket >= 0) {
            sendFTPCommand("QUIT");
            close(control_socket);
            connected = false;
            cout << "Disconnected from FTP server" << endl;
        }
    }

    string readFTPResponse() {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(control_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            return string(buffer);
        }
        return "";
    }

    bool sendFTPCommand(const string& command) {
        string full_command = command + "\r\n";
        send(control_socket, full_command.c_str(), full_command.length(), 0);
        
        string response = readFTPResponse();
        cout << "Server: " << response;
        
        // Check if response starts with 2xx or 3xx (success/continue)
        if (!response.empty() && (response[0] == '2' || response[0] == '3')) {
            return true;
        }
        return response[0] == '1'; // Also accept 1xx (preliminary positive)
    }

    bool openDataConnection() {
        if (passive_mode) {
            return openPassiveDataConnection();
        } else {
            return openActiveDataConnection();
        }
    }

    bool openPassiveDataConnection() {
        // Gửi lệnh PASV
        string pasv_command = "PASV";
        string full_command = pasv_command + "\r\n";
        send(control_socket, full_command.c_str(), full_command.length(), 0);

        // Nhận response
        string response = readFTPResponse();
        cout << "PASV Response: " << response;

        // Parse response để lấy IP và port
        // Format: 227 Entering Passive Mode (192,168,1,1,20,21)
        size_t start = response.find('(');
        size_t end = response.find(')');

        if (start == string::npos || end == string::npos) {
            return false;
        }

        string addr_str = response.substr(start + 1, end - start - 1);
        
        // Parse địa chỉ IP và port
        vector<int> nums;
        stringstream ss(addr_str);
        string token;

        while (getline(ss, token, ',')) {
            nums.push_back(stoi(token));
        }

        if (nums.size() != 6) {
            return false;
        }

        // Tạo địa chỉ IP
        string data_ip = to_string(nums[0]) + "." + 
                        to_string(nums[1]) + "." + 
                        to_string(nums[2]) + "." + 
                        to_string(nums[3]);

        // Tính port
        int data_port = nums[4] * 256 + nums[5];

        // Kết nối đến data port
        data_socket = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in data_addr;
        data_addr.sin_family = AF_INET;
        data_addr.sin_port = htons(data_port);
        inet_pton(AF_INET, data_ip.c_str(), &data_addr.sin_addr);

        return connect(data_socket, (sockaddr*)&data_addr, sizeof(data_addr)) == 0;
    }

    bool openActiveDataConnection() {
        // Implementation for active mode (tùy chọn)
        return false; // Simplified - chỉ dùng passive mode
    }

    void closeDataConnection() {
        if (data_socket >= 0) {
            close(data_socket);
            data_socket = -1;
        }
    }

    bool sendFileData(const string& filename) {
        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            return false;
        }

        char buffer[4096];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            int bytes_sent = send(data_socket, buffer, file.gcount(), 0);
            if (bytes_sent <= 0) {
                file.close();
                return false;
            }
        }

        file.close();
        return true;
    }

    bool uploadToFTPServer(const string& filename) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return false;
        }

        // Mở data connection
        if (!openDataConnection()) {
            cout << "Failed to open data connection" << endl;
            return false;
        }
        
        string path = filename;

        size_t pos = path.find_last_of('/');
        if (pos != string::npos) {
            string dir = path.substr(0, pos);
            path = path.substr(pos + 1);
        }

        // Gửi lệnh STOR
        string stor_command = "STOR " + path;
        string full_command = stor_command + "\r\n";
        send(control_socket, full_command.c_str(), full_command.length(), 0);

        // Đọc response từ server
        string response = readFTPResponse();
        cout << "STOR Response: " << response;

        // Upload file qua data connection
        if (!sendFileData(filename)) {
            cout << "Failed to send file data" << endl;
            closeDataConnection();
            return false;
        }

        // Đóng data connection
        closeDataConnection();

        // Đọc completion response
        string completion = readFTPResponse();
        cout << "Upload completion: " << completion;

        return true;
    }

    bool scanFileWithClamAV(const string& filename) {
        int clam_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (clam_socket < 0) {
            cerr << "Error creating socket to ClamAV" << endl;
            return false;
        }

        sockaddr_in clam_addr;
        clam_addr.sin_family = AF_INET;
        clam_addr.sin_port = htons(clamav_port);
        inet_pton(AF_INET, clamav_ip.c_str(), &clam_addr.sin_addr);

        if (connect(clam_socket, (sockaddr*)&clam_addr, sizeof(clam_addr)) < 0) {
            cerr << "Cannot connect to ClamAV Agent at " << clamav_ip
                 << ":" << clamav_port << endl;
            close(clam_socket);
            return false;
        }

        ifstream file(filename, ios::binary);
        if (!file.is_open()) {
            cerr << "Cannot open file: " << filename << endl;
            close(clam_socket);
            return false;
        }

        char buffer[4096];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            send(clam_socket, buffer, file.gcount(), 0);
        }
        file.close();

        shutdown(clam_socket, SHUT_WR);

        char result[20];
        memset(result, 0, sizeof(result));
        int bytes_received = recv(clam_socket, result, sizeof(result) - 1, 0);
        close(clam_socket);

        if (bytes_received <= 0) {
            cerr << "Error receiving scan result" << endl;
            return false;
        }

        string scan_result(result);
        cout << "Virus scan result: " << scan_result << endl;
        return scan_result == "OK";
    }

    void putFile(const string& filename) {
        ifstream test_file(filename);
        if (!test_file.good()) {
            cout << "Error: File '" << filename << "' not found" << endl;
            return;
        }
        test_file.close();

        cout << "Scanning file '" << filename << "' for viruses..." << endl;

        if (scanFileWithClamAV(filename)) {
            cout << "File is clean. Uploading to FTP server..." << endl;
            
            // Upload thực lên FTP server
            if (uploadToFTPServer(filename)) {
                cout << "Upload completed successfully." << endl;
            } else {
                cout << "Upload failed!" << endl;
            }
        } else {
            cout << "WARNING: File '" << filename
                 << "' is infected or scan failed! Upload aborted." << endl;
        }
    }

    void getFile(const string& filename) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        // Mở data connection
        if (!openDataConnection()) {
            cout << "Failed to open data connection" << endl;
            return;
        }

        // Gửi lệnh RETR
        string retr_command = "RETR " + filename;
        if (!sendFTPCommand(retr_command)) {
            closeDataConnection();
            return;
        }

        // Nhận file data
        ofstream outfile(filename, ios::binary);
        char buffer[4096];
        int bytes_received;

        while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
            outfile.write(buffer, bytes_received);
        }

        outfile.close();
        closeDataConnection();

        // Đọc completion response
        string completion = readFTPResponse();
        cout << "Download completion: " << completion;
        cout << "Download completed: " << filename << endl;
    }

    void listFiles() {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        // Mở data connection
        if (!openDataConnection()) {
            cout << "Failed to open data connection" << endl;
            return;
        }

        // Gửi lệnh LIST
        if (!sendFTPCommand("LIST")) {
            closeDataConnection();
            return;
        }

        // Nhận danh sách file
        cout << "Files on FTP server:" << endl;
        char buffer[4096];
        int bytes_received;

        while ((bytes_received = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
            cout.write(buffer, bytes_received);
        }

        closeDataConnection();

        // Đọc completion response
        string completion = readFTPResponse();
        cout << completion;
    }

    void changeDirectory(const string& dir) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        string cwd_command = "CWD " + dir;
        if (sendFTPCommand(cwd_command)) {
            current_dir = dir;
            cout << "Changed directory to: " << dir << endl;
        } else {
            cout << "Failed to change directory" << endl;
        }
    }

    void printWorkingDirectory() {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        if (sendFTPCommand("PWD")) {
            // Response already printed by sendFTPCommand
        }
    }

    void makeDirectory(const string& dir) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        string mkd_command = "MKD " + dir;
        if (sendFTPCommand(mkd_command)) {
            cout << "Directory created: " << dir << endl;
        } else {
            cout << "Failed to create directory" << endl;
        }
    }

    void removeDirectory(const string& dir) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        string rmd_command = "RMD " + dir;
        if (sendFTPCommand(rmd_command)) {
            cout << "Directory removed: " << dir << endl;
        } else {
            cout << "Failed to remove directory" << endl;
        }
    }

    void deleteFile(const string& filename) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        string dele_command = "DELE " + filename;
        if (sendFTPCommand(dele_command)) {
            cout << "File deleted: " << filename << endl;
        } else {
            cout << "Failed to delete file" << endl;
        }
    }

    void renameFile(const string& oldname, const string& newname) {
        if (!connected) {
            cout << "Not connected to FTP server" << endl;
            return;
        }

        string rnfr_command = "RNFR " + oldname;
        if (sendFTPCommand(rnfr_command)) {
            string rnto_command = "RNTO " + newname;
            if (sendFTPCommand(rnto_command)) {
                cout << "File renamed from " << oldname << " to " << newname << endl;
            } else {
                cout << "Failed to complete rename" << endl;
            }
        } else {
            cout << "Failed to start rename" << endl;
        }
    }

    void mputFiles(const string& pattern) {
        glob_t glob_result;
        int ret = glob(pattern.c_str(), GLOB_TILDE, nullptr, &glob_result);

        if (ret != 0) {
            cout << "No files match pattern: " << pattern << endl;
            return;
        }

        cout << "Found " << glob_result.gl_pathc << " files matching pattern" << endl;

        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            string filename = glob_result.gl_pathv[i];
            cout << "\nProcessing: " << filename << endl;
            putFile(filename);
        }

        globfree(&glob_result);
    }

    void clean() {system("clear");}

    void showStatus() {
        cout << (!lang ? "oOo FTP Client Status oOo" : "oOo Trạng Thái FTP Client oOo") << endl;
        cout << (!lang ? "FTP Server: " : "Máy chủ FTP: ") << ftp_server_ip << ":" << ftp_server_port << endl;
        cout << (!lang ? "ClamAV Agent: " : "ClamAV Agent: ") << clamav_ip << ":" << clamav_port << endl;
        cout << (!lang ? "Connected: " : "Đã kết nối: ") << (connected ? (!lang ? "Yes" : "Có") : (!lang ? "No" : "Không")) << endl;
        cout << (!lang ? "Transfer Mode: " : "Chế độ truyền: ") << (binary_mode ? "Binary" : "ASCII") << endl;
        cout << (!lang ? "Current Directory: " : "Thư mục hiện tại: ") << (current_dir.empty() ? "/" : current_dir) << endl;
    }

    void showHelp() {
        cout << (!lang ? "\noOo Available FTP Commands oOo" : "\noOo Các Lệnh FTP oOo") << endl;
        cout << (!lang ? "File Operations:" : "Tác vụ tệp:") << endl;
        cout << "  put <path>           - " << (!lang ? "Upload file (ClamAV)" : "Tải lên tệp (ClamAV)") << endl;
        cout << "  get <file>           - " << (!lang ? "Download file" : "Tải xuống tệp") << endl;
        cout << "  mput <pattern>       - " << (!lang ? "Upload multiple files" : "Tải lên nhiều tệp") << endl;
        cout << "  mget <pattern>       - " << (!lang ? "Download multiple files" : "Tải xuống nhiều tệp") << endl;
        cout << "  delete <file>        - " << (!lang ? "Delete file on server" : "Xóa tệp trên server") << endl;
        cout << "  rename <old> <new>   - " << (!lang ? "Rename file on server" : "Đổi tên tệp trên server") << endl;
        cout << "\n" << (!lang ? "Directory Operations:" : "Tác vụ thư mục:") << endl;
        cout << "  ls                   - " << (!lang ? "List files on server" : "Liệt kê tệp trên server") << endl;
        cout << "  cd <dir>             - " << (!lang ? "Change directory on server" : "Đổi thư mục trên server") << endl;
        cout << "  pwd                  - " << (!lang ? "Show current directory on server" : "Hiển thị thư mục hiện tại trên server") << endl;
        cout << "  mkdir <dir>          - " << (!lang ? "Create directory on server" : "Tạo thư mục trên server") << endl;
        cout << "  rmdir <dir>          - " << (!lang ? "Remove directory on server" : "Xóa thư mục trên server") << endl;
        cout << "\n" << (!lang ? "Session Management:" : "Quản lý phiên:") << endl;
        cout << "  vn/en                - " << (!lang ? "Vietnamese/English" : "Tiếng Việt/Tiếng Anh") << endl;
        cout << "  clear                - " << (!lang ? "Clear screen" : "Dọn màn hình") << endl;
        cout << "  open                 - " << (!lang ? "Connect to FTP server" : "Kết nối đến máy chủ FTP") << endl;
        cout << "  close                - " << (!lang ? "Disconnect from FTP server" : "Ngắt kết nối FTP") << endl;
        cout << "  status               - " << (!lang ? "Show connection status" : "Xem trạng thái kết nối") << endl;
        cout << "  binary/ascii         - " << (!lang ? "Set transfer mode" : "Chọn chế độ truyền") << endl;
        cout << "  passive              - " << (!lang ? "Toggle passive mode" : "Chuyển đổi chế độ passive") << endl;
        cout << "  quit/exit            - " << (!lang ? "Exit client" : "Thoát khỏi chương trình") << endl;
        cout << "  help/?               - " << (!lang ? "Show this help" : "Hiển thị hướng dẫn") << endl;
        cout << endl;
    }

    void printHeader() {
        cout << (!lang ? "\noOo Secure FTP Client with ClamAV oOo" : "\noOo FTP Client và ClamAV oOo") << endl;
        cout << (!lang ? "Type 'help' for available commands" : "Gõ 'help' để xem các lệnh") << endl;
    }

    void runCommandLoop() {
        string command;
        printHeader();
        
        while (running) {
            cout << "ftp> ";
            getline(cin, command);
            if (command.empty()) continue;

            istringstream iss(command);
            string cmd;
            iss >> cmd;
            transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if (cmd == "put") {
                string filename;
                iss >> filename;
                if (!filename.empty()) {
                    putFile(filename);
                } else {
                    cout << "Usage: put <filename>" << endl;
                }
            }
            else if (cmd == "get" || cmd == "recv") {
                string filename;
                iss >> filename;
                if (!filename.empty()) {
                    getFile(filename);
                } else {
                    cout << "Usage: get <filename>" << endl;
                }
            }
            else if (cmd == "mput") {
                string pattern;
                iss >> pattern;
                if (!pattern.empty()) {
                    mputFiles(pattern);
                } else {
                    cout << "Usage: mput <pattern>" << endl;
                }
            }
            else if (cmd == "mget") {
                string pattern;
                iss >> pattern;
                cout << "mget " << pattern << " - Feature to be implemented" << endl;
            }
            else if (cmd == "delete" || cmd == "del") {
                string filename;
                iss >> filename;
                if (!filename.empty()) {
                    deleteFile(filename);
                } else {
                    cout << "Usage: delete <filename>" << endl;
                }
            }
            else if (cmd == "rename" || cmd == "ren") {
                string oldname, newname;
                iss >> oldname >> newname;
                if (!oldname.empty() && !newname.empty()) {
                    renameFile(oldname, newname);
                } else {
                    cout << "Usage: rename <oldname> <newname>" << endl;
                }
            }
            else if (cmd == "ls") {
                listFiles();
            }
            else if (cmd == "cd") {
                string dir;
                iss >> dir;
                if (!dir.empty()) {
                    changeDirectory(dir);
                } else {
                    cout << "Usage: cd <directory>" << endl;
                }
            }
            else if (cmd == "pwd") {
                printWorkingDirectory();
            }
            else if (cmd == "mkdir") {
                string dir;
                iss >> dir;
                if (!dir.empty()) {
                    makeDirectory(dir);
                } else {
                    cout << "Usage: mkdir <directory>" << endl;
                }
            }
            else if (cmd == "rmdir") {
                string dir;
                iss >> dir;
                if (!dir.empty()) {
                    removeDirectory(dir);
                } else {
                    cout << "Usage: rmdir <directory>" << endl;
                }
            }
            else if (cmd == "open") {
                if (!connected) {
                    connectToFTP();
                } else {
                    cout << "Already connected to FTP server" << endl;
                }
            }
            else if (cmd == "vn") {
                lang = true;
                clean();
                printHeader();
            }
            else if (cmd == "en") {
                lang = false;
                clean();
                printHeader();
            }
            else if (cmd == "close") {
                disconnect();
            }
            else if (cmd == "status") {
                showStatus();
            }
            else if (cmd == "binary") {
                binary_mode = true;
                if (connected) sendFTPCommand("TYPE I");
                cout << "Transfer mode set to binary" << endl;
            }
            else if (cmd == "ascii") {
                binary_mode = false;
                if (connected) sendFTPCommand("TYPE A");
                cout << "Transfer mode set to ASCII" << endl;
            }
            else if (cmd == "passive") {
                passive_mode = !passive_mode;
                cout << "Passive mode: " << (passive_mode ? "ON" : "OFF") << endl;
            }
            else if (cmd == "clear") {
                clean();
                printHeader();
            }
            else if (cmd == "quit" || cmd == "exit") {
                running = false;
                cout << "Shutting down..." << endl;
                break;
            }
            else if (cmd == "help" || cmd == "?") {
                showHelp();
            }
            else {
                cout << "Unknown command: " << cmd << endl;
                cout << "Type 'help' for available commands" << endl;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    string ftp_ip = "127.0.0.1";
    int ftp_port = 21;
    string clamav_ip = "127.0.0.1";
    int clamav_port = 9000;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--ftp-server" && i + 1 < argc) {
            ftp_ip = argv[++i];
        } else if (arg == "--ftp-port" && i + 1 < argc) {
            ftp_port = atoi(argv[++i]);
        } else if (arg == "--clamav-server" && i + 1 < argc) {
            clamav_ip = argv[++i];
        } else if (arg == "--clamav-port" && i + 1 < argc) {
            clamav_port = atoi(argv[++i]);
        }
    }

    FTPClient client(ftp_ip, ftp_port, clamav_ip, clamav_port);
    client.runCommandLoop();

    return 0;
}
