#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

#define WMO_VID 0x045e
#define WMO_PID 0x028e

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yum13241");
MODULE_DESCRIPTION("Filter kernel module to set the polling rate of generic controllers to a custom value on XHCI.");
MODULE_VERSION("1.0");

static struct usb_device* adapter_device = NULL;

static unsigned short restore_interval = 8;
static unsigned short configured_interval = 1;

/* Patches all applicable endpoints. Returns the bInterval value used before patching. */
static unsigned short patch_endpoints(unsigned short interval) {
	static unsigned short old_interval = 8;

	if(adapter_device != NULL && adapter_device->actconfig != NULL) {
		struct usb_interface* interface = adapter_device->actconfig->interface[0];

		if(interface != NULL) {
			for(unsigned int altsetting = 0; altsetting < interface->num_altsetting; altsetting++) {
				struct usb_host_interface* altsettingptr = &interface->altsetting[altsetting];

				for(__u8 endpoint = 0; endpoint < altsettingptr->desc.bNumEndpoints; endpoint++) {
					if(altsettingptr->endpoint[endpoint].desc.bEndpointAddress == 0x81 || altsettingptr->endpoint[endpoint].desc.bEndpointAddress == 0x02) {
						old_interval = altsettingptr->endpoint[endpoint].desc.bInterval;
						altsettingptr->endpoint[endpoint].desc.bInterval = interval;

						printk(KERN_INFO "wmo_oc: bInterval value of endpoint 0x%.2x set to %u.\n", altsettingptr->endpoint[endpoint].desc.bEndpointAddress, interval);
					}
				}
			}

			/*
			 * Attempt to lock the device.
			 * This is required by the kernel documentation but it seems that some systems won't let you lock the USB device.
			 * Older versions before 1.2 never called this function and still worked so we proceed even if locking fails.
			 */
			int ret = usb_lock_device_for_reset(adapter_device, NULL);
			if(ret) {
				printk(KERN_ERR "wmo_oc: Warning! Failed to acquire lock for USB device (error: %d). Resetting device anyway...\n", ret);
			}
			/* TODO: It might be possible to make the new bInterval value take effect without calling usb_reset_device? */
			if(usb_reset_device(adapter_device)) {
				printk(KERN_ERR "wmo_oc: Could not reset device (error: %d). bInterval value was NOT changed.\n", ret);
			}
			/* Only unlock the device if usb_lock_device_for_reset succeeded. */
			if(!ret) {
				usb_unlock_device(adapter_device);
			}
		}
	}

	return old_interval;
}

static int on_usb_notify(struct notifier_block* self, unsigned long action, void* _device) {
	struct usb_device* device = _device;

	switch(action) {
		case USB_DEVICE_ADD:
			if(device->descriptor.idVendor == WMO_VID && device->descriptor.idProduct == WMO_PID && adapter_device == NULL) {
				adapter_device = device;
				printk(KERN_INFO "wmo_oc: Overclockable controller connected\n");

				restore_interval = patch_endpoints(configured_interval);
			}
			break;

		case USB_DEVICE_REMOVE:
			if(adapter_device == device) {
				adapter_device = NULL;
				printk(KERN_INFO "wmo_oc: Overclockable controller disconnected\n");
			}
			break;
	}

	return NOTIFY_OK;
}

static struct notifier_block usb_nb = { .notifier_call = on_usb_notify };

static int usb_device_cb(struct usb_device* device, void* data) {
	if(device->descriptor.idVendor == WMO_VID && device->descriptor.idProduct == WMO_PID && adapter_device == NULL) {
		adapter_device = device;
		printk(KERN_INFO "wmo_oc: adapter_device is null, why?\n");

		restore_interval = patch_endpoints(configured_interval);
	}

	return 0;
}

static int __init on_module_init(void) {
	if(configured_interval > 255) {
		printk(KERN_WARNING "wmo_oc: Invalid interval parameter specified.\n");
		configured_interval = 255;
	}

	if(configured_interval == 0) {
		printk(KERN_WARNING "wmo_oc: Invalid interval parameter specified.\n");
		configured_interval = 1;
	}

	usb_for_each_dev(NULL, &usb_device_cb);
	usb_register_notify(&usb_nb);

	return 0;
}

static void __exit on_module_exit(void) {
	if(adapter_device != NULL) {
		patch_endpoints(restore_interval);
	}

	usb_unregister_notify(&usb_nb);
}

module_init(on_module_init);
module_exit(on_module_exit);

static int on_interval_changed(const char* value, const struct kernel_param* kp) {
	int ret = param_set_ushort(value, kp);

	if(!ret) {
		if(configured_interval > 255) {
			printk(KERN_WARNING "wmo_oc: Invalid interval parameter specified.\n");
			configured_interval = 255;
		}
		else if(configured_interval == 0) {
			printk(KERN_WARNING "wmo_oc: Invalid interval parameter specified.\n");
			configured_interval = 1;
		}

		patch_endpoints(configured_interval);
	}

	return ret;
}

static struct kernel_param_ops interval_ops = {
	.set = &on_interval_changed,
	.get = &param_get_ushort
};

module_param_cb(rate, &interval_ops, &configured_interval, 0644);
MODULE_PARM_DESC(rate, "Polling rate (default: 1)");
