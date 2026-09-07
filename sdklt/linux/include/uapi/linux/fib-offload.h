/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generic netlink interface for FIB offload flag control.
 *
 * Copyright 2024 Broadcom. All rights reserved.
 */

#ifndef _UAPI_LINUX_FIB_OFFLOAD_H
#define _UAPI_LINUX_FIB_OFFLOAD_H

#include <linux/types.h>

#define FIB_OFFLOAD_GENL_NAME    "fib_offload"
#define FIB_OFFLOAD_GENL_VERSION 1

enum fib_offload_cmd {
	FIB_OFFLOAD_CMD_UNSPEC,
	FIB_OFFLOAD_CMD_SET,
	FIB_OFFLOAD_CMD_GET,

	__FIB_OFFLOAD_CMD_MAX,
};
#define FIB_OFFLOAD_CMD_MAX (__FIB_OFFLOAD_CMD_MAX - 1)

enum fib_offload_attr {
	FIB_OFFLOAD_ATTR_UNSPEC,
	FIB_OFFLOAD_ATTR_FAMILY,         /* u8: AF_INET or AF_INET6 */
	FIB_OFFLOAD_ATTR_ADDR,           /* binary: 4 or 16 byte address */
	FIB_OFFLOAD_ATTR_PREFIXLEN,      /* u8: prefix length */
	FIB_OFFLOAD_ATTR_TABLE_ID,       /* u32: routing table id (opt) */
	FIB_OFFLOAD_ATTR_OFFLOAD,         /* u8: 0/1 (opt, default 1) */
	FIB_OFFLOAD_ATTR_TRAP,           /* u8: 0/1 (opt, default 0) */
	FIB_OFFLOAD_ATTR_OFFLOAD_FAILED, /* u8: 0/1 (opt, default 0) */

	__FIB_OFFLOAD_ATTR_MAX,
};
#define FIB_OFFLOAD_ATTR_MAX (__FIB_OFFLOAD_ATTR_MAX - 1)

#endif /* _UAPI_LINUX_FIB_OFFLOAD_H */
