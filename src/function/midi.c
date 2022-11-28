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
#include "usbg/function/midi.h"

#include <malloc.h>
#ifdef HAS_GADGET_SCHEMES
#include <libconfig.h>
#endif

struct usbg_f_midi {
	struct usbg_function func;
};

#define MIDI_DEC_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_midi_attrs, _name),    \
		.get = usbg_get_dec,				        \
		.set = usbg_set_dec,				        \
		.import = usbg_get_config_node_int,	                \
		.export = usbg_set_config_node_int,		        \
	}

#define MIDI_STRING_ATTR(_name)						\
	{								\
		.name = #_name,						\
		.offset = offsetof(struct usbg_f_midi_attrs, _name),    \
		.get = usbg_get_string,				        \
		.set = usbg_set_string,				        \
		.import = usbg_get_config_node_string,		        \
		.export = usbg_set_config_node_string,		        \
	}

struct {
	const char *name;
	size_t offset;
	usbg_attr_get_func get;
	usbg_attr_set_func set;
	usbg_import_node_func import;
	usbg_export_node_func export;
} midi_attr[USBG_F_MIDI_ATTR_MAX] = {
	[USBG_F_MIDI_INDEX] = MIDI_DEC_ATTR(index),
	[USBG_F_MIDI_ID] = MIDI_STRING_ATTR(id),
	[USBG_F_MIDI_IN_PORTS] = MIDI_DEC_ATTR(in_ports),
	[USBG_F_MIDI_OUT_PORTS] = MIDI_DEC_ATTR(out_ports),
	[USBG_F_MIDI_BUFLEN] = MIDI_DEC_ATTR(buflen),
	[USBG_F_MIDI_QLEN] = MIDI_DEC_ATTR(qlen),
};

#undef MIDI_DEC_ATTR
#undef MIDI_STRING_ATTR

GENERIC_ALLOC_INST(midi, struct usbg_f_midi, func)

GENERIC_FREE_INST(midi, struct usbg_f_midi, func)

static int midi_set_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_midi_set_attrs(usbg_to_midi_function(f), f_attrs);
}

static int midi_get_attrs(struct usbg_function *f, void *f_attrs)
{
	return usbg_f_midi_get_attrs(usbg_to_midi_function(f), f_attrs);
}

static void midi_cleanup_attrs(struct usbg_function *f, void *f_attrs)
{
	usbg_f_midi_cleanup_attrs(f_attrs);
}

#ifdef HAS_GADGET_SCHEMES

static int midi_libconfig_import(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_midi *mf = usbg_to_midi_function(f);
	union usbg_f_midi_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_MIDI_ATTR_MIN; i < USBG_F_MIDI_ATTR_MAX; ++i) {
		ret = midi_attr[i].import(root, midi_attr[i].name, &val);
		/* node not  found */
		if (ret == 0)
			continue;
		/* error */
		if (ret < 0)
			break;

		ret = usbg_f_midi_set_attr_val(mf, i, &val);
		if (ret)
			break;
	}

	return ret;
}

static int midi_libconfig_export(struct usbg_function *f,
				  config_setting_t *root)
{
	struct usbg_f_midi *mf = usbg_to_midi_function(f);
	union usbg_f_midi_attr_val val;
	int i;
	int ret = 0;

	for (i = USBG_F_MIDI_ATTR_MIN; i < USBG_F_MIDI_ATTR_MAX; ++i) {
		ret = usbg_f_midi_get_attr_val(mf, i, &val);
		if (ret)
			break;

		ret = midi_attr[i].export(root, midi_attr[i].name, &val);
		if (ret)
			break;
	}

	return ret;
}

#endif /* HAS_GADGET_SCHEMES */

struct usbg_function_type usbg_f_type_midi = {
	.name = "midi",
	.alloc_inst = midi_alloc_inst,
	.free_inst = midi_free_inst,
	.set_attrs = midi_set_attrs,
	.get_attrs = midi_get_attrs,
	.cleanup_attrs = midi_cleanup_attrs,

#ifdef HAS_GADGET_SCHEMES
	.import = midi_libconfig_import,
	.export = midi_libconfig_export,
#endif
};

/* API implementation */

usbg_f_midi *usbg_to_midi_function(usbg_function *f)
{
	return f->ops == &usbg_f_type_midi ?
		container_of(f, struct usbg_f_midi, func) : NULL;
}

usbg_function *usbg_from_midi_function(usbg_f_midi *mf)
{
	return &mf->func;
}

int usbg_f_midi_get_attrs(usbg_f_midi *mf,
			  struct usbg_f_midi_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_MIDI_ATTR_MIN; i < USBG_F_MIDI_ATTR_MAX; ++i) {
		ret = usbg_f_midi_get_attr_val(mf, i,
					       (union usbg_f_midi_attr_val *)
					       ((char *)attrs
						+ midi_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_midi_set_attrs(usbg_f_midi *mf,
			 const struct usbg_f_midi_attrs *attrs)
{
	int i;
	int ret = 0;

	for (i = USBG_F_MIDI_ATTR_MIN; i < USBG_F_MIDI_ATTR_MAX; ++i) {
		ret = usbg_f_midi_set_attr_val(mf, i,
					       (const union usbg_f_midi_attr_val *)
					       ((const char *)attrs
						+ midi_attr[i].offset));
		if (ret)
			break;
	}

	return ret;

}

int usbg_f_midi_get_attr_val(usbg_f_midi *mf, enum usbg_f_midi_attr attr,
			    union usbg_f_midi_attr_val *val)
{
	return midi_attr[attr].get(mf->func.path, mf->func.name,
				    midi_attr[attr].name, val);
}

int usbg_f_midi_set_attr_val(usbg_f_midi *mf, enum usbg_f_midi_attr attr,
			    const union usbg_f_midi_attr_val *val)
{
	return midi_attr[attr].set(mf->func.path, mf->func.name,
				    midi_attr[attr].name, val);
}

int usbg_f_midi_get_id_s(usbg_f_midi *mf, char *buf, int len)
{
	struct usbg_function *f;
	int ret;

	if (!mf || !buf)
		return USBG_ERROR_INVALID_PARAM;

	f = &mf->func;
	/*
	 * TODO:
	 * Rework usbg_common to make this function consistent with doc.
	 * This below is only an ugly hack
	 */
	ret = usbg_read_string_limited(f->path, f->name, "id", buf, len);
	if (ret)
		goto out;

	ret = strlen(buf);
out:
	return ret;
}
