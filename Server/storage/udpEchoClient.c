#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFF_SIZE 1024
#define SERV_PORT 9000
#define SERV_ADDR "127.0.0.1"

int main() {
    int sockfd, rcvBytes, sendBytes;
    socklen_t len;
    char buff[BUFF_SIZE+1]; 
struct sockaddr_in servaddr;
//Step 1: Construct socket
if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    perror("Error: ");
    return 0;
}
//Step 2: Define the address of the server
bzero(&servaddr, sizeof(servaddr)); 
servaddr.sin_family = AF_INET; 
inet_aton(SERV_ADDR, &servaddr.sin_addr); 
servaddr.sin_port = htons(SERV_PORT);
//Step 3: Communicate with server
printf("Send to server: ");
fgets(buff, BUFF_SIZE, stdin);
buff[strcspn(buff, "\n")] = 0;  // Remove newline
len = sizeof(servaddr);
sendBytes = sendto(sockfd, buff, strlen(buff), 0,
                   (struct sockaddr *) &servaddr, len);
if(sendBytes < 0){
    perror("Error: ");
    return 0;
}
rcvBytes = recvfrom(sockfd, buff, BUFF_SIZE, 0,
                    (struct sockaddr *) &servaddr, &len);
if(rcvBytes < 0){
    perror("Error: ");
    return 0;
}
buff[rcvBytes] = '\0';
printf("Reply from server: %s\n", buff);
    
    close(sockfd);
    return 0;
}