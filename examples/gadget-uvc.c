/*
 * Thomas Haemmerle <thomas.haemmerle@wolfvision.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/**
 * @file gadget-uvc.c
 * @example gadget-uvc.c
 * This is an example of how to create UVC gadget. Basically it provides the same
 * functionality as the example provided in Documentation/usb/gadget-testing.txt
 * in linux kernel repository.
 */

#include <errno.h>
#include <stdio.h>
#include <linux/usb/ch9.h>
#include <usbg/usbg.h>
#include <usbg/function/uvc.h>

#define VENDOR          0x1d6b
#define PRODUCT         0x0104

int main(void)
{
	usbg_state *s;
	usbg_gadget *g;
	usbg_config *c;
	usbg_function *f_uvc;
	int ret = -EINVAL;
	int usbg_ret;

	struct usbg_gadget_attrs g_attrs = {
		.bcdUSB = 0x0200,
		.bDeviceClass = USB_CLASS_PER_INTERFACE,
		.bDeviceSubClass = 0x00,
		.bDeviceProtocol = 0x00,
		.bMaxPacketSize0 = 64, /* Max allowed ep0 packet size */
		.idVendor = VENDOR,
		.idProduct = PRODUCT,
		.bcdDevice = 0x0001, /* Verson of device */
	};

	struct usbg_gadget_strs g_strs = {
		.serial = "0123456789", /* Serial number */
		.manufacturer = "Foo Inc.", /* Manufacturer */
		.product = "Bar Gadget" /* Product string */
	};

	struct usbg_config_strs c_strs = { .configuration = "UVC" };

	struct usbg_f_uvc_frame_desc uncompressed_frames_array[] = {
		  {
			.name = "360p",
			.bmCapabilities = 0,
			.dwFrameInterval = "666666\n1000000\n5000000",
			.dwDefaultFrameInterval = 666666,
			.dwMaxVideoFrameBufferSize = 460800,
			.dwMaxBitRate = 55296000,
			.dwMinBitRate = 18432000,
			.wHeight = 360,
			.wWidth = 640
		  }
	};

	struct usbg_f_uvc_frame_desc *uncompressed_frames[] = {
		&uncompressed_frames_array[0],
		NULL,
	};

	struct usbg_f_uvc_u_format_desc streaming_uncompressed_array[] = {
	{
		.name = "u",
		//.bmaControls = 0x0,
		.bDefaultFrameIndex = 1,
		.bBitsPerPixel = 16,
		.guidFormat = {
			'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
			0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
		},
		.n_frame_descs = 1,
		.frame_descs = uncompressed_frames,
	}
	};

	struct usbg_f_uvc_u_format_desc *streaming_uncompressed[] = {
		&streaming_uncompressed_array[0],
		NULL,
	};

	struct usbg_f_uvc_streaming_header_desc streaming_header_array[] = {
	{
		.name = "h",
		.format_link = &streaming_uncompressed_array[0],
	}
	};

	struct usbg_f_uvc_streaming_header_desc *streaming_headers[] = {
		&streaming_header_array[0],
		NULL,
	};

	struct usbg_f_uvc_streaming_class_desc streaming_class = {
		.ss_header = &streaming_header_array[0],
		.hs_header = &streaming_header_array[0],
		.fs_header = &streaming_header_array[0],
	};

	struct usbg_f_uvc_control_header_desc control_headers_array[] = {
		{ .name = "h" }
	};

	struct usbg_f_uvc_control_header_desc *control_headers[] = {
		&control_headers_array[0],
		NULL,
	};

	struct usbg_f_uvc_control_class_desc control_class = {
		.ss_header = &control_headers_array[0],
		.fs_header = &control_headers_array[0],
	};

	struct usbg_f_uvc_control_desc control = {
		.bInterfaceNumber = 1,
		.class = &control_class,
		.terminal = NULL,
		.n_processing_descs = 0,
		.processing = NULL,
		.n_header_descs = 1,
		.header = control_headers,
	};

	struct usbg_f_uvc_streaming_desc streaming = {
		.bInterfaceNumber = 1,
		.class = &streaming_class,
		.n_color_matching_descs = 0,
		.color_matching = NULL,
		.n_mjpeg_format_descs = 0,
		.mjpeg = NULL,
		.n_uncompressed_format_descs = 1,
		.uncompressed = streaming_uncompressed,
		.n_header_descs = 1,
		.header = streaming_headers,
	};

	struct usbg_f_uvc_attrs uvc_attrs = {
		.streaming_interval = 1,
		.streaming_maxburst = 0,
		.streaming_maxpacket = 1024,
		.control = &control,
		.streaming = &streaming,
	};

	usbg_ret = usbg_init("/sys/kernel/config", &s);
	if(usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on USB gadget init\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out1;
	}

	usbg_ret = usbg_create_gadget(s, "g1", &g_attrs, &g_strs, &g);
	if(usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error on create gadget\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_create_function(g, USBG_F_UVC, "usb0", &uvc_attrs, &f_uvc);
	if(usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating f_uvc function\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	/* NULL can be passed to use kernel defaults */
	usbg_ret = usbg_create_config(g, 1, "The only one", NULL, &c_strs, &c);
	if(usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error creating config\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	usbg_ret = usbg_add_config_function(c, "some_name_here", f_uvc);
	if(usbg_ret != USBG_SUCCESS) {
		fprintf(stderr, "Error adding f_uvc\n");
		fprintf(stderr, "Error: %s : %s\n", usbg_error_name(usbg_ret),
				usbg_strerror(usbg_ret));
		goto out2;
	}

	fprintf(stdout, "UVC gadget has been created.\n");

	/*
	 * Enable your gadget:
	 * $ echo "my_udc_name" > /sys/kernel/config/usb_gadget/g1/UDC
	 *
	 * Here we end up with video device /dev/video*
	 *
	 * modprobe vivid
	 *
	 * uvc-gadget -u /dev/video<uvc video node #> -v /dev/video<vivid video node #>
	 *
	 * where uvc-gadget is this program:
	 * http://git.ideasonboard.org/uvc-gadget.git
	 *
	 * with these patches:
	 * http://www.spinics.net/lists/linux-usb/msg99220.html
	 *
	 */

	ret = 0;

	out2: usbg_cleanup(s);

	out1: return ret;
}
