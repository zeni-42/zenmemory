#include <iostream>
#include <unistd.h>

int main(){
    system("g++ src/server.cpp -o server");
    system("g++ src/client.cpp -o client");
    return 0;
}