#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

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

int main(void) {
    setbuf(stdout, NULL);

    int status = usbcan_library_init();
    if (status == USBCAN_ERROR) {
        exit(-1);
    }

    struct usbcan_bus_config config;
    config.speed = CAN_SPEED_500KBPS;
    config.filters = NULL;
    config.num_filters = 0;
    config.cb = usbcandump_callback;
    config.arg = NULL;

    status = usbcan_init(0, CAN1, &config);
    if (status == USBCAN_ERROR) {
        exit(-1);
    }

    status = usbcan_start(0, CAN1);
    if (status == USBCAN_ERROR) {
        exit(-1);
    }

    struct sigaction int_act;
    int_act.sa_handler = usbcandump_exit_handler;
    sigaction(SIGINT, &int_act, NULL);

    pause();

    return 0;
}
