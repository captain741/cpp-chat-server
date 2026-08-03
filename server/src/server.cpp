#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>

int main(){
	std::cout << "Chat Server Starting..." << std::endl;

int server_fd = socket(AF_INET, SOCK_STREAM, 0);

if(server_fd == -1){
    perror("socket");
    return 1;
}
sockaddr_in server_addr{};
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);
server_addr.sin_addr.s_addr = INADDR_ANY;

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
    while(true){
    //Buffer used to receive data from client
    char buffer[1024] = {0};
    //Read data from TCP Stream
    ssize_t bytes_received = recv(client_fd,buffer,sizeof(buffer)-1,0);
    if(bytes_received > 0){
        buffer[bytes_received] = '\0';
        std::cout<<"Received " << buffer << std::endl;
        // Echo data for client
        ssize_t bytes_sent = send(client_fd,buffer,bytes_received,0);
        if(bytes_sent == -1){
            perror("send");
            break;
        }
    }else if(bytes_received == 0){
        std::cout<<" Client disconnect." <<std::endl;
        break;
    }
    else{
        perror("recv");
        break;
    }
}
    //Close client connection immediately
    // This is temporary for learning purposes
    close(client_fd);
}   
close(server_fd);
    return 0;
}
