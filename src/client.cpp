#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

class Client {
private:
    int client_socket;
    struct sockaddr_in server_address;

public:
    Client(const char* server_ip, int server_port) {
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket < 0) {
            std::cerr << "Socket creation failed!" << std::endl;
            exit(1);
        }

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(server_port);
        if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            exit(1);
        }

        if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
            std::cerr << "Connection to server failed!" << std::endl;
            exit(1);
        }
    }

    void start() {
        std::string command;
        char buffer[2048] = {0};

        while (true) {
            std::cout << "> ";
            std::getline(std::cin, command);

            if (command == "exit" || command == "EXIT") {
                this->disconnect(client_socket);
                return;
            }
            
            int send_value;
            send_value = send(client_socket, command.c_str(), command.length(), 0);
            memset(buffer, 0, sizeof(buffer));
            ssize_t valread = read(client_socket, buffer, sizeof(buffer) - 1);
            if (valread < 0) {
                std::cout << "No response" << std::endl;
            }
            std::cout << buffer << std::endl;
        }
    }

    void disconnect(int client_socket) {
        std::cout << "Disconnected" << std::endl;
        close(client_socket);
    }
};

int main() {
    Client client("127.0.0.1", 8000);
    client.start();
    return 0;
}
