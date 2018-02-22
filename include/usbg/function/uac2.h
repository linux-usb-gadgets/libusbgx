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

#ifndef USBG_FUNCTION_UAC2__
#define USBG_FUNCTION_UAC2__

#include <usbg/usbg.h>

#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usbg_f_uac2;
typedef struct usbg_f_uac2 usbg_f_uac2;

struct usbg_f_uac2_attrs {
	int c_chmask;
	int c_srate;
	int c_ssize;
	int p_chmask;
	int p_srate;
	int p_ssize;
};

enum usbg_f_uac2_attr {
	USBG_F_UAC2_ATTR_MIN = 0,
	USBG_F_UAC2_C_CHMASK = USBG_F_UAC2_ATTR_MIN,
	USBG_F_UAC2_C_SRATE,
	USBG_F_UAC2_C_SSIZE,
	USBG_F_UAC2_P_CHMASK,
	USBG_F_UAC2_P_SRATE,
	USBG_F_UAC2_P_SSIZE,
	USBG_F_UAC2_ATTR_MAX
};

union usbg_f_uac2_attr_val {
	int c_chmask;
	int c_srate;
	int c_ssize;
	int p_chmask;
	int p_srate;
	int p_ssize;
};

#define USBG_F_UAC2_INT_TO_ATTR_VAL(WHAT)			\
	USBG_TO_UNION(usbg_f_uac2_attr_val, c_chmask, WHAT)

/**
 * @brief Cast from generic function to uac2 function
 * @param[in] f function to be converted to uac2 funciton.
 *         Function should be one of type uac2.
 * @return Converted uac2 function or NULL if function hasn't suitable type
 */
usbg_f_uac2 *usbg_to_uac2_function(usbg_function *f);

/**
 * @brief Cast form uac2 function to generic one
 * @param[in] af function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_uac2_function(usbg_f_uac2 *af);

/**
 * @brief Get attributes of given uac2 function
 * @param[in] af Pointer to uac2 function
 * @param[out] attrs Structure to be filled with data
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_uac2_get_attrs(usbg_f_uac2 *af,
			  struct usbg_f_uac2_attrs *attrs);

/**
 * @brief Set attributes of given uac2 function
 * @param[in] af Pointer to uac2 function
 * @param[in] attrs to be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_uac2_set_attrs(usbg_f_uac2 *af,
			 const struct usbg_f_uac2_attrs *attrs);

/**
 * @brief Cleanup attributes structure after usage
 * @param[in] attrs to be cleaned up
 */
static inline void usbg_f_uac2_cleanup_attrs(struct usbg_f_uac2_attrs *attrs)
{
}

/**
 * @brief Get the value of single attribute
 * @param[in] af Pointer to uac2 function
 * @param[in] attr Code of attribute which value should be taken
 * @param[out] val Current value of this attribute
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_uac2_get_attr_val(usbg_f_uac2 *af, enum usbg_f_uac2_attr attr,
			    union usbg_f_uac2_attr_val *val);

/**
 * @brief Set the value of single attribute
 * @param[in] af Pointer to uac2 function
 * @param[in] attr Code of attribute which value should be set
 * @param[in] val Value of attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_uac2_set_attr_val(usbg_f_uac2 *af, enum usbg_f_uac2_attr attr,
			     union usbg_f_uac2_attr_val val);

/**
 * @brief Get the capture channel mask of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[out] index Current capture channel mask of UAC2 adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_get_c_chmask(usbg_f_uac2 *af, int *c_chmask)
{
	return usbg_f_uac2_get_attr_val(af, USBG_F_UAC2_C_CHMASK,
					(union usbg_f_uac2_attr_val *)c_chmask);
}

/**
 * @brief Set the capture channel mask of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[in] capture channel mask which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_set_c_chmask(usbg_f_uac2 *af, int c_chmask)
{
	return usbg_f_uac2_set_attr_val(af, USBG_F_UAC2_C_CHMASK,
					USBG_F_UAC2_INT_TO_ATTR_VAL(c_chmask));
}

/**
 * @brief Get the capture sample rate of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[out] index Current sample rate mask of UAC2 adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_get_c_srate(usbg_f_uac2 *af, int *c_srate)
{
	return usbg_f_uac2_get_attr_val(af, USBG_F_UAC2_C_SRATE,
					(union usbg_f_uac2_attr_val *)c_srate);
}

/**
 * @brief Set the capture sample rate of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[in] capture sample rate which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_set_c_srate(usbg_f_uac2 *af, int c_srate)
{
	return usbg_f_uac2_set_attr_val(af, USBG_F_UAC2_C_SRATE,
					USBG_F_UAC2_INT_TO_ATTR_VAL(c_srate));
}

/**
 * @brief Get the capture sample size of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[out] index Current sample size mask of UAC2 adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_get_c_ssize(usbg_f_uac2 *af, int *c_ssize)
{
	return usbg_f_uac2_get_attr_val(af, USBG_F_UAC2_C_SSIZE,
					(union usbg_f_uac2_attr_val *)c_ssize);
}

/**
 * @brief Set the capture sample size of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[in] capture sample size which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_set_c_ssize(usbg_f_uac2 *af, int c_ssize)
{
	return usbg_f_uac2_set_attr_val(af, USBG_F_UAC2_C_SSIZE,
					USBG_F_UAC2_INT_TO_ATTR_VAL(c_ssize));
}

/**
 * @brief Get the playback channel mask of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[out] index Current playback channel mask of UAC2 adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_get_p_chmask(usbg_f_uac2 *af, int *p_chmask)
{
	return usbg_f_uac2_get_attr_val(af, USBG_F_UAC2_P_CHMASK,
					(union usbg_f_uac2_attr_val *)p_chmask);
}

/**
 * @brief Set the playback channel mask of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[in] playback channel mask which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_set_p_chmask(usbg_f_uac2 *af, int p_chmask)
{
	return usbg_f_uac2_set_attr_val(af, USBG_F_UAC2_P_CHMASK,
					USBG_F_UAC2_INT_TO_ATTR_VAL(p_chmask));
}

/**
 * @brief Get the playback sample rate of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[out] index Current sample rate mask of UAC2 adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_get_p_srate(usbg_f_uac2 *af, int *p_srate)
{
	return usbg_f_uac2_get_attr_val(af, USBG_F_UAC2_P_SRATE,
					(union usbg_f_uac2_attr_val *)p_srate);
}

/**
 * @brief Set the playback sample rate of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[in] playback sample rate which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_set_p_srate(usbg_f_uac2 *af, int p_srate)
{
	return usbg_f_uac2_set_attr_val(af, USBG_F_UAC2_P_SRATE,
					USBG_F_UAC2_INT_TO_ATTR_VAL(p_srate));
}

/**
 * @brief Get the playback sample size of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[out] index Current sample size mask of UAC2 adapter
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_get_p_ssize(usbg_f_uac2 *af, int *p_ssize)
{
	return usbg_f_uac2_get_attr_val(af, USBG_F_UAC2_P_SSIZE,
					(union usbg_f_uac2_attr_val *)p_ssize);
}

/**
 * @brief Set the playback sample size of UAC2 adapter
 * @param[in] af Pointer to uac2 function
 * @param[in] playback sample size which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_uac2_set_p_ssize(usbg_f_uac2 *af, int p_ssize)
{
	return usbg_f_uac2_set_attr_val(af, USBG_F_UAC2_P_SSIZE,
					USBG_F_UAC2_INT_TO_ATTR_VAL(p_ssize));
}

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_UAC2__ */
