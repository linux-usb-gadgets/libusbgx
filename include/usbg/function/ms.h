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

#ifndef USBG_FUNCTION_MS__
#define USBG_FUNCTION_MS__

#include <usbg/usbg.h>

#include <malloc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usbg_f_ms usbg_f_ms;

struct usbg_f_ms_lun_attrs {
	int id;
	bool cdrom;
	bool ro;
	bool nofua;
	bool removable;
	const char *file;
};

struct usbg_f_ms_attrs {
	bool stall;
	int nluns;
	struct usbg_f_ms_lun_attrs **luns;
};

enum usbg_f_ms_lun_attr {
	USBG_F_MS_LUN_ATTR_MIN = 0,
	USBG_F_MS_LUN_CDROM = USBG_F_MS_LUN_ATTR_MIN,
	USBG_F_MS_LUN_RO,
	USBG_F_MS_LUN_NOFUA,
	USBG_F_MS_LUN_REMOVABLE,
	USBG_F_MS_LUN_FILE,
	USBG_F_MS_LUN_ATTR_MAX
};

union usbg_f_ms_lun_attr_val {
	bool cdrom;
	bool ro;
	bool nofua;
	bool removable;
	const char *file;
};

#define USBG_F_MS_LUN_BOOL_TO_ATTR_VAL(WHAT)		\
	USBG_TO_UNION(usbg_f_ms_lun_attr_val, cdrom, WHAT)

#define USBG_F_MS_LUN_CCHAR_PTR_TO_ATTR_VAL(WHAT)		\
	USBG_TO_UNION(usbg_f_ms_lun_attr_val, file, WHAT)



/**
 * @brief Cast from generic function to mass storage function
 * @param[in] f function to be converted to ms funciton.
 *         Should be one of types:
 *         ecm, subset, ncm, eem, rndis
 * @return Converted ms function or NULL if function hasn't suitable type
 */
usbg_f_ms *usbg_to_ms_function(usbg_function *f);

/**
 * @brief Cast form ms function to generic one
 * @param[in] mf function to be converted to generic one
 * @return Generic usbg function
 */
usbg_function *usbg_from_ms_function(usbg_f_ms *mf);

/**
 * @brief Create new LUN in choosen funcion
 * @note LUN ids should be contiguous (so id for new LUN should be
 *         current value nluns)
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun to be created
 * @param[in] lattrs LUN attributes to be set (may be NULL to use defaults)
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_create_lun(usbg_f_ms *mf, int lun_id,
			 struct usbg_f_ms_lun_attrs *lattrs);

/**
 * @brief Remove LUN from choosen funcion
 * @note LUN ids should be contiguous (so you should remove LUN with id equal
 *        to nluns - 1)
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun to be created
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_rm_lun(usbg_f_ms *mf, int lun_id);

/**
 * @brief Get attributes of choosen LUN
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] lattrs Structure to be filled with data
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_get_lun_attrs(usbg_f_ms *mf, int lun_id,
			struct usbg_f_ms_lun_attrs *lattrs);

/**
 * @brief Set attributes of choosen LUN
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] lattrs Lun attributes to be set
 * @return 0 on success usbg_error if error occurred.
 * @note The id value in lattrs is ignored by this function
 */
int usbg_f_ms_set_lun_attrs(usbg_f_ms *mf, int lun_id,
			    const struct usbg_f_ms_lun_attrs *lattrs);

/**
 * @brief Cleanup LUN attributes structure after usage
 * @param[in] attrs to be cleaned up
 */
static inline void usbg_f_ms_cleanup_lun_attrs(struct usbg_f_ms_lun_attrs *lattrs)
{
	if (lattrs) {
		free((char *)lattrs->file);
		lattrs->file = NULL;
	}
}

/**
 * @brief Get the value of single LUN attribute
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] lattr Code of attribute which value should be taken
 * @param[out] val Current value of this attribute
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_get_lun_attr_val(usbg_f_ms *mf, int lun_id,
			       enum usbg_f_ms_lun_attr lattr,
			       union usbg_f_ms_lun_attr_val *val);

/**
 * @brief Set the value of single LUN attribute
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] lattr Code of attribute which value should be set
 * @param[in] val Value of attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_set_lun_attr_val(usbg_f_ms *mf, int lun_id,
			       enum usbg_f_ms_lun_attr lattr,
			       const union usbg_f_ms_lun_attr_val *val);

/**
 * @brief Get the value which determines if lun is visible as a cdrom
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] cdrom Current value of stall attribute
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_get_lun_cdrom(usbg_f_ms *mf, int lun_id,
					  bool *cdrom)
{
	return usbg_f_ms_get_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_CDROM,
					  (union usbg_f_ms_lun_attr_val *)cdrom);
}

/**
 * @brief Set the value which determines if lun is visible as a cdrom
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] cdrom Value of stall attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_set_lun_cdrom(usbg_f_ms *mf, int lun_id,
					  bool cdrom)
{
	return usbg_f_ms_set_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_CDROM,
					  &USBG_F_MS_LUN_BOOL_TO_ATTR_VAL(cdrom));
}

/**
 * @brief Get the value which determines if lun is read only
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] ro Indicates if LUN is read only
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_get_lun_ro(usbg_f_ms *mf, int lun_id, bool *ro)
{
	return usbg_f_ms_get_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_RO,
					  (union usbg_f_ms_lun_attr_val *)ro);
}

/**
 * @brief Set the value which determines if lun read only
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] ro True if LUN should be read only
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_set_lun_ro(usbg_f_ms *mf, int lun_id, bool ro)
{
	return usbg_f_ms_set_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_RO,
					  &USBG_F_MS_LUN_BOOL_TO_ATTR_VAL(ro));
}

/**
 * @brief Get the value which determines if lun should ignore
 *        the FUA (Force Unit Access) flag in SCSI commands
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] nofua Indicates if FUA bit is ignored
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_get_lun_nofua(usbg_f_ms *mf, int lun_id,
					  bool *nofua)
{
	return usbg_f_ms_get_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_NOFUA,
					  (union usbg_f_ms_lun_attr_val *)nofua);
}

/**
 * @brief Set the value which determines if lun should ignore
 *        the FUA (Force Unit Access) flag in SCSI commands
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] nofua True if LUN should ignore FUA bit
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_set_lun_nofua(usbg_f_ms *mf, int lun_id,
				bool nofua)
{
	return usbg_f_ms_set_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_NOFUA,
					  &USBG_F_MS_LUN_BOOL_TO_ATTR_VAL(nofua));
}

/**
 * @brief Get the value which determines if lun is removable
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] removable Indicates if LUN is removable
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_get_lun_removable(usbg_f_ms *mf, int lun_id,
				bool *removable)
{
	return usbg_f_ms_get_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_REMOVABLE,
					  (union usbg_f_ms_lun_attr_val *)removable);
}

/**
 * @brief Set the value which determines if lun removable
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] removable True if LUN should be removable
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_set_lun_removable(usbg_f_ms *mf, int lun_id,
				bool removable)
{
	return usbg_f_ms_set_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_REMOVABLE,
					  &USBG_F_MS_LUN_BOOL_TO_ATTR_VAL(removable));
}

/**
 * @brief Get the name of file which is used as backend storage by this LUN
 *         into newly allocated storage
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] file Newly allocated string name of file used as backend
 *         storage
 * @return 0 on success usbg_error if error occurred.
 * @note returned file should be free() by caller
 */
static inline int usbg_f_ms_get_lun_file(usbg_f_ms *mf, int lun_id,
			       char **file)
{
	return usbg_f_ms_get_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_FILE,
					  (union usbg_f_ms_lun_attr_val *)file);
}

/**
 * @brief Get the name of file which is used as backend storage by this LUN
 *         into user buffer
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[out] buf Place where file should be stored
 * @param[in] len Size of buffer
 * @return Number of bytes placed into buf (excluding '\0') or the number of
 *         characters which would have been written to the buffer if enough
 *         space had been available. (just like snprintf() family). This may
 *         also return negative error code from usbg_error.
 */
int usbg_f_ms_get_lun_file_s(usbg_f_ms *mf, int lun_id,
			     char *buf, int len);

/**
 * @brief Set the name of file which is used as backend storage by this LUN
 * @param[in] mf Pointer to ms function
 * @param[in] lun_id ID of lun
 * @param[in] file Name of file which should be used as backend storage
 * @return 0 on success usbg_error if error occurred.
 */
static inline int usbg_f_ms_set_lun_file(usbg_f_ms *mf, int lun_id,
			   const char *file)
{
	return usbg_f_ms_set_lun_attr_val(mf, lun_id, USBG_F_MS_LUN_FILE,
					  &USBG_F_MS_LUN_CCHAR_PTR_TO_ATTR_VAL(file));
}

/**
 * @brief Get attributes of given ms function
 * @param[in] mf Pointer to ms function
 * @param[out] attrs Structure to be filled with data
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_get_attrs(usbg_f_ms *mf, struct usbg_f_ms_attrs *attrs);

/**
 * @brief Set attributes of given ms function
 * @param[in] mf Pointer to ms function
 * @param[in] attrs to be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_set_attrs(usbg_f_ms *mf, const struct usbg_f_ms_attrs *attrs);

/**
 * @brief Cleanup attributes structure after usage
 * @param[in] attrs to be cleaned up
 */
static inline void usbg_f_ms_cleanup_attrs(struct usbg_f_ms_attrs *attrs)
{
	int i;

	if (!attrs)
		return;

	for (i = 0; i < attrs->nluns; ++i) {
		if (!attrs->luns[i])
			continue;

		usbg_f_ms_cleanup_lun_attrs(attrs->luns[i]);
		free(attrs->luns[i]);
	}
	free(attrs->luns);
	attrs->luns = NULL;
}

/**
 * @brief Get the value of device side MAC address
 * @param[in] mf Pointer to ms function
 * @param[out] stall Current value of stall attribute
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_get_stall(usbg_f_ms *mf, bool *stall);

/**
 * @brief Set the value of device side MAC address
 * @param[in] mf Pointer to ms function
 * @param[in] stall Value of stall attribute which should be set
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_set_stall(usbg_f_ms *mf, bool stall);

/**
 * @brief Get the value of device side MAC address
 * @param[in] mf Pointer to ms function
 * @param[out] nluns Current number of LUNs
 * @return 0 on success usbg_error if error occurred.
 */
int usbg_f_ms_get_nluns(usbg_f_ms *mf, int *nluns);

#ifdef __cplusplus
}
#endif

#endif /* USBG_FUNCTION_MS__ */
