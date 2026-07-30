/*
 * drivers/staging/android/ion/compat_ion.c
 *
 * Copyright (C) 2013 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/compat.h>
#include <linux/fs.h>
#include <linux/overflow.h>
#include <linux/uaccess.h>

#include "ion.h"
#include "compat_ion.h"
#include "ion_legacy.h"

/* See drivers/staging/android/uapi/ion.h for the definition of these structs */
struct compat_ion_old_allocation_data {
	compat_size_t len;
	compat_size_t align;
	compat_uint_t heap_id_mask;
	compat_uint_t flags;
	compat_int_t handle;
};

struct compat_ion_handle_data {
	compat_int_t handle;
};

struct compat_ion_custom_data {
	compat_uint_t cmd;
	compat_ulong_t arg;
};

struct compat_ion_flush_data {
	compat_int_t handle;
	compat_int_t fd;
	compat_uptr_t vaddr;
	compat_uint_t offset;
	compat_uint_t length;
};

struct compat_ion_prefetch_regions {
	compat_uint_t vmid;
	compat_uptr_t sizes;
	compat_uint_t nr_sizes;
};

struct compat_ion_prefetch_data {
	compat_int_t heap_id;
	compat_ulong_t len;
	compat_uptr_t regions;
	compat_uint_t nr_regions;
};

#define COMPAT_ION_IOC_ALLOC	_IOWR(ION_IOC_MAGIC, 0, \
				      struct compat_ion_old_allocation_data)
#define COMPAT_ION_IOC_FREE	_IOWR(ION_IOC_MAGIC, 1, \
				      struct compat_ion_handle_data)
#define COMPAT_ION_IOC_CUSTOM	_IOWR(ION_IOC_MAGIC, 6, \
				      struct compat_ion_custom_data)
#define COMPAT_ION_IOC_CLEAN_CACHES _IOWR(ION_IOC_MSM_MAGIC, 0, \
					  struct compat_ion_flush_data)
#define COMPAT_ION_IOC_INV_CACHES _IOWR(ION_IOC_MSM_MAGIC, 1, \
					struct compat_ion_flush_data)
#define COMPAT_ION_IOC_CLEAN_INV_CACHES _IOWR(ION_IOC_MSM_MAGIC, 2, \
					      struct compat_ion_flush_data)
#define COMPAT_ION_IOC_PREFETCH	_IOWR(ION_IOC_MSM_MAGIC, 3, \
				      struct compat_ion_prefetch_data)
#define COMPAT_ION_IOC_DRAIN	_IOWR(ION_IOC_MSM_MAGIC, 4, \
				      struct compat_ion_prefetch_data)

#define COMPAT_ION_MAX_PREFETCH_REGIONS	0x10
#define COMPAT_ION_MAX_PREFETCH_SIZES	0x10

static int compat_get_ion_allocation_data(
			struct compat_ion_old_allocation_data __user *data32,
			struct ion_old_allocation_data __user *data)
{
	compat_size_t s;
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(s, &data32->len);
	err |= put_user(s, &data->len);
	err |= get_user(s, &data32->align);
	err |= put_user(s, &data->align);
	err |= get_user(u, &data32->heap_id_mask);
	err |= put_user(u, &data->heap_id_mask);
	err |= get_user(u, &data32->flags);
	err |= put_user(u, &data->flags);
	err |= get_user(i, &data32->handle);
	err |= put_user(i, &data->handle);

	return err;
}

static int compat_get_ion_handle_data(
			struct compat_ion_handle_data __user *data32,
			struct ion_handle_data __user *data)
{
	compat_int_t i;
	int err;

	err = get_user(i, &data32->handle);
	err |= put_user(i, &data->handle);

	return err;
}

static int compat_put_ion_allocation_data(
			struct compat_ion_old_allocation_data __user *data32,
			struct ion_old_allocation_data __user *data)
{
	compat_size_t s;
	compat_uint_t u;
	compat_int_t i;
	int err;

	err = get_user(s, &data->len);
	err |= put_user(s, &data32->len);
	err |= get_user(s, &data->align);
	err |= put_user(s, &data32->align);
	err |= get_user(u, &data->heap_id_mask);
	err |= put_user(u, &data32->heap_id_mask);
	err |= get_user(u, &data->flags);
	err |= put_user(u, &data32->flags);
	err |= get_user(i, &data->handle);
	err |= put_user(i, &data32->handle);

	return err;
}

static int compat_ion_legacy_cache_ioctl(unsigned int cmd,
					 unsigned long arg)
{
	struct compat_ion_flush_data flush;
	unsigned int native_cmd;

	switch (cmd) {
	case COMPAT_ION_IOC_CLEAN_CACHES:
		native_cmd = ION_IOC_CLEAN_CACHES;
		break;
	case COMPAT_ION_IOC_INV_CACHES:
		native_cmd = ION_IOC_INV_CACHES;
		break;
	case COMPAT_ION_IOC_CLEAN_INV_CACHES:
		native_cmd = ION_IOC_CLEAN_INV_CACHES;
		break;
	default:
		return -ENOIOCTLCMD;
	}

	if (copy_from_user(&flush, compat_ptr(arg), sizeof(flush)))
		return -EFAULT;

	return ion_legacy_cache_ioctl(flush.handle, flush.fd,
				      flush.offset, flush.length, native_cmd);
}

static long compat_ion_legacy_prefetch_ioctl(struct file *filp,
					     unsigned int cmd,
					     unsigned long arg)
{
	struct compat_ion_prefetch_data __user *data32 = compat_ptr(arg);
	struct compat_ion_prefetch_regions __user *regions32;
	struct ion_prefetch_data __user *data;
	struct ion_prefetch_regions __user *regions;
	compat_uptr_t region_sizes[COMPAT_ION_MAX_PREFETCH_REGIONS];
	compat_uint_t region_vmids[COMPAT_ION_MAX_PREFETCH_REGIONS];
	compat_uint_t region_nr_sizes[COMPAT_ION_MAX_PREFETCH_REGIONS];
	compat_uptr_t regions_ptr;
	compat_ulong_t len;
	compat_int_t heap_id;
	compat_uint_t nr_regions;
	__u64 __user *sizes;
	size_t alloc_size = sizeof(*data);
	size_t bytes;
	unsigned int native_cmd;
	unsigned int i;
	unsigned int j;
	int err;

	switch (cmd) {
	case COMPAT_ION_IOC_PREFETCH:
		native_cmd = ION_IOC_PREFETCH;
		break;
	case COMPAT_ION_IOC_DRAIN:
		native_cmd = ION_IOC_DRAIN;
		break;
	default:
		return -ENOIOCTLCMD;
	}

	err = get_user(heap_id, &data32->heap_id);
	err |= get_user(len, &data32->len);
	err |= get_user(regions_ptr, &data32->regions);
	err |= get_user(nr_regions, &data32->nr_regions);
	if (err)
		return -EFAULT;
	if (nr_regions > COMPAT_ION_MAX_PREFETCH_REGIONS)
		return -EINVAL;

	regions32 = compat_ptr(regions_ptr);
	if (check_mul_overflow((size_t)nr_regions, sizeof(*regions), &bytes) ||
	    check_add_overflow(alloc_size, bytes, &alloc_size))
		return -EOVERFLOW;

	for (i = 0; i < nr_regions; i++) {
		err = get_user(region_vmids[i], &regions32[i].vmid);
		err |= get_user(region_sizes[i], &regions32[i].sizes);
		err |= get_user(region_nr_sizes[i], &regions32[i].nr_sizes);
		if (err)
			return -EFAULT;
		if (region_nr_sizes[i] > COMPAT_ION_MAX_PREFETCH_SIZES)
			return -EINVAL;
		if (check_mul_overflow((size_t)region_nr_sizes[i],
				       sizeof(*sizes), &bytes) ||
		    check_add_overflow(alloc_size, bytes, &alloc_size))
			return -EOVERFLOW;
	}

	data = compat_alloc_user_space(alloc_size);
	if (!data)
		return -EFAULT;
	regions = (struct ion_prefetch_regions __user *)(data + 1);
	sizes = (__u64 __user *)(regions + nr_regions);

	err = put_user((__u64)len, &data->len);
	err |= put_user((__u64)(uintptr_t)regions, &data->regions);
	err |= put_user((__u32)heap_id, &data->heap_id);
	err |= put_user((__u32)nr_regions, &data->nr_regions);
	if (err)
		return -EFAULT;

	for (i = 0; i < nr_regions; i++) {
		compat_size_t __user *sizes32 =
			compat_ptr(region_sizes[i]);

		err = put_user((__u64)(uintptr_t)sizes, &regions[i].sizes);
		err |= put_user((__u32)region_vmids[i], &regions[i].vmid);
		err |= put_user((__u32)region_nr_sizes[i],
				&regions[i].nr_sizes);
		if (err)
			return -EFAULT;

		for (j = 0; j < region_nr_sizes[i]; j++) {
			compat_size_t size;

			if (get_user(size, &sizes32[j]) ||
			    put_user((__u64)size, sizes))
				return -EFAULT;
			sizes++;
		}
	}

	return filp->f_op->unlocked_ioctl(filp, native_cmd,
					  (unsigned long)data);
}

static long compat_ion_legacy_custom_ioctl(struct file *filp,
					   unsigned long arg)
{
	struct compat_ion_custom_data custom;

	if (copy_from_user(&custom, compat_ptr(arg), sizeof(custom)))
		return -EFAULT;

	switch (custom.cmd) {
	case COMPAT_ION_IOC_CLEAN_CACHES:
	case COMPAT_ION_IOC_INV_CACHES:
	case COMPAT_ION_IOC_CLEAN_INV_CACHES:
		return compat_ion_legacy_cache_ioctl(custom.cmd, custom.arg);
	case COMPAT_ION_IOC_PREFETCH:
	case COMPAT_ION_IOC_DRAIN:
		return compat_ion_legacy_prefetch_ioctl(filp, custom.cmd,
						       custom.arg);
	default:
		return -ENOIOCTLCMD;
	}
}

long compat_ion_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret;

	if (!filp->f_op->unlocked_ioctl)
		return -ENOTTY;

	switch (cmd) {
	case COMPAT_ION_IOC_ALLOC:
	{
		struct compat_ion_old_allocation_data __user *data32;
		struct ion_old_allocation_data __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (!data)
			return -EFAULT;

		err = compat_get_ion_allocation_data(data32, data);
		if (err)
			return err;
		ret = filp->f_op->unlocked_ioctl(filp, ION_OLD_IOC_ALLOC,
							(unsigned long)data);
		err = compat_put_ion_allocation_data(data32, data);
		return ret ? ret : err;
	}
	case COMPAT_ION_IOC_FREE:
	{
		struct compat_ion_handle_data __user *data32;
		struct ion_handle_data __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (!data)
			return -EFAULT;

		err = compat_get_ion_handle_data(data32, data);
		if (err)
			return err;

		return filp->f_op->unlocked_ioctl(filp, ION_IOC_FREE,
							(unsigned long)data);
	}
	case COMPAT_ION_IOC_CUSTOM:
		return compat_ion_legacy_custom_ioctl(filp, arg);
	case COMPAT_ION_IOC_CLEAN_CACHES:
	case COMPAT_ION_IOC_INV_CACHES:
	case COMPAT_ION_IOC_CLEAN_INV_CACHES:
		return compat_ion_legacy_cache_ioctl(cmd, arg);
	case COMPAT_ION_IOC_PREFETCH:
	case COMPAT_ION_IOC_DRAIN:
		return compat_ion_legacy_prefetch_ioctl(filp, cmd, arg);
	case ION_IOC_SHARE:
	case ION_IOC_MAP:
	case ION_IOC_IMPORT:
	case ION_IOC_SYNC:
	case ION_IOC_ALLOC:
	case ION_IOC_HEAP_QUERY:
	case ION_IOC_PREFETCH:
	case ION_IOC_DRAIN:
		return filp->f_op->unlocked_ioctl(filp, cmd,
						(unsigned long)compat_ptr(arg));
	default:
		return -ENOIOCTLCMD;
	}
}
