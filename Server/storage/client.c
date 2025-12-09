
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define MAXLINE 4096
#define SEND_BUF 8192

// send a line (append '\n')
ssize_t send_line(int sock, const char *s){
    size_t len = strlen(s);
    ssize_t r = send(sock, s, len, 0);
    if(r != (ssize_t)len) return -1;
    r = send(sock, "\n", 1, 0);
    return r == 1 ? (ssize_t)(len + 1) : -1;
}

// read a line terminated by '\n' into buf (null-terminated, excludes newline)
ssize_t recv_line(int sock, char *buf, size_t maxlen){
    size_t i = 0;
    while(i < maxlen - 1){
        ssize_t r = recv(sock, buf + i, 1, 0);
        if(r == 1){
            if(buf[i] == '\n'){
                buf[i] = '\0';
                return (ssize_t)i;
            }
            i += 1;
            continue;
        }else if(r == 0){
            return -1; // closed
        }else{
            return -1; // error
        }
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

void strip_newline(char *s){
    size_t i = strlen(s);
    if(i>0 && s[i-1]=='\n') s[i-1] = '\0';
}

void to_basename(const char *path, char *out, size_t outlen){
    const char *p = strrchr(path, '/');
    if(!p) p = strrchr(path, '\\');
    if(p) strncpy(out, p+1, outlen-1);
    else strncpy(out, path, outlen-1);
    out[outlen-1] = '\0';
}

int main(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr, "Usage: %s ServerIP ServerPort\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){ perror("socket"); return 1; }
    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    serv.sin_addr.s_addr = inet_addr(server_ip);
    if(connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0){ perror("connect"); return 1; }

    char input[MAXLINE];
    while(1){
        printf("Nhap duong dan file: ");
        if(!fgets(input, sizeof(input), stdin)) break;
        strip_newline(input);
        if(strlen(input) == 0){
            printf("Good bye !\n");
            break;
        }
        // try open file
        FILE *f = fopen(input, "rb");
        if(!f){
            printf("Error: File not found\n");
            continue;
        }
        // get base name
        char fname[512];
        to_basename(input, fname, sizeof(fname));

        // send filename line
        if(send_line(sock, fname) < 0){ perror("send filename"); fclose(f); break; }

        // receive server response
        char resp[MAXLINE];
        if(recv_line(sock, resp, sizeof(resp)) <= 0){ perror("recv"); fclose(f); break; }
        if(strncmp(resp, "Error: File is existent on server", 33) == 0){
            printf("Error: File is existent on server\n");
            fclose(f);
            continue;
        }
        // expect OK
        if(strcmp(resp, "OK") != 0){
            printf("%s\n", resp);
            fclose(f);
            continue;
        }

        // compute file size
        fseek(f, 0, SEEK_END);
        unsigned long long filesize = ftell(f);
        rewind(f);

        // send filesize line
        char sizebuf[64];
        snprintf(sizebuf, sizeof(sizebuf), "%llu", (unsigned long long)filesize);
        if(send_line(sock, sizebuf) < 0){ perror("send size"); fclose(f); break; }

        // send file content
        unsigned long long sent = 0;
        char buf[SEND_BUF];
        int error = 0;
        while(sent < filesize){
            size_t toread = (filesize - sent) < sizeof(buf) ? (size_t)(filesize - sent) : sizeof(buf);
            size_t r = fread(buf, 1, toread, f);
            if(r == 0 && !feof(f)) { error = 1; break; }
            ssize_t s = send(sock, buf, r, 0);
            if(s <= 0){ error = 1; break; }
            sent += (unsigned long long)s;
        }
        fclose(f);
        if(error){ printf("Error: File tranfering is interupted\n"); break; }

        // receive final server response
        if(recv_line(sock, resp, sizeof(resp)) <= 0){ perror("recv"); break; }
        printf("%s\n", resp);
    }

    close(sock);
    return 0;
}
