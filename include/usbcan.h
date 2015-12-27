#include <stdint.h>

#include "ginkgo.h"

#define USBCAN_OK    0
#define USBCAN_ERROR 1

#define MAX_FILTERS 14

#define ID_MASK  0x1FFFFFFF
#define RTR_FLAG 0x80000000
#define EXT_FLAG 0x40000000
#define TS_FLAG  0x20000000

struct can_msg
{
    uint32_t id;
    uint32_t flags;
    uint32_t timestamp;
    uint8_t  len;
    uint8_t  data[8];
};

typedef void(*usbcan_cb)(uint32_t dev, uint32_t bus, struct can_msg *msg, void *arg);

struct usbcan_filter
{
    uint32_t flags;
    uint32_t id;
    uint32_t mask;
};

struct usbcan_bus_config
{
    uint32_t              speed;
    struct usbcan_filter *filters;
    uint8_t               num_filters;
    usbcan_cb             cb;
    void                 *arg;
};

uint32_t usbcan_library_init();
uint32_t usbcan_library_close();

uint32_t usbcan_init(uint32_t dev, uint32_t bus, struct usbcan_bus_config *config);
uint32_t usbcan_start(uint32_t dev, uint32_t bus);
uint32_t usbcan_reset(uint32_t dev, uint32_t bus);
uint32_t usbcan_stop(uint32_t dev, uint32_t bus);

uint32_t usbcan_send(uint32_t dev, uint32_t bus, struct can_msg *msg);
uint32_t usbcan_send(uint32_t dev, uint32_t bus, struct can_msg *msgs, uint32_t len);

uint32_t usbcan_set_filters(uint32_t dev, uint32_t bus, struct usbcan_filter *filters, uint8_t num_filters);
uint32_t usbcan_clear_filters(uint32_t dev, uint32_t bus);
uint32_t usbcan_register_callback(uint32_t dev, uint32_t bus, usbcan_cb callback, void *arg);
uint32_t usbcan_deregister_callback(uint32_t dev, uint32_t bus);



