#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "inf141302_struct.h"

struct user
{
    char login[64];
    char pswd[64];
    int msg;
    int failed_login;
};

struct group
{
    char name[64];
    struct user *tab[10];
};

int main()
{
    signal(SIGCLD, SIG_IGN);
    struct user all[100];
    struct group grp[10];
    
    struct user *logged_in[100] = { NULL };
    struct user *blocked[100] = { NULL };

    int fd = open("config.txt", O_RDONLY);
    if(fd != -1)
    {
        char n, buf[64] = { 0 };
        int k = 0;
        read(fd, &n, 1);
        do
        {
            buf[k] = n;
            k++;
            read(fd, &n, 1);

        } while (n != '\n');

        int max = atoi(buf);
                
        for(int i = 0; i < max; i++)
        {
            read(fd, &n, 1);
            k = 0;
            while(n != '\n')
            {
                buf[k] = n;
                k++;

                read(fd, &n, 1);
            }
            buf[k] = '\0';
            strcpy(grp[i].name, buf);
            
        }

        read(fd, &n, 1);
        k = 0;
        while (n != '\n')
        {
            buf[k] = n;
            k++;
            read(fd, &n, 1);
        }
        max = atoi(buf);

        for(int i = 0; i < max; i++)
        {
            k = 0;
            read(fd, &n, 1);
            while (n != ' ')
            {
                buf[k] = n;
                k++;
                read(fd, &n, 1);
            }
            buf[k] = '\0';
            strcpy(all[i].login, buf);
            all[i].failed_login = 0;
            
            k = 0;
            read(fd, &n, 1);
            while (n != '\n')
            {
                buf[k] = n;
                k++;
                read(fd, &n, 1);
            }
            buf[k] = '\0';
            strcpy(all[i].pswd, buf);
        }
    }
    int pub_queue = msgget(0x997, 0666 | IPC_CREAT);
    struct login request;
    struct one_line one;
    struct two_line group_reqest;
    struct group_tab names_request;
    struct message message_rec;
    
    while(1)
    {
        if( msgrcv(pub_queue, &request, 132, 1, IPC_NOWAIT) != -1) //handling login requests
        {
            struct success tmp;
            tmp.mtype = 1;
            tmp.com = -1;
            for(int i = 0; i < 100; i++)
            {
                if(blocked[i] != NULL && strcmp(request.log, blocked[i]->login) == 0)
                {
                    tmp.com = LOGIN_ATTEMPTS_ALLOWED;
                    break;
                }
            }
            if(tmp.com == -1)
            {
                for(int i = 0; i < 100; i++)
                {
                    if(logged_in[i] != NULL && strcmp(request.log, logged_in[i]->login) == 0)
                    {
                        tmp.com = LOGIN_ATTEMPTS_ALLOWED + 1;
                        break;
                    }
                }
            }
            if(tmp.com == -1)
            {
                for(int i = 0; i < 100; i++)
                {
                    if(strcmp(request.log,all[i].login) == 0)
                    {
                        if(strcmp(request.pwd, all[i].pswd) == 0)
                        {
                            tmp.com = 0;
                            all[i].failed_login = 0;
                            all[i].msg = request.id;
                            for(int j = 0; j < 100; j++)
                            {
                                if(logged_in[j] == NULL)
                                {
                                    logged_in[j] = &all[i];
                                    break;
                                }
                            }
                            
                        }
                        else
                        {
                            all[i].failed_login += 1;
                            tmp.com = all[i].failed_login;
                            if(all[i].failed_login == LOGIN_ATTEMPTS_ALLOWED)
                            {
                                for(int j = 0; j < 100; j++)
                                {
                                    if(blocked[j] == NULL)
                                    {
                                        blocked[j] = &all[i];
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                    }
                    
                }
            }
            
            msgsnd(request.id, &tmp, sizeof(int), 0);
        }
        if(msgrcv(pub_queue, &one, 256, 2, IPC_NOWAIT) != -1) // handling log out requests
        {
            for(int i = 0; i < 100 && logged_in[i] != NULL; i++)
            {
                if(strcmp(one.txt, logged_in[i]->login) == 0)
                {
                    logged_in[i]->msg = 0;
                    logged_in[i] = NULL;
                }
            }
        }
        if(msgrcv(pub_queue, &one, 256, 3, IPC_NOWAIT) != -1) //handling requests for sending list of logged in users
        {
            if(fork() == 0)
            {
                for(int i = 0; i < 100; i++)
                {
                    if(logged_in[i] != NULL && strcmp(one.txt, logged_in[i]->login) == 0)
                    {
                        int k = 0;
                        for(int j = 0; j < 100; j++)
                        {
                            if(logged_in[j] != NULL)
                            {
                                strcpy(names_request.tab[k], logged_in[j]->login);
                                k++;
                            }
                            
                        }
                        names_request.mtype = 3;
                        msgsnd(logged_in[i]->msg, &names_request, 640, 0);
                        exit(0);
                    }
                }
            }
        }
        if(msgrcv(pub_queue, &group_reqest, 128, 4, IPC_NOWAIT) != -1) //handling requests for list of group members
        {
            if(fork() == 0)
            {
                int msg = -1;
                for(int i = 0; i < 100; i++)
                {
                    if(logged_in[i] != NULL && strcmp(group_reqest.usr_name, logged_in[i]->login) == 0)
                    {
                        msg = logged_in[i]->msg;
                        break;
                    }
                }
                if(msg != -1)
                {
                    struct group_tab tmp;
                    tmp.mtype = 4;
                    int i;
                    for(i = 0; i < 10; i++)
                    {
                        if(strcmp(group_reqest.msg, grp[i].name) == 0) break;
                    }
                    if(i == 10)
                    {
                        strcpy(tmp.tab[0], "Couldn't find group");
                    }
                    else
                    {
                        int k = 0;
                        for(int j = 0; j < 10; j++)
                        {
                            if(grp[i].tab[j] != NULL)
                            {
                                strcpy(tmp.tab[k], grp[i].tab[j]->login);
                                k++;
                            }
                            
                        }
                    }
                    
                    
                    msgsnd(msg, &tmp, 640, 0);
                }
                exit(0);
            }
        }
        if(msgrcv(pub_queue, &group_reqest, 128, 5, IPC_NOWAIT) != -1) //handling requests for joining a group
        {
            struct user *tmp = NULL;
            for(int i = 0; i < 100; i++)
            {
                if(logged_in[i] != NULL && strcmp(group_reqest.usr_name, logged_in[i]->login) == 0)
                {
                    tmp = logged_in[i];
                    break;
                }
            }
            if(tmp != NULL)
            {   
                struct success s;
                s.mtype = 5;
                s.com = 1;
                int i;
                for(i = 0; i < 10; i++)
                {
                    if(strcmp(group_reqest.msg, grp[i].name) == 0) break;
                }
                if(i == 10)
                {
                    s.com = 0;
                }
                else
                {
                    for(int j = 0; j < 10; j++)
                    {
                        if(grp[i].tab[j] == NULL)
                        {
                            grp[i].tab[j] = tmp;
                            break;
                        }
                        else if(grp[i].tab[j] == tmp)
                        {
                            s.com = 0;
                            break;
                        }
                    }
                }
                msgsnd(tmp->msg, &s, sizeof(int), 0);
            }
        }
        if(msgrcv(pub_queue, &group_reqest, 128, 6, IPC_NOWAIT) != -1) //handling requests for leaving a group
        {
            struct user *tmp = NULL;
            for(int i = 0; i < 100; i++)
            {
                if(logged_in[i] != NULL && strcmp(group_reqest.usr_name, logged_in[i]->login) == 0)
                {
                    tmp = logged_in[i];
                    break;
                }
            }
            if(tmp != NULL)
            {
                int i;
                struct success s;
                s.mtype = 6;
                s.com = 1;
                for(i = 0; i < 10; i++)
                {
                    if(strcmp(group_reqest.msg, grp[i].name) == 0) break;
                }
                if(i == 10)
                {
                    s.com = 0;
                }
                else
                {
                    int j;
                    for(j = 0; j < 10; j++)
                    {
                        if(grp[i].tab[j] == tmp)
                        {
                            grp[i].tab[j] = NULL;
                            break;
                        }
                    }
                    if(j == 10)
                    {
                        s.com = 0;
                    }
                }
                msgsnd(tmp->msg, &s, sizeof(int), 0);
            }
        }
        if(msgrcv(pub_queue, &one, 256, 7, IPC_NOWAIT) != -1) //handling requests for sending groups list
        {
            if(fork() == 0)
            {
                int msg = -1;
                for(int i = 0; i < 100; i++)
                {
                    if(logged_in[i] != NULL && strcmp(one.txt, logged_in[i]->login) == 0)
                    {
                        msg = logged_in[i]->msg;
                        break;
                    }
                }
                if(msg != -1)
                {
                    struct group_tab tab;
                    tab.mtype = 7;
                    for(int i = 0; i < 10 && strlen(grp[i].name) != 0; i++)
                    {
                        strcpy(tab.tab[i], grp[i].name);
                    }
                    msgsnd(msg, &tab, 640, 0);
                }
                exit(0);
            }
        }
        if(msgrcv(pub_queue, &message_rec, 384, 8, IPC_NOWAIT) != -1) //sending message to a group
        {
            if(fork() == 0)
            {
                struct success s;
                s.mtype = 8;
                s.com = 1;
                int i, j, q_id;
                for(i = 0; i < 10; i++)
                {
                    if(strcmp(message_rec.receiver, grp[i].name) == 0)
                    {
                        for(j = 0; j < 10; j++)
                        {
                            if(grp[i].tab[j] != NULL && strcmp(message_rec.sender, grp[i].tab[j]->login) == 0)
                            {
                                q_id = grp[i].tab[j]->msg;
                                break;
                            }
                        }
                        break;
                    }
                }
                
                if(i != 10 && j != 10)
                {
                    message_rec.mtype = 10;
                    for(int j = 0; j < 10; j++)
                    {
                        if(grp[i].tab[j] != NULL && grp[i].tab[j]->msg != 0)
                        {
                            if(strcmp(message_rec.sender, grp[i].tab[j]->login) != 0)
                            {
                                msgsnd(grp[i].tab[j]->msg, &message_rec, 384, 0);
                            }
                        }
                    }
                }
                else
                {
                    s.com = 0;
                    for(j = 0; j < 100; j++)
                    {
                        if(logged_in[j] != NULL && strcmp(message_rec.sender, logged_in[j]->login) == 0)
                        {
                            q_id = logged_in[j]->msg;
                        }
                    }
                }
                msgsnd(q_id, &s, sizeof(int), 0);

                exit(0);
            }
        }
        if(msgrcv(pub_queue, &message_rec, 384, 9, IPC_NOWAIT) != -1) //sending message to user
        {
            if(fork() == 0)
            {
                struct success s;
                s.mtype = 9;
                s.com = 1;
                
                int msg = -1, q_id = -1, i;
                for(i = 0; i < 100; i++)
                {
                    if(logged_in[i] != NULL && strcmp(message_rec.receiver, logged_in[i]->login) == 0)
                    {
                        msg = logged_in[i]->msg;
                        if(q_id != -1) break;
                    }
                    if(logged_in[i] != NULL && strcmp(message_rec.sender, logged_in[i]->login) == 0)
                    {
                        q_id = logged_in[i]->msg;
                        if(msg != -1) break;
                    }
                }
                if(i == 100)
                {
                    s.com = 0;
                }
                else
                {
                    message_rec.mtype = 10;
                    msgsnd(msg, &message_rec, 384, 0);
                }
                
                msgsnd(q_id, &s, sizeof(int), 0);
                exit(0);
            }
        }
        if(msgrcv(pub_queue, &group_reqest, 128, 10, IPC_NOWAIT) != -1 || msgrcv(pub_queue, &group_reqest, 128, 11, IPC_NOWAIT) != -1) //handling requests for blocking user or group
        {
            
            int msg = -1, found = -1;
            for(int i = 0; i < 100; i++)
            {
                if(logged_in[i] != NULL && strcmp(group_reqest.usr_name, logged_in[i]->login) == 0)
                {
                    msg = logged_in[i]->msg;
                    if(found != -1) break;
                }
                if(group_reqest.mtype == 10 && strcmp(all[i].login, group_reqest.msg) == 0)
                {
                    found = 1;
                    if(msg != -1) break;
                }
                if(group_reqest.mtype == 11 && i < 10 && strcmp(grp[i].name, group_reqest.msg) == 0)
                {
                    found = 1;
                    if(msg != -1) break;
                }
            }
            if(found == -1)
            {
                strcpy(group_reqest.msg, "Fail");
            }
            group_reqest.mtype = 12;
            msgsnd(msg, &group_reqest, 128, 0);
        }
    }

    return 0;
}