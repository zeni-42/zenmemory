#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>
#include <thread>
#include <cstring>

// Server components
class Server {
    private: 
        int server_socket;
        struct sockaddr_in server_address;
        bool isActive = true;
        std::unordered_map<int, std::string> client_ips;
    public:
        // Constructor to get the ip and the port
        Server(int port): isActive(true) {
            this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (this->server_socket < 0) {
                std::cout << "socket creation failed!" << std::endl;
                exit(1);
            }
        
            int socket_options = 1;
            if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_options, sizeof(socket_options)) < 0 ){
                std::cout << "socket options binding failed!" << std::endl;
                exit(1);
            }
        
            server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(port);
            if (bind(this->server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0 ){
                std::cout << "socket binding failed!" << std::endl;
                exit(1);
            }
        
            int backlog_connections = 10;
            if (listen(this->server_socket, backlog_connections) < 0) {
                std::cout << "failed to listen at the give port!" << std::endl;
                exit(1);
            }
            std::cout << "SERVER: "<< port << std::endl;
        }

        // This methord will accept connecrion from the client and responde them accordingly
        void start() {
            struct sockaddr_in client_address;
            while (isActive) {
                socklen_t client_address_length = sizeof(client_address);
                int client_socket = accept(this->server_socket, (struct sockaddr*)&client_address, &client_address_length);
                if (client_socket < 0) {
                    std::cout << "failed to accept connections!" << std::endl;
                }
            
                char ip[16];
                inet_ntop(AF_INET, &client_address.sin_addr, ip, 16);
                client_ips[client_socket] = std::string(ip);
            
                std::cout << "Client connected: " << ip << std::endl;
            
                while (true)
                {
                    char read_buffer[2048] = {};
                    long int bytes_recieved = recv(client_socket, read_buffer, sizeof(read_buffer), 0);
                
                    if (bytes_recieved <= 0) {
                        std::cout << "Client disconnected: " << ip << std::endl;
                        disconnect_client(client_socket);
                        break;
                    }
                
                    std::string modified_command;
                    std::string command(reinterpret_cast<char*>(read_buffer), bytes_recieved);
                    for (char ch: command) {
                        if (!std::isspace(ch)) {
                            modified_command += ch;
                        }
                    }
                    std::string processed_commnd = process_command(modified_command);
                    send_response(client_socket, processed_commnd);
                }
            }
        }

        // This method will stop the server 
        void stop(){
            if (server_socket != -1) {
                close(server_socket);
                isActive = false;
            }
            std::cout << "Stopped!!" << std::endl;
        }

        // Using this methord the client can disconnect quitely
        void disconnect_client(int client_socket){
            close(client_socket);
            return;
        }

        // Destructor to free up the resources
        ~Server() {
            stop();
        }
    private:
        // Method which will process the command.
        std::string process_command(std::string command){
            if (command == "hola" || command == "HOLA")  {
                return "AMIGO!";
            } else if (command == "sudostop") {
                return "STOP";
            }
            return "command not found";
        }

        // Method which is responsible for sending the response.
        void send_response(int client_socket, std::string response){
            ssize_t send_bytes = write(client_socket, response.c_str(), response.size());
            if (send_bytes < 0) {
                std::cout << "failed to send response!" << std::endl;
            }
        }
};

int main(){
    Server server(8000);
    server.start();
    return 0;
}