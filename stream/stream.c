#include "stream.h"

static void stream(char *ip_addr, char *port, FILE *fp);

static void eval_config_item(char const *token,
        char const *value, struct arg_configer *arg_conf); 

static FILE *log_fp;

#ifdef STREAM
int main(int argc, char *argv[]) 
{
    const char *ip_addr, *port, *fname;
    FILE *fp;
    int i = 0, ret;

    log_fp = fopen(STREAM_LOG, "w");
    if (log_fp == NULL) {
        perror("fopen: ");
        exit(1);
    }
    
    #ifdef ARGV_ENABLE
    if (argc == 4) {
        struct option long_options[] = {
            {"ip_addr", required_argument, NULL, 'A'},
            {"port", required_argument, NULL, 'B'},
            {"fname", required_argument, NULL, 'C'}
        };

        arg_val_t **argv_vals = \
            (arg_val_t **) malloc(sizeof(arg_val_t*) * (argc - 1));

        for (i = 0; i < argc - 1; i++)
            argv_vals[i] = (arg_val_t *) malloc(sizeof(arg_val_t));

        ret = argv_reader(argv_vals,
                long_options, argv, argc);

        ip_addr = get_argv((char *) "ip_addr", argv_vals, argc-1);
        LOG_ASSERT(log_fp, LL_ERROR, ip_addr!=NULL);

        port = get_argv((char *) "port", argv_vals, argc-1);
        LOG_ASSERT(log_fp, LL_ERROR, port!=NULL);

        fname = get_argv((char *) "fname", argv_vals, argc-1);
        LOG_ASSERT(log_fp, LL_ERROR, fname!=NULL);
    }
    #endif
    
    #ifdef CONF_ENABLE
    if (argc == 1) {
        config_t cfg;
        config_setting_t *setting;
        char *config_file = "../network.conf";
        config_init(&cfg);
        if (!config_read_file(&cfg, config_file)) {
            printf("\n%s:%d - %s", config_error_file(&cfg), 
                    config_error_line(&cfg),
                    config_error_text(&cfg));
            config_destroy(&cfg);
            return -1;
        }
        setting = config_lookup(&cfg, "stream");
        if (setting != NULL) {
            if (config_setting_lookup_string(setting, "dest_ip", &ip_addr)) {
                printf("\n dest_ip: %s\n", ip_addr);
            } else {
                printf("destination ip is not configured\n");
                return -1;
            }
            if (config_setting_lookup_string(setting, "dest_port", &port)) {
                printf("\n dest_port: %s\n", port);
            } else {
                printf("destination port is not configured\n");
                return -1;
            }
            if (config_setting_lookup_string(setting, "file_name", &fname)) {
                printf("\n file_name: %s\n", fname);
            } else {
                printf("local file is not configured\n");
                return -1;
            }
        } else {
            printf("no configuration for streamer\n");
            return -1;
        }
    }
    #endif

    fp = fopen(fname, "r");
    LOG_ASSERT(log_fp, LL_ERROR, fp!=NULL);

    stream((char*) ip_addr, (char*) port, fp);
}
#endif

/**
 * @brief read file and stream 
 * to ip:port node
 *
 * @param[in] ip_addr
 * @param[in] port
 * @param[in] fp
 */
static void stream(char *ip_addr, char *port, FILE *fp)
{ 
    int ret, sockfd;
    int byte_count = 0, temp_count = 0, capacity = 0;
    int file_size = 0, amount = 0, residue = 0;
    char *thr_buff;

    //Initialize sockaddr_in struct to connect to dest
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ip_addr);
    server.sin_family = AF_INET;
    server.sin_port = htons( atoi(port) );

    printf("socket initializing ...\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    LOG_ASSERT(log_fp, LL_ERROR, sockfd!=-1);

    printf("connecting ...\n");
    ret = connect(sockfd, (struct sockaddr*)&server,
            sizeof(server));
    LOG_ASSERT(log_fp, LL_ERROR, ret!=-1);

    thr_buff = (char *) malloc(BLOCKSIZE);
    file_size = get_file_size(fp);

    while (byte_count < file_size) {
        if (byte_count + BLOCKSIZE < file_size) {
            read_file(fp, byte_count, byte_count + BLOCKSIZE, thr_buff);
            capacity = BLOCKSIZE;
        } else {
            residue = file_size - byte_count;
            read_file(fp, byte_count, byte_count + residue, thr_buff);
            capacity = residue;
        }
        while (temp_count < capacity) {
            amount = write(sockfd, thr_buff+temp_count, 
                    capacity-temp_count); 
            if (amount > 0) {
                temp_count += amount;
            }
        }
        byte_count += temp_count;
        temp_count = 0;
    }
} 

static void eval_config_item(char const *token,
        char const *value, struct arg_configer *arg_conf) {
    if (!strcmp(token, "ip_addr")) {
        strcpy(arg_conf->ip_addr, value);
        printf("arg_conf->ip_addr: %s\n", arg_conf->ip_addr);
        return;
    }

    if (!strcmp(token, "port")) {
        strcpy(arg_conf->port, value);
        printf("arg_conf->port: %s\n", arg_conf->port);
        return;
    }

    if (!strcmp(token, "fname")) {
        strcpy(arg_conf->fname, value);
        printf("arg_conf->log_file: %s\n", arg_conf->fname);
        return;
    }
}
