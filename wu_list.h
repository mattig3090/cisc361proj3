struct watchuser_list{
    char *node;
    struct watchuser_list *next;
    struct watchuser_list *prev;
    char *user; // The user[s] being watched
    int watch;
};
