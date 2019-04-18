struct watchmail_list{
    char *node;
    struct watchmail_list *next;
    struct watchmail_list *prev;
}
void add_mail(char *fn);
void delm(struct watchmail_list *wml, char *name);