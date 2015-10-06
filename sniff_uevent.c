#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/netlink.h>

#define ADD_MAGIC "add@/bus/my_pseudo_bus"
#define DEL_MAGIC "remove@/my_pseudo_bus"

extern int thread_run;
extern int bypass_root;

void die(char *s)
{
    write(2,s,strlen(s));
    exit(1);
}

void process_uevent(char *uevent, int len)
{
    char *line = uevent;

    if (strcmp(line, ADD_MAGIC) == 0) {
        printf("Added\n");
        bypass_root = 1;
    }
    else if (strcmp(line , DEL_MAGIC) == 0) {
        printf("Removed\n");
        bypass_root = 0;
    }
}

void *do_udev_sniff(void *data)
{
    struct sockaddr_nl nls;
    struct pollfd pfd;
    char buf[512];


    memset(&nls,0,sizeof(struct sockaddr_nl));
    nls.nl_family = AF_NETLINK;
    nls.nl_pid = getpid();
    nls.nl_groups = -1;

    pfd.events = POLLIN;
    pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (pfd.fd==-1)
        die("Not root\n");


    if (bind(pfd.fd, (void *)&nls, sizeof(struct sockaddr_nl)))
        die("Bind failed\n");
    while (-1!=poll(&pfd, 1, -1) && thread_run) {
        int len = recv(pfd.fd, buf, sizeof(buf), MSG_DONTWAIT);
        if (len == -1) die("recv\n");
        process_uevent(buf, len);
    }
    die("poll\n");

    return 0;
}

