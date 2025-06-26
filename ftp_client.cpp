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
#include <dirent.h>
#include <sys/stat.h>
#define endl '\n'
using namespace std;

class FTPClient {
private:
    int control_socket, ftp_server_port, clamav_port;
    string ftp_server_ip, clamav_ip, current_dir;
    bool connected, binary_mode, lang;
    atomic<bool> running;

public:
    FTPClient(const string& ftp_ip = "127.0.0.1", int ftp_port = 21,
              const string& clam_ip = "127.0.0.1", int clam_port = 9000)
        : control_socket(-1),
          ftp_server_port(ftp_port),
          clamav_port(clam_port),
          ftp_server_ip(ftp_ip),
          clamav_ip(clam_ip),
          current_dir("."),
          connected(0),
          binary_mode(1),
          lang(0),
          running(1) {}

    ~FTPClient() {disconnect();}

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

        connected = true;
        cout << "Connected to FTP server " << ftp_server_ip
                  << ":" << ftp_server_port << endl;
        return true;
    }

    void disconnect() {
        if (connected && control_socket >= 0) {
            close(control_socket);
            connected = false;
            cout << "Disconnected from FTP server" << endl;
        }
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
            cout << "File is clean. Simulating upload to FTP server..." << endl;
            simulateUploadToFTP(filename);
        } else {
            cout << "WARNING: File '" << filename
                      << "' is infected or scan failed! Upload aborted." << endl;
        }
    }

    void simulateUploadToFTP(const string& filename) {
        cout << "Uploading '" << filename << "' to FTP server..." << endl;
        cout << "Upload completed successfully." << endl;
    }

    void getFile(const string& filename) {
        cout << "Downloading '" << filename << "' from FTP server..." << endl;
        cout << "Download completed." << endl;
    }


    void listFiles() {
        DIR* dir = opendir(current_dir.c_str());
        if (!dir) {
            perror("opendir failed");
            return;
        }

        dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name == "." || name == "..") continue;

            string fullpath = joinPath(current_dir, name);

            struct stat st;
            if (stat(fullpath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                name += '/';
            }

            cout << name << endl;
        }

        closedir(dir);
    }

    void changeDirectory(const string& dir) {
        if (chdir(dir.c_str()) == 0) {
            char buf[PATH_MAX];
            if (getcwd(buf, sizeof(buf)) != nullptr) {
                current_dir = string(buf);
            } else {
                perror("getcwd failed");
                current_dir = dir;
            }
            cout << "Changed directory to: " << current_dir << endl;
        } else {
            perror("chdir failed");
        }
    }

    string joinPath(const string& dir, const string& file) {
        if (dir.empty() || dir.back() == '/') return dir + file;
        return dir + "/" + file;
    }

    void clean() {system("clear");}

    void printWorkingDirectory() {
        cout << "Current directory: " << (current_dir.empty() ? "/" : current_dir) << endl;
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

    void showStatus() {
        cout << (!lang ? "=== FTP Client Status ===" : "=== Trạng Thái FTP Client ===") << endl;
        cout << (!lang ? "FTP Server: " : "Máy chủ FTP: ") << ftp_server_ip << ":" << ftp_server_port << endl;
        cout << (!lang ? "ClamAV Agent: " : "ClamAV Agent: ") << clamav_ip << ":" << clamav_port << endl;
        cout << (!lang ? "Connected: " : "Đã kết nối: ") << (connected ? (!lang ? "Yes" : "Có") : (!lang ? "No" : "Không")) << endl;
        cout << (!lang ? "Transfer Mode: " : "Chế độ truyền: ") << (binary_mode ? "Binary" : "ASCII") << endl;
        cout << (!lang ? "Current Directory: " : "Thư mục hiện tại: ") << (current_dir.empty() ? "/" : current_dir) << endl;
    }

    void showHelp() {
        cout << (!lang ? "\n=== Available FTP Commands ===" : "\n=== Các Lệnh FTP Có Thể Dùng ===") << endl;

        cout << (!lang ? "File Operations:" : "Tác vụ tệp:") << endl;
        cout << "  put <file>       - " << (!lang ? "Upload file (with virus scan)" : "Tải lên tệp (có quét virus)") << endl;
        cout << "  get <file>       - " << (!lang ? "Download file" : "Tải xuống tệp") << endl;
        cout << "  mput <pattern>   - " << (!lang ? "Upload multiple files" : "Tải lên nhiều tệp") << endl;
        cout << "  mget <pattern>   - " << (!lang ? "Download multiple files" : "Tải xuống nhiều tệp") << endl;

        cout << "\n" << (!lang ? "Directory Operations:" : "Tác vụ thư mục:") << endl;
        cout << "  ls               - " << (!lang ? "List files" : "Liệt kê tệp") << endl;
        cout << "  cd <dir>         - " << (!lang ? "Change directory" : "Đổi thư mục") << endl;
        cout << "  pwd              - " << (!lang ? "Show current directory" : "Hiển thị thư mục hiện tại") << endl;
        cout << "  mkdir <dir>      - " << (!lang ? "Create directory" : "Tạo thư mục") << endl;

        cout << "\n" << (!lang ? "Session Management:" : "Quản lý phiên:") << endl;
        cout << "  vn/en            - " << (!lang ? "Vietnamese/English" : "Tiếng Việt/Tiếng Anh") << endl;
        cout << "  clear            - " << (!lang ? "Clear screen" : "Dọn màn hình") << endl;
        cout << "  open             - " << (!lang ? "Connect to FTP server" : "Kết nối đến máy chủ FTP") << endl;
        cout << "  close            - " << (!lang ? "Disconnect from FTP server" : "Ngắt kết nối FTP") << endl;
        cout << "  status           - " << (!lang ? "Show connection status" : "Xem trạng thái kết nối") << endl;
        cout << "  binary/ascii     - " << (!lang ? "Set transfer mode" : "Chọn chế độ truyền") << endl;
        cout << "  quit/exit        - " << (!lang ? "Exit client" : "Thoát khỏi chương trình") << endl;
        cout << "  help/?           - " << (!lang ? "Show this help" : "Hiển thị hướng dẫn") << endl;

        cout << endl;
    }


    void printHeader() {
        cout << (!lang ? "\n=== Secure FTP Client with Virus Scanning ===" : "\n=== FTP Client Bảo Mật với Quét Virus ===") << endl;
        cout << (!lang ? "Type 'help' for available commands" : "Gõ 'help' để xem các lệnh có sẵn") << endl;
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
                cout << "mget " << pattern << " - Feature simulated" << endl;
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
                cout << "mkdir " << dir << " - Feature simulated" << endl;
            }
            else if (cmd == "open") {
                if (!connected) {
                    connectToFTP();
                } else {
                    cout << "Already connected to FTP server" << endl;
                }
            }
            else if (cmd == "vn") {
                lang = 1;
                clean();
                printHeader();
            }
            else if (cmd == "en") {
                lang = 0;
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
                cout << "Transfer mode set to binary" << endl;
            }
            else if (cmd == "ascii") {
                binary_mode = false;
                cout << "Transfer mode set to ASCII" << endl;
            }
            else if (cmd == "clear") {
                clean(); printHeader();
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
