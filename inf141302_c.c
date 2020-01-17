#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "inf141302_struct.h"

void send_one_line(int msgq_id, long mtype, char *txt)
{
    struct one_line one;
    one.mtype = mtype;
    strcpy(one.txt, txt);
    msgsnd(msgq_id, &one, 256, 0);
}

void send_two_line(int msgq_id, long mtype, char *usr, char *txt)
{
    struct two_line two;
    two.mtype = mtype;
    strcpy(two.usr_name, usr);
    strcpy(two.msg, txt);
    msgsnd(msgq_id, &two, 128, 0);
}

void send_message(int msgq_id, long mtype, char *usr, int rcv_q)
{
    struct message msg;
    struct success s;
    char a[64];
    printf("Write name of receiver: ");
    fgets(a, 64, stdin);
    fgets(msg.receiver, 64, stdin);
    msg.receiver[strlen(msg.receiver) - 1] = '\0';
    strcpy(msg.sender, usr);
    msg.mtype = mtype;
    printf("Write your message:\n");
    fgets(msg.msg, 256, stdin);
    msgsnd(msgq_id, &msg, 384, 0);
    msgrcv(rcv_q, &s, sizeof(int), mtype, 0);
    if(s.com == 1)
    {
        printf("Successfuly sent message to %s\n\n", msg.receiver);
    }
    else
    {
        printf("Failed to send message\n\n");
    }
    
}

int main()
{
    srand(time(NULL));
    int pub_queue = msgget(0x997, 0666 | IPC_CREAT);
    struct login me;
    me.mtype = 1;
    me.id = msgget(rand(), 0666 | IPC_CREAT);
    struct success answer;
    
    do
    {
        printf("Hello to messenger. Please log in\nLogin: ");
        scanf("%s", me.log);
        printf("Password: ");
        scanf("%s", me.pwd);
        msgsnd(pub_queue, &me, 128 + sizeof(int), 0);
        msgrcv(me.id, &answer, sizeof(int), 1, 0);
        
        if(answer.com == -1)
        {
            printf("\nNo such user in database. Try again\n");
        }
        else if(answer.com == 0)
        {
            printf("\nHello %s!\n", me.log);
        }
        else if(answer.com < LOGIN_ATTEMPTS_ALLOWED)
        {
            printf("\nWrong password. Attempts left : %d\n", LOGIN_ATTEMPTS_ALLOWED - answer.com);
        }
        else if(answer.com == LOGIN_ATTEMPTS_ALLOWED)
        {
            printf("\nAccount blocked\n");
        }
        else if(answer.com == LOGIN_ATTEMPTS_ALLOWED + 1)
        {
            printf("\nUser already logged in!\n");
        }
        
        
        
    } while (answer.com != 0);
    
    pid_t pid;
    if((pid = fork()) == 0)
    {
        char blocked[100][64] = { { 0 } };
        struct message msg;
        struct two_line block;
        
        while(1)
        {
            if(msgrcv(me.id, &msg, 384, 10, IPC_NOWAIT) != -1)
            {
                int b = 0;
                for(int i = 0; i < 100; i++)
                {
                    if(strcmp(msg.receiver, blocked[i]) == 0)
                    {
                        b = 1;
                        break;
                    }
                    if(strcmp(msg.sender, blocked[i]) == 0)
                    {
                        b = 1;
                        break;
                    }
                }
                if(b == 0)
                {
                    printf("\n\nNew message from %s to ", msg.sender);
                    if(strcmp(msg.receiver, me.log) == 0)
                    {
                        printf("you\n");
                    }
                    else
                    {
                        printf("group %s\n", msg.receiver);
                    }
                    
                    printf("%s\n", msg.msg);
                }
                
            }
            if(msgrcv(me.id, &block, 128, 12, IPC_NOWAIT) != -1)
            {
                if(strcmp(block.msg, "Fail") == 0)
                {
                    printf("Failed to block user/group");
                }
                else
                {
                    int i = 0;
                    while (strlen(blocked[i]) != 0)
                    {
                        i++;
                    }
                    strcpy(blocked[i], block.msg);
                }
                
            }
            sleep(1);
        }
    }
    else
    {
        int choice = 0;
        struct group_tab names_list;
        struct success s;
        char group[64] , tmp[64];
    
        while (choice != 12)
        {
            printf("Select option (1 for help) : ");
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                printf( "2 - log out\n3 - show list of logged in users\n"
                        "4 - show members of chosen group\n5 - join chosen group\n"
                        "6 - leave chosen group\n7 - show list of available groups\n"
                        "8 - send message to chosen group\n9 - send message to chosen user\n"
                        "10 - block messages from user\n11 - block messages from group\n"
                        "12 - exit\n");
                break;
            case 2:
                send_one_line(pub_queue, choice, me.log);
                msgctl(me.id, IPC_RMID, NULL);
                kill(pid, SIGKILL);
                execl("inf141302_c", "", NULL);
                break;
            case 3:
                send_one_line(pub_queue, choice, me.log);
                msgrcv(me.id, &names_list, 640, 3, 0);
                printf("Available users\n");
                for(int i = 0; i < 10 && strcmp(names_list.tab[i], "") != 0; i++)
                {
                    printf("%s\n", names_list.tab[i]);
                }
                break;
            case 4:
                
                printf("Write name of chosen group: ");
                scanf("%s", group);
                send_two_line(pub_queue, choice, me.log, group);
                msgrcv(me.id, &names_list, 640, choice, 0);
                printf("Members of group %s :\n", group);
                for(int i = 0; i < 10 && strcmp(names_list.tab[i], "") != 0; i++)
                {
                    printf("%s\n", names_list.tab[i]);
                }
                break;
            case 5:
                printf("Write name of chosen group: ");
                scanf("%s", group);
                send_two_line(pub_queue, choice, me.log, group);
                msgrcv(me.id, &s, sizeof(int), choice, 0);
                if(s.com == 1)
                {
                    printf("Successfuly joined group %s\n\n", group);
                }
                else
                {
                    printf("Failed to join the group %s\n\n", group);
                }
                break;
            case 6:
                printf("Write name of chosen group: ");
                scanf("%s", group);
                send_two_line(pub_queue, choice, me.log, group);
                msgrcv(me.id, &s, sizeof(int), choice, 0);
                if(s.com == 1)
                {
                    printf("Successfuly left group %s\n\n", group);
                }
                else
                {
                    printf("Failed to leave group %s\n\n", group);
                }
                break;
            case 7:
                send_one_line(pub_queue, choice, me.log);
                msgrcv(me.id, &names_list, 640, choice, 0);
                printf("Available groups :\n");
                for(int i = 0; i < 10 && strcmp(names_list.tab[i], "") != 0; i++)
                {
                    printf("%s\n", names_list.tab[i]);
                }
                break;
            case 8:
            case 9:
                send_message(pub_queue, choice, me.log, me.id);
                break;
            case 10:
                printf("What user do you want to block? ");
                scanf("%s", tmp);
                send_two_line(pub_queue, choice, me.log, tmp);
                break;
            case 11:
                printf("What group do you want to block? ");
                scanf("%s", tmp);
                send_two_line(pub_queue, choice, me.log, tmp);
                break;
            case 12:
                send_one_line(pub_queue, 2, me.log);
                break;
            default:
                break;
            }
        }
        

        kill(pid, SIGKILL);
        msgctl(me.id, IPC_RMID, NULL);
    }
    return 0;
}

