#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

#include "getopt.h"
#include "usbcan.h"

uint32_t count = 0;

void usbcandump_exit_handler(int signal) {
#pragma unused(signal)

    usbcan_library_close();

    exit(0);
}

void usbcandump_callback(uint32_t dev, uint32_t bus, struct usbcan_msg *msgs,
                         uint32_t n, void *arg) {
#pragma unused(arg)
    for (uint32_t i = 0; i < n; i++) {
        count++;
        printf(" usbcan%i:%i  %3X   [%i] ", dev, bus, msgs[i].frame.can_id,
               msgs[i].frame.can_dlc);
        for (int j = 0; j < 8; j++) {
            printf(" %02X", msgs[i].frame.data[j]);
        }
        printf(" %i\n", count);
    }
}

void usage() {
    exit(-1);
}

int main(int argc, char **argv) {
    uint32_t dev = 0, bus = 0;

    struct sigaction int_act;
    int_act.sa_handler = usbcandump_exit_handler;
    sigemptyset (&int_act.sa_mask);
    int_act.sa_flags = 0;
    sigaction(SIGINT, &int_act, NULL);
    
    setbuf(stdout, NULL);

    const char *ch;
    while ((ch = GETOPT(argc, argv)) != NULL) {
        GETOPT_SWITCH(ch) {
          GETOPT_OPTARG("--dev") : dev = atoi(optarg);
            break;
          GETOPT_OPTARG("--bus") : bus = atoi(optarg);
            break;
          GETOPT_DEFAULT:
            usage();
        }
    }

    if (!usbcan_library_init()) {
        exit(-1);
    }

    struct usbcan_bus_config config;
    config.speed = CAN_SPEED_500KBPS;
    config.filters = NULL;
    config.num_filters = 0;
    config.cb = usbcandump_callback;
    config.arg = NULL;

    if (!usbcan_init(dev, bus, &config)) {
        exit(-1);
    }

    if (!usbcan_start(dev, bus)) {
        exit(-1);
    }

    pause();

    return 0;
}
