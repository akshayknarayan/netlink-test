#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>

/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 22

int nl_open(void) {
    int sock;
    struct sockaddr_nl addr;
    int group = MYMGRP;

    sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
    if (sock < 0) {
        printf("sock < 0.\n");
        return sock;
    }

    memset((void *) &addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("bind < 0.\n");
        return -1;
    }

    if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        printf("setsockopt < 0\n");
        return -1;
    }

    return sock;
}

void nl_recv(int sock, int count) {
    struct sockaddr_nl nladdr;
    struct msghdr msg;
    struct iovec iov;
    char buffer[65536];
    int ret;

    iov.iov_base = (void *) buffer;
    iov.iov_len = sizeof(buffer);
    msg.msg_name = (void *) &(nladdr);
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    printf("[%d] waiting... ", count);
    ret = recvmsg(sock, &msg, 0);
    if (ret < 0)
        printf("ret < 0.\n");
    else
        printf("received message payload: %s\n", (char*) NLMSG_DATA((struct nlmsghdr *) &buffer));
}

int main(int argc, char *argv[]) {
    int nls;
    int count = 0;

    nls = nl_open();
    if (nls < 0)
        return nls;

    while (1) {
        nl_recv(nls, count);
        sleep(1);
        count++;
    }

    return 0;
}
