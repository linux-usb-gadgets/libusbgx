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

#ifndef USBG_FUNCTION_HID__
#define USBG_FUNCTION_HID__

#include <usbg/usbg.h>

#include <malloc.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_hid;
typedef struct usbg_f_hid usbg_f_hid;

struct usbg_f_hid_report_desc {
	char *desc;
	unsigned int len;
};

struct usbg_f_hid_attrs {
	dev_t dev;
	unsigned int protocol;
	struct usbg_f_hid_report_desc report_desc;
	unsigned int report_length;
	unsigned int subclass;
};

enum usbg_f_hid_attr {
	USBG_F_HID_ATTR_MIN = 0,
	USBG_F_HID_DEV = USBG_F_HID_ATTR_MIN,
	USBG_F_HID_PROTOCOL,
	USBG_F_HID_REPORT_DESC,
	USBG_F_HID_REPORT_LENGTH,
	USBG_F_HID_SUBCLASS,
	USBG_F_HID_ATTR_MAX
};

union usbg_f_hid_attr_val {
	dev_t dev;
	unsigned int protocol;
	struct usbg_f_hid_report_desc report_desc;
	unsigned int report_length;
	unsigned int subclass;
};

/**
 * @brief Cast from generic function to hid function
 * @param[in] f function to be converted to hid funciton.
 *         Function should be one of type hid.
 * @return Converted hid function or NULL if function hasn't suitable type
 */
usbg_f_hid *usbg_to_hid_function(usbg_function *f);

/**
 * @brief Cast form hid function to generic one
 * @param[in] hf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_hid_function(usbg_f_hid *hf);

/**
 * @brief Get attributes of given hid function
 * @param[in] hf Pointer to hid function
 * @param[out] attrs Structure to be filled with data
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_hid_get_attrs(usbg_f_hid *hf,
			  struct usbg_f_hid_attrs *attrs);

/**
 * @brief Set attributes of given hid function
 * @param[in] hf Pointer to hid function
 * @param[in] attrs to be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_hid_set_attrs(usbg_f_hid *hf,
			 const struct usbg_f_hid_attrs *attrs);


/**
 * @brief Cleanup HID report descriptor structure after usage
 * @param[in] report_desc to be cleaned up
 */
static inline void usbg_f_hid_cleanup_report_desc(
	struct usbg_f_hid_report_desc *report_desc)
{
	if (report_desc->desc) {
		free(report_desc->desc);
		/* Just for safety */
		report_desc->desc = NULL;
	}
}

/**
 * @brief Cleanup attributes structure after usage
 * @param[in] attrs to be cleaned up
 */
static inline void usbg_f_hid_cleanup_attrs(struct usbg_f_hid_attrs *attrs)
{
	if (attrs)
		usbg_f_hid_cleanup_report_desc(&attrs->report_desc);
}

/**
 * @brief Get the value of single attribute
 * @param[in] hf Pointer to hid function
 * @param[in] attr Code of attribute which value should be taken
 * @param[out] val Current value of this attribute
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_hid_get_attr_val(usbg_f_hid *hf, enum usbg_f_hid_attr attr,
			    union usbg_f_hid_attr_val *val);

/**
 * @brief Set the value of single attribute
 * @param[in] hf Pointer to hid function
 * @param[in] attr Code of attribute which value should be set
 * @param[in] val Value of attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_hid_set_attr_val(usbg_f_hid *hf, enum usbg_f_hid_attr attr,
			    const union usbg_f_hid_attr_val *val);

/**
 * @brief Get the minor and major of corresponding character device
 * @param[in] hf Pointer to hid function
 * @param[out] dev Minor and major of corresponding
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_get_dev(usbg_f_hid *hf, dev_t *dev)
{
	return usbg_f_hid_get_attr_val(hf, USBG_F_HID_DEV,
				       (union usbg_f_hid_attr_val *)dev);
}

/**
 * @brief Get HID protocol code
 * @param[in] hf Pointer to hid function
 * @param[out] protocol code
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_get_protocol(usbg_f_hid *hf,
					  unsigned int *protocol)
{
	return usbg_f_hid_get_attr_val(hf, USBG_F_HID_PROTOCOL,
				       (union usbg_f_hid_attr_val *)protocol);
}

/**
 * @brief Set HID protocol code
 * @param[in] hf Pointer to hid function
 * @param[out] protocol code
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_set_protocol(usbg_f_hid *hf,
					  unsigned int protocol)
{

	union usbg_f_hid_attr_val val = {.protocol = protocol};
	return usbg_f_hid_set_attr_val(hf, USBG_F_HID_PROTOCOL, &val);
}

/**
 * @brief Get HID report descriptor
 * @param[in] hf Pointer to hid function
 * @param[out] report_desc Report descriptor
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_get_report_desc(usbg_f_hid *hf,
					     struct usbg_f_hid_report_desc *report_desc)
{
	return usbg_f_hid_get_attr_val(hf, USBG_F_HID_REPORT_DESC,
				       (union usbg_f_hid_attr_val *)report_desc);
}

/**
 * @brief Set HID report descriptor
 * @param[in] hf Pointer to hid function
 * @param[out] report_desc Report descriptor
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_set_report_desc(usbg_f_hid *hf,
					     struct usbg_f_hid_report_desc report_desc)
{
	union usbg_f_hid_attr_val val = {.report_desc = report_desc};
	return usbg_f_hid_set_attr_val(hf, USBG_F_HID_REPORT_DESC, &val);
}

/**
 * @brief Get HID report descriptor
 * @param[in] hf Pointer to hid function
 * @param[out] report_desc Report descriptor
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_get_report_desc_raw(usbg_f_hid *hf,
						 char **desc,
						 unsigned int *len)
{
	struct usbg_f_hid_report_desc report_desc;
	int ret;

	ret = usbg_f_hid_get_attr_val(hf, USBG_F_HID_REPORT_DESC,
				      (union usbg_f_hid_attr_val *)&report_desc);
	if (ret != USBG_SUCCESS)
		return ret;

	*desc = report_desc.desc;
	*len = report_desc.len;

	return USBG_SUCCESS;
}

/**
 * @brief Set HID report descriptor
 * @param[in] hf Pointer to hid function
 * @param[out] report_desc Report descriptor
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_set_report_desc_raw(usbg_f_hid *hf,
						 char *desc,
						 unsigned int len)
{
	struct usbg_f_hid_report_desc report_desc = {
		.desc = desc,
		.len = len,
	};

	union usbg_f_hid_attr_val val = {.report_desc = report_desc};
	return usbg_f_hid_set_attr_val(hf, USBG_F_HID_REPORT_DESC, &val);
}

/**
 * @brief Get HID report length
 * @param[in] hf Pointer to hid function
 * @param[out] report_length Length of HID report
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_get_report_length(usbg_f_hid *hf,
					  unsigned int *report_length)
{
	return usbg_f_hid_get_attr_val(hf, USBG_F_HID_REPORT_LENGTH,
				       (union usbg_f_hid_attr_val *)report_length);
}

/**
 * @brief Set HID report length
 * @param[in] hf Pointer to hid function
 * @param[out] report_length Length of HID report
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_set_report_length(usbg_f_hid *hf,
					  unsigned int report_length)
{
	union usbg_f_hid_attr_val val = {.report_length = report_length};
	return usbg_f_hid_set_attr_val(hf, USBG_F_HID_REPORT_LENGTH, &val);
}

/**
 * @brief Get HID subclass code
 * @param[in] hf Pointer to hid function
 * @param[out] subclass code
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_get_subclass(usbg_f_hid *hf,
					  unsigned int *subclass)
{
	return usbg_f_hid_get_attr_val(hf, USBG_F_HID_SUBCLASS,
				       (union usbg_f_hid_attr_val *)subclass);
}

/**
 * @brief Set HID subclass code
 * @param[in] hf Pointer to hid function
 * @param[out] subclass code
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_hid_set_subclass(usbg_f_hid *hf,
					  unsigned int subclass)
{
	union usbg_f_hid_attr_val val = {.subclass = subclass};
	return usbg_f_hid_set_attr_val(hf, USBG_F_HID_SUBCLASS, &val);
}

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_MIDI__ */
