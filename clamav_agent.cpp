#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <thread>
#define endl '\n'
using namespace std;

class ClamAVAgent {
private:
    int server_socket, port;
    bool running;
    
public:
    ClamAVAgent(int p) : port(p), running(true) {}
    
    void start() {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            cerr << "Error creating socket" << endl;
            return;
        }
        
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            cerr << "Error binding socket" << endl;
            close(server_socket);
            return;
        }
        
        if (listen(server_socket, 5) < 0) {
            cerr << "Error listening on socket" << endl;
            close(server_socket);
            return;
        }
        
        cout << "=== ClamAV Agent Server Started ===" << endl;
        cout << "Listening on port " << port << endl;
        cout << "Press Ctrl+C to stop" << endl;
        
        while(running) {
            int client_socket = accept(server_socket, nullptr, nullptr);
            if (client_socket < 0) {
                if (running) {
                    cerr << "Error accepting connection" << endl;
                }
                continue;
            }
            
            cout << "New client connected" << endl;
            thread client_thread(&ClamAVAgent::handleClient, this, client_socket);
            client_thread.detach();
        }
    }
    
    void stop() {
        running = false;
        close(server_socket);
    }
    
    void handleClient(int client_socket) {
        string temp_filename = "temp_scan_file_" + to_string(client_socket);
        
        try {
            receiveFile(client_socket, temp_filename);
            string scan_result = scanFile(temp_filename);
            cout << "Scan result for " << temp_filename << ": " << scan_result << endl;
            send(client_socket, scan_result.c_str(), scan_result.length(), 0);
            remove(temp_filename.c_str());
        } catch (const exception& e) {
            cerr << "Error handling client: " << e.what() << endl;
            string error_msg = "ERROR";
            send(client_socket, error_msg.c_str(), error_msg.length(), 0);
        }
        
        close(client_socket);
        cout << "Client disconnected" << endl;
    }
    
    void receiveFile(int client_socket, const string& filename) {
        ofstream temp_file(filename, ios::binary);
        if (!temp_file.is_open()) {
            throw runtime_error("Cannot create temp file");
        }
        
        char buffer[4096];
        int bytes_received;
        
        while((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
            temp_file.write(buffer, bytes_received);
        }
        temp_file.close();
        
        if (bytes_received < 0) {
            throw runtime_error("Error receiving file");
        }
    }
    
    string scanFile(const string& filename) {
        string command = "clamscan --no-summary " + filename + " 2>/dev/null";
        int result = system(command.c_str());
        
        if (result == 0) {
            return "OK";
        } else if (result == 256) { // 1 << 8 (virus found)
            return "INFECTED";
        } else {
            return "ERROR";
        }
    }
};

ClamAVAgent* global_agent = nullptr;

void signalHandler(int) {
    cout << "\nShutting down ClamAV Agent..." << endl;
    if (global_agent) {
        global_agent->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 9000;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    ClamAVAgent agent(port);
    global_agent = &agent;
    
    cout << "Starting ClamAV Agent on port " << port << endl;
    cout << "Make sure ClamAV is installed: sudo apt-get install clamav" << endl;
    
    agent.start();
    
    return 0;
}
