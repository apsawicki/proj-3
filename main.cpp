#include"main.h" // header file
#include<unistd.h> // read and write to fd
#include<string> // strings
#include<pthread.h> // threads
#include<string.h> // for bzero() to clean strings
#include<arpa/inet.h> // for definitions like AF_INET, SOCK_STREAM etc...
#include<queue> // for connections buffer priority queue
#include<algorithm> // to help with removing whitespaces from a string
#include<vector> // need vector for the connections buffer priority queue
#include<chrono> // implementing time
//#include<sys/socket.h>
//#include<cstdlib>
//#include<netinet/in.h>
//#include<iostream> // I/O
#define DEFAULT_PORT 2348
#define DEFAULT_DICT user/share/dict/words
using namespace std;


// queue<CONNECTION> connectionsSequential; // if user wants sequential buffer
priority_queue<CONNECTION, vector<CONNECTION>, my_compare> connectionsBuffer; // if user wants priority buffer
queue<CONNECTION> loggerQueue;

bool sequentialBuffer; // lets the threads know whether the user chose sequential or priority buffer
int connectionSize;


[[noreturn]] void serverThreadFunc(){

    struct sockaddr_in serverAdd, clientAdd;
    int listen_socket, accept_socket;


    if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) >= 0){ // create the socket file descriptor
        printf("server socket created\n");
    }
    else{
        perror("SOCKET CREATION FAILURE\n");
        exit(EXIT_FAILURE);
    }

    int opt = 1; // TODO: I saw online that this was supposed to reduce/prevent binding errors, however I need to test it to make sure it is something useful/doesn't break my program
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


    if (bind(listen_socket, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == 0){ // Attaches socket to the port and address in sockaddr_in struct
        printf("bind success\nPORT: %i\nIP: %s\n", DEFAULT_PORT, "localhost");
    }
    else{
        perror("BIND FAILURE\n");
        exit(EXIT_FAILURE);
    }


    if (listen(listen_socket, 5) == 0){ // puts the server socket in listen mode, waits for client to approach the server for a connection
        printf("server listening\n\n");
    }
    else{
        perror("SERVER LISTEN FAILURE\n");
        exit(EXIT_FAILURE);
    }


    socklen_t len = sizeof(clientAdd);
    while(true){ // the server will continuously wait for clients to connect and then add them to the queue
        accept_socket = accept(listen_socket, (struct sockaddr*)&clientAdd, &len);
        if (accept_socket >= 0){
            printf("client detected\n");

            pthread_mutex_lock(&connectionMTX);
            CONNECTION con{accept_socket, 0, 0, "", true, 0}; // TODO: requestArrivalTime for CONNECTION con

            while(connectionsBuffer.size() == (unsigned int) connectionSize){
                pthread_cond_wait(&emptySpace, &connectionMTX);
            }

            if (sequentialBuffer){
                connectionsBuffer.push(con);
            }
            else{
                con.priority = (rand() % 10) + 1;
                connectionsBuffer.push(con);
            }

            printf("client accepted - fd: %i\n", accept_socket);

            pthread_cond_signal(&nonEmptyQueue);
            pthread_mutex_unlock(&connectionMTX);
        }
    }

} // Main Server Thread (Producer) - doesn't need locking mechanisms since it is the only thread that will produce for it's consumers

[[noreturn]] void *loggerThreadFunc(void *arg){
    // TODO: take logs from the log buffer: The log buffer entry is to contain the arrival time of the request, the time the spell check was completed, the word checked, the result of the spell check and the priority of the request.
    // TODO: and send them to a log file
    while (true){

    }
    return nullptr;
}

[[noreturn]] void *workerThreadFunc(void *arg){
    while (true){

        CONNECTION client;
        int client_fd = client.clientFD;
        char buff[80]; // message maximum length is 80
        string serverMessage;


        pthread_mutex_lock(&connectionMTX);

        while (connectionsBuffer.empty()){
            pthread_cond_wait(&nonEmptyQueue, &connectionMTX); // wait until the connection queue has something in it and then we will start working
        }

        client = connectionsBuffer.top();
        connectionsBuffer.pop();

        pthread_cond_signal(&emptySpace);
        pthread_mutex_unlock(&connectionMTX);



        // read client message
        read(client_fd, buff, sizeof(buff));
        serverMessage = string(buff);
        serverMessage.erase(remove(serverMessage.begin(), serverMessage.end(), ' '), serverMessage.end()); // should remove all whitespace from the string
        bzero(buff, 80);

        printf("Client: %i : (%s)\n", client_fd, serverMessage.c_str());
        // if client message is exit then disconnect from the client
//        if (serverMessage == "exit"){
//            printf("Client: %i - exit\n", client_fd);
//            printf("closing communications with client_fd: %i\n", client_fd);
//            string goodbye = "closing communications with server\n";
//            write(client_fd, goodbye.c_str(), sizeof(goodbye.c_str()));
//            close(client_fd);
//            break; // will break and wait for the next
//        }

        // TODO: check dictionary if it is a correctly spelled word
        // TODO: add log to the logger queue

        // write spell checked word to client
        write(client_fd, serverMessage.c_str(), sizeof(serverMessage.c_str()));

    }
    return nullptr;
} // Worker Threads (Consumer)

void spawnThreads(int workerThreadCount){

    // create logger thread
    pthread_t loggerThread;
    if(pthread_create(&loggerThread, nullptr, loggerThreadFunc, nullptr) != 0){
        printf("Error: failed to create logger thread\n");
        exit(EXIT_FAILURE);
    }

    // create worker threads
    pthread_t workerThreads[workerThreadCount];
    for (int i = 0; i < workerThreadCount; i++){
        if (pthread_create(&workerThreads[i], nullptr, workerThreadFunc, nullptr) != 0){
            printf("Error: failed to create worker thread\n");
            exit(EXIT_FAILURE);
        }
    }
} // Creates the worker and logger threads


int main(int argc, char** argv){

    // TODO: implement chrono time for log info, add log into the log queue
    srand(0); // TODO: seed this in respect to time
    // these values are going to be set by the user through the argv[] arguments
    sequentialBuffer = true; // using either the sequential or priority queue
    connectionSize = 10; // total connections allowed
    int workerThreadCount = 10; // total worker threads allowed


    spawnThreads(workerThreadCount); // spawns worker + log threads
    serverThreadFunc(); // server thread starts


    return 0;
}
