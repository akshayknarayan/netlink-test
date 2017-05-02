#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/gfp.h>
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <net/sock.h>

#define MYMGRP 22

struct sock *nl_sk = NULL;
static struct timer_list timer;
struct kprobe *kp;

void nl_send_msg(unsigned long data) {
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int res;
    char *msg = "hello from kernel!\n";
    int msg_size = strlen(msg);

    printk(KERN_INFO "sending... ");

    skb_out = nlmsg_new(
        NLMSG_ALIGN(msg_size), // @payload: size of the message payload
        GFP_KERNEL             // @flags: the type of memory to allocate.
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

    memcpy(nlmsg_data(nlh), msg, msg_size+1);
    res = nlmsg_multicast(
            nl_sk,     // @sk: netlink socket to spread messages to
            skb_out,   // @skb: netlink message as socket buffer
            0,         // @portid: own netlink portid to avoid sending to yourself
            MYMGRP,    // @group: multicast group id
            GFP_KERNEL // @flags: allocation flags
    );
    if (res < 0) {
        printk(KERN_INFO "Error while sending to user: %d\n", res);
    } else {
        mod_timer(&timer, jiffies + msecs_to_jiffies(1));
        printk(KERN_INFO "Send ok\n");
    }
}

//int kprobe_print(struct kprobe *kp, struct pt_regs *r) {
//    printk(KERN_INFO "hit netlink_broadcast\n");
//    return 0;
//}
//
static int __init nl_init(void) {
    //int ok;
    struct netlink_kernel_cfg cfg = {};
    //struct kprobe k = {};
    //k.symbol_name = "netlink_broadcast";
    //k.pre_handler = &kprobe_print;

    //kp = kmalloc(sizeof(struct kprobe), GFP_KERNEL);
    //memcpy(kp, &k, sizeof(struct kprobe));
    //ok = register_kprobe(kp);
    //if (ok < 0) {
    //    printk(KERN_ALERT "Error creating kprobe: %d.\n", ok);
    //    return ok;
    //}
    
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
    //unregister_kprobe(kp);
    del_timer_sync(&timer);
    netlink_kernel_release(nl_sk);
}

module_init(nl_init);
module_exit(nl_exit);

MODULE_LICENSE("GPL");
