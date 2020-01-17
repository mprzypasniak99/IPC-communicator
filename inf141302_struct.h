#ifndef STRUCT_H_INCLUDED
#define STRUCT_H_INCLUDED

#define LOGIN_ATTEMPTS_ALLOWED  5

struct login
{
    long mtype;
    char log[64];
    char pwd[64];
    int id;
};
struct success
{
    long mtype;
    int com;
};
struct one_line
{
    long mtype;
    char txt[256];
};

struct group_tab
{
    long mtype;
    char tab[10][64];
};

struct two_line
{
    long mtype;
    char msg[64];
    char usr_name[64];
};

struct message
{
    long mtype;
    char sender[64];
    char receiver[64];
    char msg[256];
};

#endif