#ifndef NFQUEUE_H
#define NFQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/user.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <linux/netfilter.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <errno.h>

#include <net/if.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../network/network.h"

#include <libnetfilter_queue/libnetfilter_queue.h>

#define BUFF_SIZE 10
#define INITIAL_CAPACITY 1000

struct ipv4_packet{
    struct iphdr iph;
    struct tcphdr tcph;
};

/**
 * @brief call-back arguments
 * of TCP SYN hijaker function
 */
struct cb_args_syn{
    struct nfq_q_handle *qh;
    struct nfgenmsg *nfmsg;
    struct nfq_data *nfa;
    void *data;
};

/**
 * @brief Splitting function
 * would push data into pointer
 * array of this struct. tx_chain()  
 * would use this struct to get data
 * and update its index and tx_byte.
 */
typedef struct proxy_buff{

    /**
     * @brief total amount of 
     * proxied bytes
     */
    int rx_byte;

    /**
     * @brief total amount of
     * transmitted bytes
     */
    int tx_byte;

    int set_ind;
    int get_ind;

    /**
     * @brief size of **buffer
     */
    int capacity; 

    /**
     * @brief pointers of 
     * proxied raw packets
     * added in this array
     */
    char **buffer;

    pthread_mutex_t lock;
}proxy_buff;


/**
 * @brief call-back arguments
 * of split thread
 */
struct split_args{
    /**
     * @brief nfqueue redirects
     * packets towards this port
     */
    char *local_port;

    proxy_buff *buff;
};


#define QUEUER_BUF_SIZE 10000

#endif 
