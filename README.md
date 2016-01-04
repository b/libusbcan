# libusbcan

libusbcan provides a simple, pure C API for the ViewTool Ginkgo USB-CAN interfaces. It also adds functionality absent
from the standard Viewtool API. The most significant are callbacks for received messages are per bus rather than per
device and callbacks can have an argument associated with them. Where possible, the Linux SocketCAN data structures are
used.

# Library lifecycle

libusbcan does some internal bookkeeping that must be performed explicitly before and after it is used.

	uint32_t usbcan_library_init();
	uint32_t usbcan_library_close();

# Application lifecycle

	uint32_t usbcan_init(uint32_t dev, uint32_t bus, struct usbcan_bus_config *config);

	struct usbcan_bus_config
	{
		uint32_t           speed;
		struct can_filter *filters;
		uint8_t            num_filters;
		usbcan_cb          cb;
		void              *arg;
	};

	CAN_SPEED_1000KBPS
	CAN_SPEED_500KBPS
	CAN_SPEED_250KBPS
	CAN_SPEED_125KBPS
	CAN_SPEED_100KBPS
	CAN_SPEED_83KBPS
	CAN_SPEED_50KBPS
	CAN_SPEED_20KBPS
	CAN_SPEED_10KBPS
	
	uint32_t usbcan_start(uint32_t dev, uint32_t bus);
	uint32_t usbcan_reset(uint32_t dev, uint32_t bus);
	uint32_t usbcan_stop(uint32_t dev, uint32_t bus);

# Sending and receiving messages

	struct usbcan_msg
	{
		uint32_t timestamp;
		struct can_frame frame;
	};

	void(*usbcan_cb)(uint32_t dev, uint32_t bus, struct usbcan_msg *msg, void *arg);

	uint32_t usbcan_send(uint32_t dev, uint32_t bus, struct can_frame *frame);
	uint32_t usbcan_send_n(uint32_t dev, uint32_t bus, struct can_frame *frames, uint32_t len);

# Examples

