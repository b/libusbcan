#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "usbcan.h"
#include "ginkgo.h"

struct usbcan_cb_entry
{
    uint32_t        dev;
    uint32_t        bus;
    usbcan_cb       cb;
    void           *arg;
    struct usbcan_cb_entry *next;
};

struct usbcan_state
{
    uint32_t type;
    uint32_t num_devs;
    struct usbcan_cb_entry *callbacks;
    struct usbcan_cb_entry *tail;
};

struct usbcan_state state;

uint32_t
usbcan_library_init()
{
    state.callbacks = state.tail = NULL;

    state.num_devs = VCI_ScanDevice(1);
    if(state.num_devs == 0)
	{
	    return USBCAN_ERROR;
	}

    state.type = VCI_USBCAN2;
	
    return USBCAN_OK;
}

uint32_t
usbcan_init(uint32_t dev, uint32_t bus, struct usbcan_bus_config *config)
{
    if (state.num_devs <= dev)
	{
	    return USBCAN_ERROR;
	}
    
    int status = VCI_OpenDevice(state.type, dev, bus);
    if(status == STATUS_ERR)
	{
	    return USBCAN_ERROR;
	}

    VCI_INIT_CONFIG_EX init_config;
    init_config.CAN_ABOM = 0;
    init_config.CAN_Mode = 0;
    init_config.CAN_BRP = CAN_SPEEDS[config->speed][0];
    init_config.CAN_BS1 = CAN_SPEEDS[config->speed][1];
    init_config.CAN_BS2 = CAN_SPEEDS[config->speed][2];
    init_config.CAN_SJW = CAN_SPEEDS[config->speed][3];
    init_config.CAN_NART = 0;
    init_config.CAN_RFLM = 0;
    init_config.CAN_TXFP = 1;
    init_config.CAN_RELAY = 0;

    status = VCI_InitCANEx(state.type, dev, bus, &init_config);
    if(status == STATUS_ERR)
	{
	    return USBCAN_ERROR;
	}

    if (config->num_filters == 0)
	{
	    usbcan_clear_filters(dev, bus);
	}
    else
	{
	    usbcan_set_filters(dev, bus, config->filters, config->num_filters);
	}
    
    usbcan_register_callback(dev, bus, config->cb, config->arg);
    
    return USBCAN_OK;
}

uint32_t
usbcan_start(uint32_t dev, uint32_t bus)
{
    int status = VCI_StartCAN(state.type, dev, bus);
    if(status == STATUS_ERR)
	{
	    return USBCAN_ERROR;
	}

    return USBCAN_OK;
}

uint32_t
usbcan_reset(uint32_t dev, uint32_t bus)
{
    VCI_ResetCAN(type, dev, bus);

    return USBCAN_OK;
}

uint32_t
usbcan_stop(uint32_t dev, uint32_t bus)
{
    usbcan_deregister_callback(dev, bus);
    usbcan_reset(dev, bus);

    return USBCAN_OK;
}

uint32_t
usbcan_set_filters(uint32_t dev, uint32_t bus, struct usbcan_filter *filters, uint8_t num_filters)
{
    int status;

    usbcan_clear_filters(dev, bus);

    if (num_filters >= MAX_FILTERS)
	{
	    return USBCAN_ERROR;
	}

    for (int i = 0; i < num_filters; i++)
	{
	    VCI_FILTER_CONFIG filter_config;
	    filter_config.FilterIndex = i;
	    filter_config.Enable = 1;
	    filter_config.ExtFrame = 0;
	    filter_config.FilterMode = 1;
	    filter_config.ID_IDE = 0;
	    filter_config.ID_RTR = 0;
	    filter_config.ID_Std_Ext = filters[i].id;
	    filter_config.MASK_IDE = 0;
	    filter_config.MASK_RTR = 0;
	    filter_config.MASK_Std_Ext = 0;

	    status = VCI_SetFilter(state.type, dev, bus, &filter_config);
	    if(status == STATUS_ERR)
		{
		    return USBCAN_ERROR;
		}
	}

    return USBCAN_OK;
}

uint32_t
usbcan_clear_filters(uint32_t dev, uint32_t bus)
{
    int status;
    
    for (int i = 0; i < MAX_FILTERS; i++)
	{
	    VCI_FILTER_CONFIG filter_config;
	    filter_config.FilterIndex = i;
	    filter_config.Enable = 1;
	    filter_config.ExtFrame = 0;
	    filter_config.FilterMode = 1;
	    filter_config.ID_IDE = 0;
	    filter_config.ID_RTR = 0;
	    filter_config.ID_Std_Ext = 0;
	    filter_config.MASK_IDE = 0;
	    filter_config.MASK_RTR = 0;
	    filter_config.MASK_Std_Ext = 0;
	    
	    status = VCI_SetFilter(state.type, dev, bus, &filter_config);
	    if(status == STATUS_ERR)
		{
		    return USBCAN_ERROR;
		}
	}

    return USBCAN_OK;
}

struct usbcan_cb_entry*
usbcan_get_callback(uint32_t dev, uint32_t bus)
{
    struct usbcan_cb_entry *cbe;

    cbe = state.callbacks;
    while(cbe != NULL)
	{
	    if (cbe->dev == dev && cbe->bus == bus)
		{
		    return cbe;
		}
	    cbe = cbe->next;
	}

    return NULL;
}

void
usbcan_callback_dispatcher(uint32_t dev, uint32_t bus, uint32_t len)
{
    struct usbcan_cb_entry *cbe;
    if ((cbe = usbcan_get_callback(dev, bus)) != NULL)
	{
	    int msgs_avail = VCI_GetReceiveNum(state.type, dev, bus);
	    for (int i = 0; i < msgs_avail; i++)
		{
		    VCI_CAN_OBJ vci_msg;
		    struct can_msg msg;
		    
		    int msgs_read = VCI_Receive(state.type, dev, bus, &vci_msg, 1);
		    if (msgs_read == 1)
			{
			    msg.id = vci_msg.ID;
			    
			    if (vci_msg.TimeFlag > 0)
				{
				    msg.flags |= TS_FLAG;
				    msg.timestamp = vci_msg.TimeStamp; 
				}
			    
			    if (vci_msg.RemoteFlag > 0)
				{
				    msg.flags |= RTR_FLAG;
				}

			    if (vci_msg.ExternFlag > 0)
				{
				    msg.flags |= EXT_FLAG;
				}
			    
			    msg.len = vci_msg.DataLen;
			    for (int j = 0; j < vci_msg.DataLen; j++)
				{
				    msg.data[j] = vci_msg.Data[j];
				}
			    
			    cbe->cb(dev, bus, &msg, cbe->arg);
			}
		}
	}
}

uint32_t
usbcan_register_callback(uint32_t dev, uint32_t bus, usbcan_cb cb, void *arg)
{
    struct usbcan_cb_entry *cbe;
    if ((cbe = usbcan_get_callback(dev, bus)) != NULL)
	{
	    return USBCAN_ERROR;
	}

    cbe = (struct usbcan_cb_entry *)malloc(sizeof(struct usbcan_cb_entry));
    cbe->dev = dev;
    cbe->bus = bus;
    cbe->cb = cb;
    cbe->arg = arg;
    cbe->next = NULL;
    
    state.tail->next = cbe;
    state.tail = cbe;
    
    return USBCAN_OK;
}

uint32_t
usbcan_deregister_callback(uint32_t dev, uint32_t bus)
{
    struct usbcan_cb_entry *cbe, *pred_cbe = state.callbacks;
    if ((cbe = usbcan_get_callback(dev, bus)) != NULL)
	{
	    if (pred_cbe != cbe)
		{
		    while (pred_cbe->next != cbe)
			{
			    pred_cbe = pred_cbe->next;
			}
		}

	    pred_cbe->next = cbe->next;
	    
	    if (state.tail == cbe)
		{
		    state.tail = pred_cbe;
		}
	    
	    free(cbe);

	    return USBCAN_OK;
	}

    return USBCAN_ERROR;
}
