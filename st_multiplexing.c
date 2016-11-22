
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>
#include <sys/epoll.h>
#include <errno.h>

#define MAX_CLINETS 1000
//select
static int clientfds[MAX_CLINETS] = { 0 };
static fd_set readfds;
static fd_set writefds;
//poll
static struct pollfd	pollfds[MAX_CLINETS] = { 0 };
static int  pollfds_count = 1;
//epoll
static int epfd = -1;
static struct epoll_event epollevs[MAX_CLINETS] = { 0 };


static int sockfd = -1;
static char buffer[256] = { 0 };

static const char *SLEEP_MSG = "sleep 1 second....\n";
static const int DELAY_MS = 500;


void error(char *msg)
{
    perror(msg);
    exit(1);
}

void select_request_process()
{
    //检查客户端socket是否可读（客户端是否发送消息）
    for (int i = 0; i < MAX_CLINETS; i++)
    {
        if (clientfds[i] <= 0) continue;
        if (FD_ISSET(clientfds[i], &readfds))
        {
            bzero(buffer, 256);
            int n = read(clientfds[i], buffer, 255);
            if (n < 0)
            {
                perror("ERROR reading from socket");
                close(clientfds[i]);
                clientfds[i] = -1; //从列表中清除
                continue;
            }
            time_t cur = time(0);
            struct tm *ptm = localtime(&cur);
            printf("%s:", asctime(ptm));

            printf("msg: %s\n", buffer);
            //读操作完成后，马上给客户端回应
            if (FD_ISSET(clientfds[i], &writefds))
            {
                int n = write(clientfds[i], "I got your message", 18);
                if (n < 0) error("ERROR writing to socket");
            }
        }

    }
    //检查是否有新的客户端连接
    if (FD_ISSET(sockfd, &readfds))
    {

        struct sockaddr_in cli_addr;
        int clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        for (int i = 0; i < MAX_CLINETS; i++)
        {
            if (clientfds[i] <= 0)
            {
                clientfds[i] = newsockfd;
                break;
            }

        }
    }

}

void select_eventloop()
{
    printf("--------staring server with select mode------------\n");
    struct timeval timeout;
    while (1)
    {
        //清空读、写文件描述符集合
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        //计算最大文件描述符
        int max_fd = sockfd;
        //初始化读、写文件描述符集合
        FD_SET(sockfd, &readfds);
        for (int i = 0; i < MAX_CLINETS; i++)
        {
            if (clientfds[i] > 0)
            {
                FD_SET(clientfds[i], &readfds);
                FD_SET(clientfds[i], &writefds);
            }
            max_fd = max_fd > clientfds[i] ? max_fd : clientfds[i];
        }
        //构造超时结构体
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * DELAY_MS;

        int ret = select(max_fd + 1, &readfds, &writefds, 0, &timeout);
        if (ret == -1)
        {
            error("select:");
        }
        select_request_process();
    }
}

void poll_request_process()
{
    //检查客户端socket是否可读（客户端是否发送消息）
    for (int i = 1; i < pollfds_count; i++)
    {
        if (pollfds[i].revents & POLLIN)
        {
            bzero(buffer, 256);
            int n = read(pollfds[i].fd, buffer, 255);
            if (n < 0)
            {
                perror("ERROR reading from socket");
                close(pollfds[i].fd);
                pollfds[i].fd = -1; //从列表中清除
                memcpy(&pollfds[i], &pollfds[pollfds_count - 1], sizeof(struct pollfd));
                i--;
                pollfds_count--;
                continue;
            }
            time_t cur = time(0);
            struct tm *ptm = localtime(&cur);
            printf("%s:", asctime(ptm));

            printf("msg: %s\n", buffer);
            //读操作完成后，马上给客户端回应
            if (pollfds[i].revents & POLLOUT)
            {
                int n = write(pollfds[i].fd, "I got your message", 18);
                if (n < 0) error("ERROR writing to socket");
            }
        }

    }
    //检查是否有新的客户端连接
    if (pollfds[0].revents & POLLIN)
    {

        struct sockaddr_in cli_addr;
        int clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        for (int i = 0; i < MAX_CLINETS; i++)
        {
            if (pollfds[i].fd <= 0)
            {
                pollfds[i].fd = newsockfd;
                pollfds[i].events = POLLIN | POLLOUT;
                pollfds_count++;
                break;
            }

        }
    }

}

void poll_eventloop()
{
    printf("--------staring server with poll mode------------\n");
    pollfds[0].fd = sockfd;
    pollfds[0].events = POLLIN;
    while (1)
    {
        int ret = poll(pollfds, pollfds_count, DELAY_MS);
        if (ret == -1)
        {
            error("select:");
        }
        if (ret == 0)
        {
            continue;
        }
        poll_request_process();
    }

}

void epoll_request_process(int ready)
{
    for (int i = 0; i < ready; i++)
    {
        if (epollevs[i].events & EPOLLIN)
        {
            if (epollevs[i].data.fd == sockfd)
            {
                struct sockaddr_in cli_addr;
                int clilen = sizeof(cli_addr);
                int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd < 0) error("ERROR on accept");
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = newsockfd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, newsockfd, &ev) == -1)
                {
                    perror("epoll_ctl:");
                }
            } else
            {
                bzero(buffer, 256);
                int n = read(epollevs[i].data.fd, buffer, 255);
                if (n < 0)
                {
                    perror("ERROR reading from socket");

                    epoll_ctl(epfd, EPOLL_CTL_DEL, epollevs[i].data.fd, 0);

                    close(epollevs[i].data.fd);

                    continue;
                }
                time_t cur = time(0);
                struct tm *ptm = localtime(&cur);
                printf("%s:", asctime(ptm));

                printf("msg: %s\n", buffer);
                //读操作完成后，马上给客户端回应

                n = write(epollevs[i].data.fd, "I got your message", 18);
                if (n < 0) error("ERROR writing to socket");

            }
        }
    }

}
void epoll_eventloop()
{
    printf("--------staring server with epoll mode------------\n");
    epfd = epoll_create1(0);
    if (epfd == -1)
    {
        error("epoll_create1:");
    }

    epollevs[0].events = EPOLLIN;
    epollevs[0].data.fd = sockfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &epollevs[0]) == -1)
    {
        error("epoll_ctl:");
    }

    while (1)
    {
        int ret = epoll_wait(epfd, epollevs, MAX_CLINETS, DELAY_MS);
        if (ret == -1)
        {
            if (errno == EINTR) continue;
            error("epoll_wait:");
        }
        epoll_request_process(ret);
    }

}



int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided \n"); /*or server type specified*/
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) error("ERROR opening socket");
    struct sockaddr_in serv_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));
    int portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0) error("ERROR on binding");
    listen(sockfd, 5);

    int server_type = 0;
    if (argc == 3) server_type = atoi(argv[2]);
    switch (atoi(argv[2]))
    {
    case 0:
        select_eventloop();
        break;
    case 1:
        poll_eventloop();
        break;
    case 2:
        epoll_eventloop();
        break;
    default:
        break;
    }

    close(sockfd);

    return 0;
}
