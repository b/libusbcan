/*
   Copyright 2015 Benjamin Black

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef USBCAN_H
#define USBCAN_H

#include <stdint.h>

#ifdef __linux__
#include <linux/can.h>
#else
#include "can_compat.h"
#endif

#define CAN1  0
#define CAN2  1

const uint32_t CAN_SPEEDS[33][4] = {
    // CAN_BRP, CAN_BS1, CAN_BS2, CAN_SJW

    // The first 9 achieve a duration of ~16 bit times at each speed
    {240, 12, 2, 1},
    {120, 12, 2, 1},
    {45, 13, 2, 1},
    {27, 13, 2, 1},
    {15, 20, 3, 1},
    {18, 13, 2, 1},
    {9, 13, 2, 1},
    {4, 15, 2, 1},
    {2, 15, 2, 1},

    // The remainder are the values given in the Ginkgo API manual
    {1000, 10, 6, 2},
    {1000, 6, 4, 2},
    {600, 6, 4, 2},
    {600, 3, 2, 1},
    {300, 3, 2, 1},
    {120, 6, 3, 1},
    {150, 3, 2, 1},
    {120, 3, 2, 1},
    {60, 6, 3, 1},
    {75, 3, 2, 1},
    {50, 4, 3, 1},
    {60, 3, 2, 1},
    {48, 3, 2, 1},
    {40, 3, 2, 1},
    {30, 3, 2, 1},
    {24, 3, 2, 1},
    {20, 3, 2, 1},
    {10, 5, 3, 1},
    {12, 3, 2, 1},
    {9, 5, 3, 1},
    {6, 6, 3, 1},
    {5, 5, 3, 1},
    {5, 4, 3, 1},
    {6, 3, 2, 1}
};

#define CAN_SPEED_10KBPS    0
#define CAN_SPEED_20KBPS    1
#define CAN_SPEED_50KBPS    2
#define CAN_SPEED_83KBPS    3
#define CAN_SPEED_100KBPS   4
#define CAN_SPEED_125KBPS   5
#define CAN_SPEED_250KBPS   6
#define CAN_SPEED_500KBPS   7
#define CAN_SPEED_1000KBPS  8

#define GINKGO_CAN_SPEED_2KBPS      9
#define GINKGO_CAN_SPEED_3KBPS     10
#define GINKGO_CAN_SPEED_5KBPS     11
#define GINKGO_CAN_SPEED_10KBPS    12
#define GINKGO_CAN_SPEED_20KBPS    13
#define GINKGO_CAN_SPEED_30KBPS    14
#define GINKGO_CAN_SPEED_40KBPS    15
#define GINKGO_CAN_SPEED_50KBPS    16
#define GINKGO_CAN_SPEED_60KBPS    17
#define GINKGO_CAN_SPEED_80KBPS    18
#define GINKGO_CAN_SPEED_90KBPS    19
#define GINKGO_CAN_SPEED_100KBPS   20
#define GINKGO_CAN_SPEED_125KBPS   21
#define GINKGO_CAN_SPEED_150KBPS   22
#define GINKGO_CAN_SPEED_200KBPS   23
#define GINKGO_CAN_SPEED_250KBPS   24
#define GINKGO_CAN_SPEED_300KBPS   25
#define GINKGO_CAN_SPEED_400KBPS   26
#define GINKGO_CAN_SPEED_500KBPS   27
#define GINKGO_CAN_SPEED_600KBPS   28
#define GINKGO_CAN_SPEED_666KBPS   29
#define GINKGO_CAN_SPEED_800KBPS   30
#define GINKGO_CAN_SPEED_900KBPS   31
#define GINKGO_CAN_SPEED_1000KBPS  32

#define USBCAN_OK    0
#define USBCAN_ERROR 1

#define MAX_FILTERS 14

struct usbcan_msg
{
    uint32_t timestamp;
    struct can_frame frame;
};

typedef void(*usbcan_cb)(uint32_t dev, uint32_t bus, struct usbcan_msg *msgs, uint32_t n, void *arg);

struct usbcan_bus_config
{
    uint32_t           speed;
    struct can_filter *filters;
    uint8_t            num_filters;
    usbcan_cb          cb;
    void              *arg;
};

#ifdef __cplusplus
extern "C" {
#endif
uint32_t usbcan_library_init();
uint32_t usbcan_library_close();

uint32_t usbcan_init(uint32_t dev, uint32_t bus, struct usbcan_bus_config *config);
uint32_t usbcan_start(uint32_t dev, uint32_t bus);
uint32_t usbcan_reset(uint32_t dev, uint32_t bus);
uint32_t usbcan_stop(uint32_t dev, uint32_t bus);

uint32_t usbcan_send(uint32_t dev, uint32_t bus, struct can_frame *frame);
uint32_t usbcan_send_n(uint32_t dev, uint32_t bus, struct can_frame *frames, uint32_t len);

uint32_t usbcan_set_filters(uint32_t dev, uint32_t bus, struct can_filter *filters, uint8_t num_filters);
uint32_t usbcan_clear_filters(uint32_t dev, uint32_t bus);
uint32_t usbcan_register_callback(uint32_t dev, uint32_t bus, usbcan_cb callback, void *arg);
uint32_t usbcan_deregister_callback(uint32_t dev, uint32_t bus);
#ifdef __cplusplus
}
#endif

#endif /* USBCAN_H */
