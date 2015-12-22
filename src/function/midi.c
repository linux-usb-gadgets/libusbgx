/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "usbg/usbg.h"
#include "usbg/usbg_internal.h"

#include <malloc.h>
#ifdef HAS_LIBCONFIG
#include <libconfig.h>
#endif

static int midi_set_attrs(struct usbg_function *f,
			  const usbg_function_attrs *f_attrs)
{
	const usbg_f_midi_attrs *attrs = &f_attrs->attrs.midi;
	int ret = USBG_ERROR_INVALID_PARAM;

	if (f_attrs->header.attrs_type &&
	    f_attrs->header.attrs_type != USBG_F_ATTRS_MIDI)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "index", attrs->index);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_string(f->path, f->name, "id", attrs->id);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "in_ports", attrs->in_ports);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "out_ports", attrs->out_ports);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "buflen", attrs->buflen);
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_write_dec(f->path, f->name, "qlen", attrs->qlen);

out:
	return ret;

}

static int midi_get_attrs(struct usbg_function *f,
			  usbg_function_attrs *f_attrs)
{
	int ret;
	usbg_f_midi_attrs *attrs = &f_attrs->attrs.midi;

	ret = usbg_read_dec(f->path, f->name, "index", &(attrs->index));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_string_alloc(f->path, f->name, "id", &(attrs->id));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_dec(f->path, f->name, "in_ports", (int*)&(attrs->in_ports));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_dec(f->path, f->name, "out_ports", (int*)&(attrs->out_ports));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_dec(f->path, f->name, "buflen", (int*)&(attrs->buflen));
	if (ret != USBG_SUCCESS)
		goto out;

	ret = usbg_read_dec(f->path, f->name, "qlen", (int*)&(attrs->qlen));
	if (ret != USBG_SUCCESS)
		goto out;

	f_attrs->header.attrs_type = USBG_F_ATTRS_MIDI;
out:
	return ret;
}

static void midi_cleanup_attrs(struct usbg_function *f,
			       usbg_function_attrs *f_attrs)
{
	free((char*)f_attrs->attrs.midi.id);
	f_attrs->attrs.midi.id = NULL;
}

#ifdef HAS_LIBCONFIG

static int midi_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	int ret = USBG_ERROR_NO_MEM;
	int tmp;
	usbg_function_attrs attrs;
	usbg_f_midi_attrs *midi_attrs = &attrs.attrs.midi;

	attrs.header.attrs_type = USBG_F_ATTRS_MIDI;

#define ADD_F_MIDI_INT_ATTR(attr, defval, minval)			\
	do {								\
		node = config_setting_get_member(root, #attr);		\
		if (node) {						\
			if (!usbg_config_is_int(node)) {		\
				ret = USBG_ERROR_INVALID_TYPE;		\
				goto out;				\
			}						\
			tmp = config_setting_get_int(node);		\
			if (tmp < minval) {				\
				ret = USBG_ERROR_INVALID_VALUE;		\
				goto out;				\
			}						\
			midi_attrs->attr = tmp;				\
		} else {						\
			midi_attrs->attr = defval;			\
		}							\
	} while (0)

	ADD_F_MIDI_INT_ATTR(index, -1, INT_MIN);
	ADD_F_MIDI_INT_ATTR(in_ports, 1, 0);
	ADD_F_MIDI_INT_ATTR(out_ports, 1, 0);
	ADD_F_MIDI_INT_ATTR(buflen, 256, 0);
	ADD_F_MIDI_INT_ATTR(qlen, 32, 0);

#undef ADD_F_MIDI_INT_ATTR

	node = config_setting_get_member(root, "id");
	if (node) {
		if (!usbg_config_is_string(node)) {
			ret = USBG_ERROR_INVALID_TYPE;
			goto out;
		}

		midi_attrs->id = config_setting_get_string(node);
	} else {
		midi_attrs->id = "";
	}


	ret = usbg_set_function_attrs(f, &attrs);
out:
	return ret;
}

static int midi_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	config_setting_t *node;
	int cfg_ret, usbg_ret;
	usbg_function_attrs f_attrs;
	usbg_f_midi_attrs *attrs = &f_attrs.attrs.midi;
	int ret = USBG_ERROR_NO_MEM;

	usbg_ret = midi_get_attrs(f, &f_attrs);
	if (usbg_ret != USBG_SUCCESS) {
		ret = usbg_ret;
		goto out;
	}

#define ADD_F_MIDI_INT_ATTR(attr, minval)				\
	do { 								\
		if ((int)attrs->attr < minval) {			\
			ret = USBG_ERROR_INVALID_VALUE;			\
			goto cleanup;					\
		}							\
		node = config_setting_add(root, #attr, CONFIG_TYPE_INT);\
		if (!node) 						\
			goto cleanup; 					\
		cfg_ret = config_setting_set_int(node, attrs->attr); 	\
		if (cfg_ret != CONFIG_TRUE) { 				\
			ret = USBG_ERROR_OTHER_ERROR; 			\
			goto cleanup; 					\
		}							\
	} while (0)

	ADD_F_MIDI_INT_ATTR(index, INT_MIN);

	node = config_setting_add(root, "id", CONFIG_TYPE_STRING);
	if (!node)
		goto cleanup;

	cfg_ret = config_setting_set_string(node, attrs->id);
	if (cfg_ret != CONFIG_TRUE) {
		ret = USBG_ERROR_OTHER_ERROR;
		goto cleanup;
	}

	ADD_F_MIDI_INT_ATTR(in_ports, 0);
	ADD_F_MIDI_INT_ATTR(out_ports, 0);
	ADD_F_MIDI_INT_ATTR(buflen, 0);
	ADD_F_MIDI_INT_ATTR(qlen, 0);

#undef ADD_F_MIDI_INT_ATTR

	ret = USBG_SUCCESS;
cleanup:
	midi_cleanup_attrs(f, &f_attrs);
out:
	return ret;
}

#endif /* HAS_LIBCONFIG */

struct usbg_function_type usbg_f_type_midi = {
	.name = "midi",
	.set_attrs = midi_set_attrs,
	.get_attrs = midi_get_attrs,
	.cleanup_attrs = midi_cleanup_attrs,

#ifdef HAS_LIBCONFIG
	.import = midi_libconfig_import,
	.export = midi_libconfig_export,
#endif
};

