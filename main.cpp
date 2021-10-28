#include<unistd.h>
//#include<sys/socket.h>
//#include<cstdlib>
//#include<netinet/in.h>
#include<iostream> //i/o
#include<string> // strings
#include<pthread.h> // threads
#include <string.h> // for bzero()
#include <arpa/inet.h>
#define PORT 2348
using namespace std;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; // lock to make a critical section
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // queue for the threads to wait in

int done = 0;

void chat(int client_fd){
    pthread_mutex_lock(&mtx);
    int maxCharCount = 80;
    char buff[maxCharCount];
    int n;
    // infinite loop for chat
    while (true){
        bzero(buff, maxCharCount);

        // read the message from client and copy it in buffer
        read(client_fd, buff, sizeof(buff));
        // print buffer which contains the client contents
        printf("Received: %sSend: ", buff);
        bzero(buff, maxCharCount);
        n = 0;
        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n')
            ;

        // and send that buffer to client
        write(client_fd, buff, sizeof(buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            done = 1;
            pthread_cond_signal(&cond);
            break;
        }
    }

    pthread_mutex_unlock(&mtx);
}

void *socketCreation(void *arg){



    struct sockaddr_in serverAdd, clientAdd;
    int listen_socket, accept_socket;


    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0){ // create the socket file descriptor
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    else{
        printf("socket created\n");
    }

    int opt = 1; // this if statement will prevent errors
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    else{
        printf("socket options set\n");
    }

    bzero(&serverAdd, sizeof(serverAdd));
    serverAdd.sin_family = AF_INET; // Internet Protocol
    serverAdd.sin_addr.s_addr = htonl(INADDR_ANY); // I think this sets the address to localhost
    serverAdd.sin_port = htons(2348); // PORT 8080


    if (bind(listen_socket, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) != 0){ // Attaches socket to the port and address in sockaddr_in struct

        // you cant bind if the address is already in use
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    else{
        printf("bind success\nPORT: %i\nIP: %s\n", PORT, "localhost");
    }


    if (listen(listen_socket, 5) < 0){ // puts the server socket in listen mode, waits for client to approach the server for a connection

        perror("listen");
        exit(EXIT_FAILURE);
    }
    else{
        printf("server listening\n");
    }

    socklen_t len = sizeof(clientAdd);
    accept_socket = accept(listen_socket, (struct sockaddr*)&clientAdd, &len);

    while (accept_socket < 0){
        accept_socket = accept(listen_socket, (struct sockaddr*)&clientAdd, &len);

    }
    printf("client accepted - client_fd: %i\n", accept_socket);

//    if (accept_socket < 0){
//        printf("client accept failure\n");
//        exit(EXIT_FAILURE);
//    }
//    else{
//        printf("client accepted - client_fd: %i\n", accept_socket);
//    }


    chat(accept_socket);


    return nullptr;
}

void *workerThreads(void *arg){

//    printf("workerThread");
//    struct sockaddr_in serv_addr;
//    string hello = "Hello from client";
//    char buffer[1024];
//
//    int sock = 0;
//    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
//    {
//        printf("\n Socket creation error \n");
//        return nullptr;
//    }
//
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_port = htons( 8080 );
//
//    // Convert IPv4 and IPv6 addresses from text to binary form
//    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
//    {
//        printf("\nInvalid address/ Address not supported \n");
//        return nullptr;
//    }
//
//    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
//    {
//        printf("\nConnection Failed \n");
//        return nullptr;
//    }
//    int valread;
//    send(sock , hello.c_str() , hello.size() , 0 );
//    printf("Hello message sent\n");
//    valread = read( sock , buffer, 1024);
//    printf("%s\n",buffer );
//    printf("valread: %d", valread);

    return nullptr;
}

void spawnThreads(){

    pthread_t socketThread;

    if(pthread_create(&socketThread, nullptr, socketCreation, nullptr) != 0){
        printf("Error: failed to create client socket thread\n");
        exit(1);
    }

    while (done == 0){

        pthread_cond_wait(&cond, &mtx);
    }

//    pthread_t clientThread;
//
//    if(pthread_create(&clientThread, nullptr, workerThreads, nullptr) != 0){
//        printf("Error: failed to create worker client thread\n");
//        exit(1);
//    }
}



int main(){

    spawnThreads();




    return 0;
}
