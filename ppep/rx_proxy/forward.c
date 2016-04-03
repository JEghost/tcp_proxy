#include "forward.h"

static int fill_queue(pqueue_t *pq, fqueue_t *fq);

static void forward_data(fqueue_t* fq, int pack_cnt);

static void forward_loop(pool_t *pl, fqueue_t *fq);

/**
 * @brief gets and  initializes pool_t and fqueue_t 
 * structs, then starts forward loop
 *
 * @param args
 *
 * @return 
 */
void *wait2forward(void *args) {
    int sockfd;
    char *dest_ip = NULL, *dest_port = NULL;
    struct sockaddr_in server;

    fqueue_t *fq;
    pool_t *pl = NULL;
    queue_args_t* queue_args = NULL;

    queue_args = (queue_args_t*) args;
    dest_ip = queue_args->dest_ip;
    dest_port = queue_args->dest_port;
    printf("dest %s:%s\n", dest_ip, dest_port);
    
    // init receive socket
    server.sin_addr.s_addr = inet_addr(dest_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(dest_port));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr*)&server,
                sizeof(server)) < 0) {
        perror("forward connection failed");
    }

    // init forward queue
    fq = (fqueue_t*) malloc(sizeof(fqueue_t));
    fq->byte_count = 0;
    fq->byte_capacity = INIT_QUEUE_SIZE;
    fq->sockfd = sockfd;
    fq->state = SLEEP;
    fq->sent = -1;

    pl = queue_args->pl;
    forward_loop(pl, fq);
    return NULL;
}

/**
 * @brief running queue thread, sleeps to get
 * signal, then gets packets from priority queue
 * and forwards through end-destination
 *
 * @param pl
 * @param fq
 */
static void forward_loop(pool_t *pl, fqueue_t *fq) {

    struct timespec to;
    int pack_cnt;

    pthread_mutex_lock(&pl->lock);
    while (true) {
        to.tv_sec = WAIT_TIME;
        clock_gettime(CLOCK_MONOTONIC, &to);
        sleep(WAIT_TIME);
        pthread_cond_timedwait(&pl->cond, &pl->lock, &to);
        pack_cnt = fill_queue(pl->pq, fq);
        if (pack_cnt != -1) {
            pthread_mutex_unlock(&pl->lock);
            forward_data(fq, pack_cnt);
            pl->sent_min_seq += pack_cnt;
        } else {
            pthread_mutex_unlock(&pl->lock);
        }

    }
}

/**
 * @brief forward data towards agnostic
 * end destination, and update forward queue.
 *
 * @param fq
 * @param[in] pack_cnt
 */
static void forward_data(fqueue_t* fq, int pack_cnt)
{
    int byte = 0, count = 0, numbytes = 0;

    for (count = 0; count < pack_cnt; count++) {
        while (byte < BLOCKSIZE) {
            numbytes = send(fq->sockfd, fq->buffer[count],
                    BLOCKSIZE - byte, 0);
            if (numbytes < 0) {
                perror("Error in forward_data");
            } else if (numbytes > 0) {
                byte += numbytes;
            }
        }
        byte = 0;
        fq->sent += 1;
    } 
    free(fq->buffer);
}

/**
 * @brief 
 *
 * @param pq
 * @param fq
 *
 * @return number of ready consecutive 
 * packets to forward, or -1
 */
static int fill_queue(pqueue_t *pq, fqueue_t *fq)
{
    int pack_cnt = 0;
    node_t *ns;
    
    // init buffer of queue
    fq->buffer = (char **) malloc(10 * BLOCKSIZE);
    ns = (node_t *) malloc(sizeof(node_t));
    ns = (node_t *) pqueue_pop(pq);
    if (ns == NULL) {
        return -1;
    }
    
    // check whether the packet is ordered
    if ((int) ns->pri != fq->sent + 1) {
        pqueue_insert(pq, ns);
        return -1;
    } else {
        fq->buffer[pack_cnt] = (char*) ns->raw_packet;
        pack_cnt += 1;
        while (true) {
            ns = (node_t *) malloc(sizeof(node_t));
            ns = (node_t *) pqueue_pop(pq);

            // extend buffer, if full
            if (fq->byte_capacity - fq->byte_count < 10 * BLOCKSIZE) {
                fq->byte_capacity = fq->byte_capacity * 2;
                fq->buffer = (char **) realloc(fq->buffer,
                        fq->byte_capacity);
            }

            if (ns != NULL) {
                if ( (int) ns->pri == fq->sent + pack_cnt + 1) {
                    fq->buffer[pack_cnt] = (char*) ns->raw_packet;
                    pack_cnt += 1;
                    fq->byte_count += (int) sizeof(ns->raw_packet);
                } else {
                    pqueue_insert(pq, ns);
                    goto CNT_REPORT;
                }
            } else {
                goto CNT_REPORT;
            }

        }
CNT_REPORT:
        return pack_cnt;
    } 
}
