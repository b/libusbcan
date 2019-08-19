/*

  usbcan.c -- ViewTool Ginkgo USB-CAN C API

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

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "usbcan.h"
#include "ginkgo.h"

struct usbcan_cb_entry {
    uint32_t dev;
    uint32_t bus;
    usbcan_cb cb;
    void *arg;
    struct usbcan_cb_entry *next;
};

struct usbcan_state {
    uint32_t type;
    uint32_t num_devs;
    uint32_t *devs;
    struct usbcan_cb_entry *callbacks;
    struct usbcan_cb_entry *tail;
};

struct usbcan_state state;

void usbcan_callback_dispatcher(uint32_t dev, uint32_t bus, uint32_t n);

bool usbcan_library_init() {
    state.type = VCI_USBCAN2;

    state.callbacks = state.tail = NULL;

    state.num_devs = VCI_ScanDevice(1);
    if (state.num_devs == 0) {
        return false;
    }

    state.devs = (uint32_t *)calloc(state.num_devs, sizeof(uint32_t));

    return true;
}

bool usbcan_library_close() {
    struct usbcan_cb_entry *cbe = state.callbacks;

    while (cbe != NULL) {
        usbcan_deregister_callback(cbe->dev, cbe->bus);
        cbe = cbe->next;
    }

    for (uint32_t dev = 0; dev < state.num_devs; dev++) {
        if (state.devs[dev] > 0) {
            int ginkgo_status = VCI_CloseDevice(state.type, dev);
            if (ginkgo_status == STATUS_ERR) {
                return false;
            }

            ginkgo_status = VCI_LogoutReceiveCallback(dev);
            if (ginkgo_status == STATUS_ERR) {
                return false;
            }
        }
    }

    return true;
}

bool usbcan_dev_init(uint32_t dev) {
    if (state.num_devs <= dev) {
        return false;
    }

    if (state.devs[dev] > 0) {
        return true;
    }

    int ginkgo_status = VCI_OpenDevice(state.type, dev, 0);
    if (ginkgo_status == STATUS_ERR) {
        return false;
    }

    ginkgo_status =
        VCI_RegisterReceiveCallback(dev, usbcan_callback_dispatcher);
    if (ginkgo_status == STATUS_ERR) {
        return false;
    }

    state.devs[dev] = 1;

    return true;
}

bool usbcan_init(uint32_t dev, uint32_t bus, struct usbcan_bus_config *config) {
    bool status = usbcan_dev_init(dev);
    if (!status) {
        return false;
    }

    VCI_INIT_CONFIG_EX init_config;
    init_config.CAN_ABOM = 0;
    init_config.CAN_Mode = 0;
    init_config.CAN_BRP = CAN_SPEEDS[config->speed][0];
    init_config.CAN_BS1 = CAN_SPEEDS[config->speed][1];
    init_config.CAN_BS2 = CAN_SPEEDS[config->speed][2];
    init_config.CAN_SJW = CAN_SPEEDS[config->speed][3];
    init_config.CAN_NART = 1;
    init_config.CAN_RFLM = 0;
    init_config.CAN_TXFP = 1;
    init_config.CAN_RELAY = 0;

    uint32_t ginkgo_status = VCI_InitCANEx(state.type, dev, bus, &init_config);
    if (ginkgo_status == STATUS_ERR) {
        return false;
    }

    if (config->num_filters == 0) {
        usbcan_clear_filters(dev, bus);
    } else {
        usbcan_set_filters(dev, bus, config->filters, config->num_filters);
    }

    if (config->cb != NULL) {
        status = usbcan_register_callback(dev, bus, config->cb, config->arg);
        if (!status) {
            return false;
        }
    }

    return true;
}

bool usbcan_start(uint32_t dev, uint32_t bus) {
    int ginkgo_status = VCI_StartCAN(state.type, dev, bus);
    if (ginkgo_status == STATUS_ERR) {
        return false;
    }

    return true;
}

bool usbcan_reset(uint32_t dev, uint32_t bus) {
    VCI_ResetCAN(state.type, dev, bus);

    return true;
}

bool usbcan_stop(uint32_t dev, uint32_t bus) {
    usbcan_deregister_callback(dev, bus);
    usbcan_reset(dev, bus);

    return true;
}

uint32_t usbcan_send(uint32_t dev, uint32_t bus, struct can_frame *frame) {
    return usbcan_send_n(dev, bus, frame, 1);
}

uint32_t usbcan_send_n(uint32_t dev, uint32_t bus, struct can_frame *frames,
                       uint32_t n) {
    PVCI_CAN_OBJ msgs = (PVCI_CAN_OBJ)calloc(n, sizeof(VCI_CAN_OBJ));

    for (uint32_t i = 0; i < n; i++) {
        msgs[i].ID = (frames[i].can_id & CAN_EFF_FLAG) > 0
            ? frames[i].can_id & CAN_EFF_MASK
            : frames[i].can_id & CAN_SFF_MASK;
        msgs[i].SendType = 0;
        msgs[i].RemoteFlag = (frames[i].can_id & CAN_RTR_FLAG) > 0 ? 1 : 0;
        msgs[i].ExternFlag = (frames[i].can_id & CAN_EFF_FLAG) > 0 ? 1 : 0;

        for (int j = 0; j < frames[i].can_dlc; j++) {
            msgs[i].Data[j] = frames[i].data[j];
        }
        msgs[i].DataLen = frames[i].can_dlc;
    }

    int sent = VCI_Transmit(state.type, dev, bus, msgs, n);

    free(msgs);

    return sent;
}

bool usbcan_set_filters(uint32_t dev, uint32_t bus, struct can_filter *filters,
                        uint8_t num_filters) {
    usbcan_clear_filters(dev, bus);

    if (num_filters >= MAX_FILTERS) {
        return false;
    }

    for (int i = 0; i < num_filters; i++) {
        int mode = 1;

        if (filters[i].can_mask == 0 && filters[i].can_id > 0) {
            mode = 0;
        }

        VCI_FILTER_CONFIG filter_config;
        filter_config.FilterIndex = i;
        filter_config.Enable = 1;
        filter_config.ExtFrame = 0;
        filter_config.FilterMode = mode;
        filter_config.ID_IDE = (filters[i].can_id & CAN_EFF_FLAG) > 0 ? 1 : 0;
        filter_config.ID_RTR = (filters[i].can_id & CAN_RTR_FLAG) > 0 ? 1 : 0;
        filter_config.ID_Std_Ext = (filters[i].can_id & CAN_EFF_FLAG) > 0
            ? filters[i].can_id & CAN_EFF_MASK
            : filters[i].can_id & CAN_SFF_MASK;
        filter_config.MASK_IDE =
            (filters[i].can_mask & CAN_EFF_FLAG) > 0 ? 1 : 0;
        filter_config.MASK_RTR =
            (filters[i].can_mask & CAN_RTR_FLAG) > 0 ? 1 : 0;
        filter_config.MASK_Std_Ext = (filters[i].can_mask & CAN_EFF_FLAG) > 0
            ? filters[i].can_mask & CAN_EFF_MASK
            : filters[i].can_mask & CAN_SFF_MASK;

        int ginkgo_status = VCI_SetFilter(state.type, dev, bus, &filter_config);
        if (ginkgo_status == STATUS_ERR) {
            return false;
        }
    }

    return true;
}

bool usbcan_clear_filters(uint32_t dev, uint32_t bus) {
    for (int i = 0; i < MAX_FILTERS; i++) {
        VCI_FILTER_CONFIG filter_config;
        filter_config.FilterIndex = i;
        filter_config.Enable = 1;
        filter_config.ExtFrame = 0;
        filter_config.FilterMode = 0;
        filter_config.ID_IDE = 0;
        filter_config.ID_RTR = 0;
        filter_config.ID_Std_Ext = 0;
        filter_config.MASK_IDE = 0;
        filter_config.MASK_RTR = 0;
        filter_config.MASK_Std_Ext = 0;

        int ginkgo_status = VCI_SetFilter(state.type, dev, bus, &filter_config);
        if (ginkgo_status == STATUS_ERR) {
            return false;
        }
    }

    return true;
}

struct usbcan_cb_entry *usbcan_get_callback(uint32_t dev, uint32_t bus) {
    struct usbcan_cb_entry *cbe = state.callbacks;

    while (cbe != NULL) {
        if (cbe->dev == dev && cbe->bus == bus) {
            return cbe;
        }
        cbe = cbe->next;
    }

    return NULL;
}

void usbcan_callback_dispatcher(uint32_t dev, uint32_t bus, uint32_t n) {
#pragma unused(n)
    struct usbcan_cb_entry *cbe = usbcan_get_callback(dev, bus);

    if (cbe != NULL) {
        int msgs_avail = VCI_GetReceiveNum(state.type, dev, bus);

        PVCI_CAN_OBJ vci_msgs =
            (PVCI_CAN_OBJ)calloc(msgs_avail, sizeof(VCI_CAN_OBJ));
        struct usbcan_msg *msgs =
            (struct usbcan_msg *)calloc(msgs_avail, sizeof(struct usbcan_msg));

        uint32_t msgs_read;
        while (msgs_avail > 0) {
            msgs_read = 0;
            msgs_read =
                VCI_Receive(state.type, dev, bus, vci_msgs, msgs_avail, -1);
            if (msgs_read == 0 || msgs_read >= 0xFFFFFFFF) {
                goto dispatcher_cleanup;
            }

            for (uint32_t i = 0; i < msgs_read; i++) {
                msgs[i].frame.can_id = vci_msgs[i].ID;

                if (vci_msgs[i].TimeFlag > 0) {
                    msgs[i].timestamp = vci_msgs[i].TimeStamp;
                }

                if (vci_msgs[i].RemoteFlag > 0) {
                    msgs[i].frame.can_id |= CAN_RTR_FLAG;
                }

                if (vci_msgs[i].ExternFlag > 0) {
                    msgs[i].frame.can_id |= CAN_EFF_FLAG;
                }

                msgs[i].frame.can_dlc = vci_msgs[i].DataLen;
                for (int j = 0; j < vci_msgs[i].DataLen; j++) {
                    msgs[i].frame.data[j] = vci_msgs[i].Data[j];
                }
            }
            cbe->cb(dev, bus, msgs, msgs_read, cbe->arg);

            msgs_avail -= msgs_read;
        }
      dispatcher_cleanup:
        free(msgs);
        free(vci_msgs);
    }
}

bool usbcan_register_callback(uint32_t dev, uint32_t bus, usbcan_cb cb,
                              void *arg) {
    struct usbcan_cb_entry *cbe = usbcan_get_callback(dev, bus);

    if (cbe != NULL) {
        return false;
    }

    cbe = (struct usbcan_cb_entry *)calloc(1, sizeof(struct usbcan_cb_entry));
    cbe->dev = dev;
    cbe->bus = bus;
    cbe->cb = cb;
    cbe->arg = arg;
    cbe->next = NULL;

    if (state.tail != NULL) {
        state.tail->next = cbe;
    } else {
        state.callbacks = cbe;
    }
    state.tail = cbe;

    return true;
}

bool usbcan_deregister_callback(uint32_t dev, uint32_t bus) {
    struct usbcan_cb_entry *cbe = usbcan_get_callback(dev, bus);
    struct usbcan_cb_entry *pred_cbe = state.callbacks;

    if (cbe != NULL) {
        if (pred_cbe != cbe) {
            while (pred_cbe->next != cbe) {
                pred_cbe = pred_cbe->next;
            }
        }

        pred_cbe->next = cbe->next;

        if (state.tail == cbe) {
            state.tail = pred_cbe;
        }

        free(cbe);
    }

    return true;
}
