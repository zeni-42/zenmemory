#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <vector>
#include <thread>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <sys/epoll.h>
#include <fcntl.h>

#define MAX_EVENT 100

// Server components
class Server {
    private: 
        int server_socket; // this is my server file descriptor or the fd of the server
        struct sockaddr_in server_address;
        bool isActive = true;
        std::unordered_map<int, std::string> client_ips;
        int epoll_fd;   // epoll file descriptor;
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
        
            // Creating an instance of epoll
            epoll_fd = epoll_create1(0);
            if (epoll_fd < 0) {
                std::cout << "failed to create epoll" << std::endl;
                exit(1);
            }
        
            // setting the server in non blocking mode.
            set_non_blocking(server_socket);
        
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = server_socket;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) < 0) {
                std::cout << "epoll creation failed!" << std::endl;
                exit(1);
            }
        
            std::cout << "SERVER: "<< port << std::endl;
        }

        // This methord will start the server and call accept_client or handle_client based on the existing connections
        void start() {
            struct epoll_event event[MAX_EVENT];
            while (isActive) {
                int event_count = epoll_wait(epoll_fd, event, MAX_EVENT, -1);
                if (event_count < 0) {
                    std::cout << "epoll failed while waiting!" << std::endl;
                    exit(1);
                }
            
                for (int i = 0; i < event_count; i++) {
                    int event_fd = event[i].data.fd;
                    if (event_fd == server_socket) {
                        accept_client();
                    } else if (event[i].events & EPOLLIN ) {
                        handle_client(event_fd);
                    }
                }
            }
        }

        // This method will stop the server 
        void stop(){
            close(epoll_fd);
            close(server_socket);
            isActive = false;
            std::cout << "Stopped!!" << std::endl;
        }

        // Destructor to free up the resources
        ~Server() {
            this->stop();
        }
    private:
        // This method make the server non blocking
        void set_non_blocking(int server_fd){
            int flags = fcntl(server_fd, F_GETFL, 0);
            fcntl(server_fd, F_SETFL, flags |= O_NONBLOCK);
        }

        // A method which will accepts the clients if they don't exist previously
        void accept_client() {
            struct sockaddr_in client_address;
            while (isActive) {
                socklen_t client_address_length = sizeof(client_address);
                int client_socket = accept(this->server_socket, (struct sockaddr*)&client_address, &client_address_length);
                if (client_socket == -1) {                    
                    if (client_socket == EAGAIN || client_socket == EWOULDBLOCK  ) {
                        return;
                    }
                    std::cout << "failed to accept connections!" << std::endl;
                    return;
                }
            
                set_non_blocking(client_socket);
                struct epoll_event event;
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
            
                char ip[16];
                inet_ntop(AF_INET, &client_address.sin_addr, ip, 16);
                client_ips[client_socket] = std::string(ip);
            
                std::cout << "Client connected: " << ip << std::endl;
            }
        }

        // A method which will accepts the client requests if they are previously connected
        void handle_client(int client_fd) {
            struct sockaddr_in client_address;
            char read_buffer[2048] = {};
            long int bytes_recieved = recv(client_fd, read_buffer, sizeof(read_buffer), 0);
        
            char ip[16];
                inet_ntop(AF_INET, &client_address.sin_addr, ip, 16);
                client_ips[client_fd] = std::string(ip);
        
            // Will disconnect the client
            if (bytes_recieved <= 0) {
                std::cout << "Client disconnected: " << ip << std::endl;
                close(client_fd);
            }
        
            std::string command(reinterpret_cast<char*>(read_buffer), bytes_recieved);
            std::vector <std::string> array_of_token = parse_command(command);
            std::string processed_commnd = process_command(command);
            send_response(client_fd, processed_commnd);
            array_of_token = {};
        }

        // Methord that will parse the command when if there is no a one line output
        std::vector<std::string> parse_command(const std::string command){
            std::vector<std::string> tokens;
            std::stringstream t(command);
            std::string token;
            while (t >> token) {
                tokens.push_back(token);
            }
            return tokens;
        }

        // Method which will process the command.
        std::string process_command(std::string command){
            if (command == "hola")  {
                return "AMIGO!";
            } else if (command == "get" || command == "GET") {
                return "GET";
            } else if (command == "set" || command == "SET") {
                return "SET";
            } else {
                return "command not found";
            }
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