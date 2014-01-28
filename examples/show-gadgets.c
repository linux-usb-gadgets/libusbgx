/*
 * Copyright (C) 2013 Linaro Limited
 *
 * Matt Porter <mporter@linaro.org>
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ether.h>
#include <usbg/usbg.h>

/**
 * @file show-gadgets.c
 * @example show-gadgets.c
 * This example shows how to display all configured USB gadgets
 * in the system
 */

void show_gadget(struct gadget *g)
{
	fprintf(stdout, "ID %04x:%04x '%s'\n",
		g->attrs.vendor, g->attrs.product, g->name);
	fprintf(stdout, "  UDC\t\t\t%s\n", g->udc);
	fprintf(stdout, "  bDeviceClass\t\t0x%02x\n", g->attrs.dclass);
	fprintf(stdout, "  bDeviceSubClass\t0x%02x\n", g->attrs.dsubclass);
	fprintf(stdout, "  bDeviceProtocol\t0x%02x\n", g->attrs.dproto);
	fprintf(stdout, "  bMaxPacketSize0\t0x%02x\n", g->attrs.maxpacket);
	fprintf(stdout, "  bcdDevice\t\t0x%04x\n", g->attrs.bcddevice);
	fprintf(stdout, "  bcdUSB\t\t0x%04x\n", g->attrs.bcdusb);
	fprintf(stdout, "  idVendor\t\t0x%04x\n", g->attrs.vendor);
	fprintf(stdout, "  idProduct\t\t0x%04x\n", g->attrs.product);
	fprintf(stdout, "  Serial Number\t\t%s\n", g->str_ser);
	fprintf(stdout, "  Manufacturer\t\t%s\n", g->str_mnf);
	fprintf(stdout, "  Product\t\t%s\n", g->str_prd);
}

void show_function(struct function *f)
{
	fprintf(stdout, "  Function '%s'\n", f->name);
	switch (f->type) {
	case F_SERIAL:
	case F_ACM:
	case F_OBEX:
		fprintf(stdout, "    port_num\t\t%d\n",
			f->attr.serial.port_num);
		break;
	case F_ECM:
	case F_SUBSET:
	case F_NCM:
	case F_EEM:
	case F_RNDIS:
		fprintf(stdout, "    dev_addr\t\t%s\n",
			ether_ntoa(&f->attr.net.dev_addr));
		fprintf(stdout, "    host_addr\t\t%s\n",
			ether_ntoa(&f->attr.net.host_addr));
		fprintf(stdout, "    ifname\t\t%s\n", f->attr.net.ifname);
		fprintf(stdout, "    qmult\t\t%d\n", f->attr.net.qmult);
		break;
	case F_PHONET:
		fprintf(stdout, "    ifname\t\t%s\n", f->attr.phonet.ifname);
		break;
	default:
		fprintf(stdout, "    UNKNOWN\n");
	}
}

void show_config(struct config *c)
{
	struct binding *b;

	fprintf(stdout, "  Configuration '%s'\n", c->name);
	fprintf(stdout, "    MaxPower\t\t%d\n", c->maxpower);
	fprintf(stdout, "    bmAttributes\t0x%02x\n", c->bmattrs);
	fprintf(stdout, "    configuration\t%s\n", c->str_cfg);
	usbg_for_each_binding(b, c)
		fprintf(stdout, "    %s -> %s\n", b->name,b->target->name);
}

int main(void)
{
	struct state *s;
	struct gadget *g;
	struct function *f;
	struct config *c;
	struct binding *b;
	struct function *f_acm0, *f_acm1, *f_ecm;

	s = usbg_init("/sys/kernel/config");
	if (!s) {
		fprintf(stderr, "Error on USB gadget init\n");
		return -EINVAL;
	}

	usbg_for_each_gadget(g, s) {
		show_gadget(g);
		usbg_for_each_function(f, g)
			show_function(f);
		usbg_for_each_config(c, g)
			show_config(c);
	}

	usbg_cleanup(s);

	return 0;
}
