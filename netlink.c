#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/gfp.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/time.h>
#include <net/sock.h>

#define MYMGRP 22

struct sock *nl_sk = NULL;
static struct timer_list timer;
struct kprobe *kp;

/* Receive echo messages from userspace 
 * compute and log message RTT
 */
void nl_recv_msg(struct sk_buff *skb) {
    ktime_t now, then;
    struct nlmsghdr *nlh = nlmsg_hdr(skb);
    now = ktime_get();
    then = *((ktime_t*) nlmsg_data(nlh));
    printk(KERN_INFO "netlink_rtt: %lld\n", (s64)(now) - (s64)(then));
}

/* Send message to userspace
 * payload: time of message send
 */
void nl_send_msg(unsigned long data) {
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int res;
    int msg_size;

    ktime_t now = ktime_get();
    msg_size = sizeof(ktime_t);

    skb_out = nlmsg_new(
        NLMSG_ALIGN(msg_size), // @payload: size of the message payload
        GFP_NOWAIT             // @flags: the type of memory to allocate.
    );
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(
        skb_out,    // @skb: socket buffer to store message in
        0,          // @portid: netlink PORTID of requesting application
        0,          // @seq: sequence number of message
        NLMSG_DONE, // @type: message type
        msg_size,   // @payload: length of message payload
        0           // @flags: message flags
    );

    memcpy(nlmsg_data(nlh), &now, msg_size);
    res = nlmsg_multicast(
            nl_sk,     // @sk: netlink socket to spread messages to
            skb_out,   // @skb: netlink message as socket buffer
            0,         // @portid: own netlink portid to avoid sending to yourself
            MYMGRP,    // @group: multicast group id
            GFP_NOWAIT // @flags: allocation flags
    );
    if (res < 0) {
        printk(KERN_INFO "Error while sending to user: %d\n", res);
    } else {
        mod_timer(&timer, jiffies + msecs_to_jiffies(10));
    }
}

static int __init nl_init(void) {
    struct netlink_kernel_cfg cfg = {
           .input = nl_recv_msg,
    };
    
    printk(KERN_INFO "init NL\n");
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USERSOCK, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    init_timer(&timer);
    timer.function = nl_send_msg;
    timer.expires = jiffies + 1000;
    timer.data = 0;
    add_timer(&timer);
    nl_send_msg(0);

    return 0;
}

static void __exit nl_exit(void) {
    printk(KERN_INFO "exit NL\n");
    del_timer_sync(&timer);
    netlink_kernel_release(nl_sk);
}

module_init(nl_init);
module_exit(nl_exit);

MODULE_LICENSE("GPL");
