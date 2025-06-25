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
    bool connected, binary_mode;
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
          connected(false),
          binary_mode(true),
          running(true) {}

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
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            std::string fullpath = joinPath(current_dir, name);

            struct stat st;
            if (stat(fullpath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                name += '/';
            }

            cout << name << endl;
        }

        closedir(dir);
    }

    void changeDirectory(const std::string& dir) {
        if (chdir(dir.c_str()) == 0) {
            char buf[PATH_MAX];
            if (getcwd(buf, sizeof(buf)) != nullptr) {
                current_dir = std::string(buf);
            } else {
                perror("getcwd failed");
                current_dir = dir;
            }
            cout << "Changed directory to: " << current_dir << endl;
        } else {
            perror("chdir failed");
        }
    }

    std::string joinPath(const std::string& dir, const std::string& file) {
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
        cout << "=== FTP Client Status ===" << endl;
        cout << "FTP Server: " << ftp_server_ip << ":" << ftp_server_port << endl;
        cout << "ClamAV Agent: " << clamav_ip << ":" << clamav_port << endl;
        cout << "Connected: " << (connected ? "Yes" : "No") << endl;
        cout << "Transfer Mode: " << (binary_mode ? "Binary" : "ASCII") << endl;
        cout << "Current Directory: " << (current_dir.empty() ? "/" : current_dir) << endl;
    }

    void showHelp() {
        cout << "\n=== Available FTP Commands ===" << endl;
        cout << "File Operations:" << endl;
        cout << "  put <file>       - Upload file (with virus scan)" << endl;
        cout << "  get <file>       - Download file" << endl;
        cout << "  mput <pattern>   - Upload multiple files" << endl;
        cout << "  mget <pattern>   - Download multiple files" << endl;
        cout << "\nDirectory Operations:" << endl;
        cout << "  ls               - List files" << endl;
        cout << "  cd <dir>         - Change directory" << endl;
        cout << "  pwd              - Show current directory" << endl;
        cout << "  mkdir <dir>      - Create directory" << endl;
        cout << "\nSession Management:" << endl;
        cout << "  open             - Connect to FTP server" << endl;
        cout << "  close            - Disconnect from FTP server" << endl;
        cout << "  status           - Show connection status" << endl;
        cout << "  binary/ascii     - Set transfer mode" << endl;
        cout << "  quit/exit        - Exit client" << endl;
        cout << "  help/?           - Show this help" << endl;
        cout << endl;
    }

    void printHeader() {
        cout << "\n=== Secure FTP Client with Virus Scanning ===" << endl;
        cout << "Type 'help' for available commands" << endl;
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
                cout << "Shutting Down..." << endl;
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
