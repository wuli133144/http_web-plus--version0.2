

#include "../../inc/sock.h"
#include "../../inc/epoll_inc.h"
#include "../../http_module/http_module.h"
#include "../../utils/utils.h"

int start(int argc, char **argv)
{

    /*@socket description@*/
    int listenfd;
    int clientfd;
    socklen_t client_len;
    struct sockaddr_in clientsock;
    /*@socket description@*/

    /*@epoll var@*/

    int epfd;
    int i;
    struct epoll_event *pevent;
    struct epoll_event event;
    char buf[BUFSIZE];
        
    /*@epollar @*/
    
       /*@signal handler@*/
       signal(SIGPIPE,SIG_IGN);
       signal(SIGCHILD,SIG_DFL);
       /*@signal hander end@*/

    /*@get socket filefd@*/
    listenfd = open_listenfd(sockpair.ip_addr, sockpair.port);
    Setnoblock(listenfd, O_NONBLOCK);
    /*@get socket filefd end@*/

    /*@epoll @*/
    bzero(buf, BUFSIZE);
    event.data.fd = listenfd;
    event.events = EPOLLIN;

    pevent = (struct epoll_event *)Malloc(BUFSIZE * sizeof(struct epoll_event));

    epfd = Epoll_create(BUFSIZE);

    Epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);
    /*@epoll end@*/

    while (1)
    {

        int event_cnt = Epoll_wait(epfd, pevent, BUFSIZE, -1);
        for (i = 0; i < event_cnt; i++)
        {
            if (pevent[i].events & EPOLLERR || pevent[i].events & EPOLLHUP)
            {
                free(pevent);
                close(listenfd);
                unix_error("there is something wrong with socketfd");
            }
            else if (pevent[i].data.fd == listenfd)
            {
                while (1)
                {

                    clientfd = Accept(listenfd, (struct sockaddr *)&clientsock, (socklen_t *)&client_len);
                    Setnoblock(clientfd, O_NONBLOCK);

                    event.events = EPOLLIN | EPOLLET; //
                    event.data.fd = clientfd;
                    Epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &event); //add read event
                    printf("connect fd=%d\n", clientfd);

                }
                continue;
            }
            else if (pevent[i].events & EPOLLIN)
            {   //Read
                //int read_cnt=0;
                //dup->http_module.c
                //http_module_handler_request(pevent[i].data.fd);
                //clientfd
                event.events = EPOLLOUT|EPOLLET; //
                event.data.fd = clientfd;
                Epoll_ctl(epfd, EPOLL_CTL_MOD, clientfd, &event);

            }
            else if (pevent[i].events & EPOLLOUT)
            { //Write
                       
                http_module_handler_response(pevent[i].data.fd);
                event.events = EPOLLOUT|EPOLLET; //
                event.data.fd = clientfd;
                Epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &event);

            }
            else
            { //default
                break;
            }
        }

        free(pevent);
        close(listefd);
        close(clientfd);

        return 0;
    }