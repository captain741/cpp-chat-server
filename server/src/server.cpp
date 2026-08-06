#include<iostream>
#include<string>
#include<cstring>
#include<thread>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<vector>
#include<mutex>
#include<algorithm>
#include<unordered_map>
// List of all currently online clients
std::vector<int> clients;
// Protect shared resources
std::mutex clients_mutex;
//FD->Name
std::unordered_map<int,std::string> client_names;
//-----------------------------------------------------------------
// Broadcast helper
void broadcast_message(const std::string& message,int sender_fd = -1){
    std::lock_guard<std::mutex> lock(clients_mutex);
    for(int fd : clients){
        // do not return sender
        if(fd == sender_fd){
            continue;
        }
        ssize_t sent= send(fd,message.c_str(),message.size(),0);
        if(sent < 0){
            perror("send");
        }
    }
}
//----------------------------------------------------------------------
//function run in thread for each client
void handle_client(int client_fd){
    // ask username
    const char* ask_name = "Enter your name: ";
    ssize_t s = send(client_fd,ask_name,strlen(ask_name),0);
    if(s <= 0){
        close(client_fd);
        return;
    }
    // Receive username
    char name_buffer[128] = {0};
    ssize_t n = recv(client_fd, name_buffer,sizeof(name_buffer)-1,0);
    if(n <= 0){
        close(client_fd);
        return;
    }
    name_buffer[n] = '\0';
    
    std::string username = name_buffer;
    // remove '\n'
    username.erase(std::remove(username.begin(),username.end(),'\n'),username.end());
    //remove '\r'
    username.erase(std::remove(username.begin(),username.end(),'\r'),username.end());
    if(username.empty()){
        close(client_fd);
        return;
    }
    // Login sucess -> Add into chat room
   {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(client_fd);
    client_names[client_fd] = username;
    std::cout << username << " joined. FD=" << client_fd << std::endl;
    std::cout << "Online clients: " << clients.size()<< std::endl;
    }
    //Notification everyone
    broadcast_message(username + " joined the room\n");
 // Chat loop
    // ----------------------------------------------

    char buffer[1024];

    while (true)
    {
        ssize_t bytes_received =
            recv(
                client_fd,
                buffer,
                sizeof(buffer) - 1,
                0);

        // Client disconnected
        if (bytes_received == 0)
        {
            std::cout
                << username
                << " disconnected"
                << std::endl;

            break;
        }

        // Error
        if (bytes_received < 0)
        {
            perror("recv");
            break;
        }

        buffer[bytes_received] = '\0';

        std::string message =
            "[" +
            username +
            "]: " +
            buffer;

        std::cout
            << message;

        broadcast_message(
            message,
            client_fd);
    }

    // ----------------------------------------------
    // Remove client
    // ----------------------------------------------

    {
        std::lock_guard<std::mutex> lock(clients_mutex);

        clients.erase(
            std::remove(
                clients.begin(),
                clients.end(),
                client_fd),
            clients.end());

        client_names.erase(client_fd);
    }

    // ----------------------------------------------
    // Notify leave
    // ----------------------------------------------

    broadcast_message(
        username + " left the room\n");

    close(client_fd);
}


int main(){
	std::cout << "Chat Server Starting..." << std::endl;
// create socket
int server_fd = socket(AF_INET, SOCK_STREAM, 0);

if(server_fd == -1){
    perror("socket");
    return 1;
}

// Resue port quickly
int opt = 1;
setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

sockaddr_in server_addr{};
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);
server_addr.sin_addr.s_addr = INADDR_ANY;
//Bind
int ret = bind(
    server_fd,
    reinterpret_cast<sockaddr*>(&server_addr),
    sizeof(server_addr));

if (ret == -1){
    perror("bind");
    close(server_fd);
    return 1;
}
    std::cout <<"Bind sucessful." << std::endl;

/*Convert socket state:
BOUND -> LISTENING
backlog = 5 pending connections
*/
if(listen(server_fd, 5) == -1){
    perror("listen");
    close(server_fd);
    return 1;
}
std::cout <<"Listening on port 8080..." << std::endl;
while(true) {
    std::cout<<"Waiting for  client..." <<std::endl;

    // Block here untill a client connects
    int client_fd = accept(server_fd,nullptr,nullptr);
    if(client_fd == -1){
        perror("accept");
        continue;
    }
    std::cout<<"New client connected. FD = "<< client_fd << std::endl;
    // add new client into vector
    std::thread t(handle_client,client_fd);
    t.detach();
    //Close client connection immediately
    // This is temporary for learning purposes
//    close(client_fd);
}   
close(server_fd);
    return 0;
}
