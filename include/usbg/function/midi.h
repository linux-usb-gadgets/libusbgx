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

#ifndef USBG_FUNCTION_MIDI__
#define USBG_FUNCTION_MIDI__

#include <usbg/usbg.h>

#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_midi;
typedef struct usbg_f_midi usbg_f_midi;

struct usbg_f_midi_attrs {
	int index;
	const char *id;
	unsigned int in_ports;
	unsigned int out_ports;
	unsigned int buflen;
	unsigned int qlen;
};

enum usbg_f_midi_attr {
	USBG_F_MIDI_ATTR_MIN = 0,
	USBG_F_MIDI_INDEX = USBG_F_MIDI_ATTR_MIN,
	USBG_F_MIDI_ID,
	USBG_F_MIDI_IN_PORTS,
	USBG_F_MIDI_OUT_PORTS,
	USBG_F_MIDI_BUFLEN,
	USBG_F_MIDI_QLEN,
	USBG_F_MIDI_ATTR_MAX
};

union usbg_f_midi_attr_val {
	int index;
	const char *id;
	unsigned int in_ports;
	unsigned int out_ports;
	unsigned int buflen;
	unsigned int qlen;
};

#define USBG_F_MIDI_INT_TO_ATTR_VAL(WHAT)		\
	USBG_TO_UNION(usbg_f_midi_attr_val, index, WHAT)

#define USBG_F_MIDI_UINT_TO_ATTR_VAL(WHAT)		\
	USBG_TO_UNION(usbg_f_midi_attr_val, qlen, WHAT)

#define USBG_F_MIDI_CCHAR_PTR_TO_ATTR_VAL(WHAT)		\
	USBG_TO_UNION(usbg_f_midi_attr_val, id, WHAT)

/**
 * @brief Cast from generic function to midi function
 * @param[in] f function to be converted to midi funciton.
 *         Function should be one of type midi.
 * @return Converted midi function or NULL if function hasn't suitable type
 */
usbg_f_midi *usbg_to_midi_function(usbg_function *f);

/**
 * @brief Cast form midi function to generic one
 * @param[in] mf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_midi_function(usbg_f_midi *mf);

/**
 * @brief Get attributes of given midi function
 * @param[in] mf Pointer to midi function
 * @param[out] attrs Structure to be filled with data
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_midi_get_attrs(usbg_f_midi *mf,
			  struct usbg_f_midi_attrs *attrs);

/**
 * @brief Set attributes of given midi function
 * @param[in] mf Pointer to midi function
 * @param[in] attrs to be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_midi_set_attrs(usbg_f_midi *mf,
			 const struct usbg_f_midi_attrs *attrs);

/**
 * @brief Cleanup attributes structure after usage
 * @param[in] attrs to be cleaned up
 */
static inline void usbg_f_midi_cleanup_attrs(struct usbg_f_midi_attrs *attrs)
{
	if (attrs) {
		free((char *)attrs->id);
		/* Just for safety */
		attrs->id = NULL;
	}
}

/**
 * @brief Get the value of single attribute
 * @param[in] mf Pointer to midi function
 * @param[in] attr Code of attribute which value should be taken
 * @param[out] val Current value of this attribute
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_midi_get_attr_val(usbg_f_midi *mf, enum usbg_f_midi_attr attr,
			    union usbg_f_midi_attr_val *val);

/**
 * @brief Set the value of single attribute
 * @param[in] mf Pointer to midi function
 * @param[in] attr Code of attribute which value should be set
 * @param[in] val Value of attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_midi_set_attr_val(usbg_f_midi *mf, enum usbg_f_midi_attr attr,
			    const union usbg_f_midi_attr_val *val);

/**
 * @brief Get the index value of MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[out] index Current index value of MIDI adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_get_index(usbg_f_midi *mf, int *index)
{
	return usbg_f_midi_get_attr_val(mf, USBG_F_MIDI_INDEX,
					(union usbg_f_midi_attr_val *)index);
}

/**
 * @brief Set the index value of MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[in] index value which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_set_index(usbg_f_midi *mf, int index)
{
	return usbg_f_midi_set_attr_val(mf, USBG_F_MIDI_INDEX,
					&USBG_F_MIDI_INT_TO_ATTR_VAL(index));
}

/**
 * @brief Get the value of ID string for associated MIDI adapter
 *        into newly allocated storage
 * @param[in] mf Pointer to midi function
 * @param[out] id Newly allocated string containing id string of MIDI adapter
 * @return 0 on success usbg_error if error occurred.
 * @note returned id should be free() by caller
 */
static inline int usbg_f_midi_get_id(usbg_f_midi *mf, char **id)
{
	return usbg_f_midi_get_attr_val(mf, USBG_F_MIDI_ID,
					(union usbg_f_midi_attr_val *)id);
}

/**
 * @brief Get the value of ID string for associated MIDI adapter
 *         into user buffer
 * @param[in] mf Pointer to midi function
 * @param[out] buf Place where id should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family). This may
 *         also return negative error code from usbg_error.
 * @note returned id should be free() by caller
 */
int usbg_f_midi_get_id_s(usbg_f_midi *mf, char *buf, int len);

/**
 * @brief Set the value of ID string for associated MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[in] id string for MIDI adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_set_id(usbg_f_midi *mf, const char *id)
{
	return usbg_f_midi_set_attr_val(mf, USBG_F_MIDI_ID,
					&USBG_F_MIDI_CCHAR_PTR_TO_ATTR_VAL(id));
}

/**
 * @brief Get the number of in ports of MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[out] in_ports Number of in ports of MIDI adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_get_in_ports(usbg_f_midi *mf, unsigned *in_ports)
{
	return usbg_f_midi_get_attr_val(mf, USBG_F_MIDI_IN_PORTS,
					(union usbg_f_midi_attr_val *)in_ports);
}

/**
 * @brief Set the number of in ports of MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[in] in_ports number of in ports of MIDI adapters which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_set_in_ports(usbg_f_midi *mf, unsigned in_ports)
{
	return usbg_f_midi_set_attr_val(mf, USBG_F_MIDI_IN_PORTS,
					&USBG_F_MIDI_UINT_TO_ATTR_VAL(in_ports));
}

/**
 * @brief Get the number of out ports of MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[out] out_ports Number of out ports of MIDI adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_get_out_ports(usbg_f_midi *mf, unsigned *out_ports)
{
	return usbg_f_midi_get_attr_val(mf, USBG_F_MIDI_OUT_PORTS,
					(union usbg_f_midi_attr_val *)out_ports);
}

/**
 * @brief Set the number of out ports of MIDI adapter
 * @param[in] mf Pointer to midi function
 * @param[in] out_ports number of out ports of MIDI adapters which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_set_out_ports(usbg_f_midi *mf, unsigned out_ports)
{
	return usbg_f_midi_set_attr_val(mf, USBG_F_MIDI_OUT_PORTS,
					&USBG_F_MIDI_UINT_TO_ATTR_VAL(out_ports));
}

/**
 * @brief Get the size of request buffer
 * @details This is the maximum number of bytes which can be received
 *        using single usb_request.
 * @param[in] lf Pointer to midi function
 * @param[out] buflen Current queue length
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_get_buflen(usbg_f_midi *mf, int *buflen)
{
	return usbg_f_midi_get_attr_val(mf, USBG_F_MIDI_BUFLEN,
					(union usbg_f_midi_attr_val *)buflen);
}

/**
 * @brief Set the size of request buffer
 * @details This is the maximum number of bytes which can be received
 *        using single usb_request.
 * @param[in] lf Pointer to midi function
 * @param[in] buflen Current queue length
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_set_buflen(usbg_f_midi *mf, unsigned buflen)
{
	return usbg_f_midi_set_attr_val(mf, USBG_F_MIDI_BUFLEN,
					&USBG_F_MIDI_UINT_TO_ATTR_VAL(buflen));
}

/**
 * @brief Get the value of request queue length
 * @param[in] lf Pointer to midi function
 * @param[out] qlen Current queue length
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_get_qlen(usbg_f_midi *mf, unsigned *qlen)
{
	return usbg_f_midi_get_attr_val(mf, USBG_F_MIDI_QLEN,
					(union usbg_f_midi_attr_val *)qlen);
}

/**
 * @brief Set the value of request queue length
 * @param[in] lf Pointer to midi function
 * @param[in] qlen Current queue length
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_midi_set_qlen(usbg_f_midi *mf, unsigned qlen)
{
	return usbg_f_midi_set_attr_val(mf, USBG_F_MIDI_QLEN,
					&USBG_F_MIDI_UINT_TO_ATTR_VAL(qlen));
}

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_MIDI__ */
