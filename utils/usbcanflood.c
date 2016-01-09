#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include "getopt.h"
#include "usbcan.h"

int32_t s_count = 0, r_count = 0, errors = 0, batches = 0, r_batches = 0;

void usbcandump_exit_handler(int signal) {
#pragma unused(signal)

    usbcan_library_close();

    exit(0);
}

void usbcandump_callback(uint32_t dev, uint32_t bus, struct usbcan_msg *msgs,
                         uint32_t n, void *arg) {
#pragma unused(dev)
#pragma unused(bus)
#pragma unused(msgs)
#pragma unused(arg)

    r_batches++;
    r_count += n;
}

void usbcandump_null_callback(uint32_t dev, uint32_t bus,
                              struct usbcan_msg *msgs, uint32_t n, void *arg) {
#pragma unused(msgs)
#pragma unused(arg)

    printf("Received unexpected callback with %u messages %u/%u\n", n, dev,
           bus);
}

void usage() {
    printf("nope!\n");
    exit(-1);
}

int main(int argc, char **argv) {
    uint32_t dev_src = 0, bus_src = 0;
    uint32_t dev_dst = 0, bus_dst = 1;
    uint32_t delay = 1, batch_size = 200;

    setbuf(stdout, NULL);

    struct sigaction int_act;
    int_act.sa_handler = usbcandump_exit_handler;
    sigaction(SIGINT, &int_act, NULL);

    const char *ch;
    while ((ch = GETOPT(argc, argv)) != NULL) {
        GETOPT_SWITCH(ch) {
          GETOPT_OPTARG("--dev-src") : dev_src = atoi(optarg);
            break;
          GETOPT_OPTARG("--bus-src") : bus_src = atoi(optarg);
            break;
          GETOPT_OPTARG("--dev-dst") : dev_dst = atoi(optarg);
            break;
          GETOPT_OPTARG("--bus-dst") : bus_dst = atoi(optarg);
            break;
          GETOPT_OPTARG("--delay_ms") : delay = atoi(optarg);
            break;
          GETOPT_OPTARG("--batch_size") : batch_size = atoi(optarg);
            break;
          GETOPT_DEFAULT:
            usage();
        }
    }

    if (!usbcan_library_init()) {
        exit(-1);
    }

    struct usbcan_bus_config config;
    config.speed = GINKGO_CAN_SPEED_500KBPS; // CAN_SPEED_500KBPS;
    config.filters = NULL;
    config.num_filters = 0;
    config.cb = usbcandump_null_callback;
    config.arg = NULL;

    if (!usbcan_init(dev_src, bus_src, &config)) {
        exit(-1);
    }

    if (!usbcan_start(dev_src, bus_src)) {
        exit(-1);
    }

    sleep(1);

    config.cb = usbcandump_callback;
    if (!usbcan_init(dev_dst, bus_dst, &config)) {
        exit(-1);
    }

    if (!usbcan_start(dev_dst, bus_dst)) {
        exit(-1);
    }

    printf("Initialization complete: %u/%u -> %u/%u\n", dev_src, bus_src,
           dev_dst, bus_dst);

    sleep(1);

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = delay + 1000000;

    struct timeval tv;
    time_t last = 0;

    if (gettimeofday(&tv, NULL) == 0) {
        last = tv.tv_sec;
    }

    struct can_frame *frames =
        (struct can_frame *)calloc(batch_size, sizeof(struct can_frame));
    for (uint32_t f = 0; f < batch_size; f++) {
        frames[f].can_id = 0x7FD;
        frames[f].data[0] = 0x01;
        frames[f].data[1] = 0x02;
        frames[f].data[2] = 0x03;
        frames[f].data[3] = 0x04;
        frames[f].data[4] = 0x05;
        frames[f].data[5] = 0x06;
        frames[f].data[6] = 0x07;
        frames[f].data[7] = 0x08;
        frames[f].can_dlc = 8;
    }

    while (1) {
        int sent = usbcan_send_n(dev_src, bus_src, frames, batch_size);
        errors += batch_size - sent;
        s_count += sent;
        batches++;

        gettimeofday(&tv, NULL);
        if (tv.tv_sec > last) {
            last = tv.tv_sec;
            printf("Sent %u/%u (%u), received %u (%u)\n", s_count,
                   s_count - errors, batches, r_count, r_batches);
            s_count = 0;
            batches = 0;
            r_count = 0;
            r_batches = 0;
            errors = 0;
        }

        nanosleep(&ts, NULL);
    }

    free(frames);

    return 0;
}
