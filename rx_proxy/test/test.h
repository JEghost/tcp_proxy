#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#include "../../receive/receive.h"
#include "../boss_server.h"
#include "../../network/network.h"


/**
 * @brief number of raw data
 * sent towards receiver side
 * of proxy
 */
#define MAX_PACKET 100

struct rx_params{
    FILE *fp;
    char *port;
};

struct pxy_params{
    char* server_port;
    pool_t* pl;
};

struct stream_params{
    char *rx_pxy_ip;
    char *rx_pxy_port;
};

struct queue_params{
    pool_t* pl;
    char *dest_ip;
    char *dest_port;
};

#endif