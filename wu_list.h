struct watchuser_list{
    char *node;
    struct watchuser_list *next;
    struct watchuser_list *prev;
    char *user; // The user[s] being watched
    int watch;
};
void add(char *command);
void del(struct watchuser_list *l, char *un);