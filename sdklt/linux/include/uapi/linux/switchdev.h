/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generic netlink interface for switchdev FIB offload flag control.
 *
 * Copyright 2024 Broadcom. All rights reserved.
 */

#ifndef _UAPI_LINUX_SWITCHDEV_H
#define _UAPI_LINUX_SWITCHDEV_H

#include <linux/types.h>

#define SWITCHDEV_GENL_NAME    "switchdev"
#define SWITCHDEV_GENL_VERSION 1

enum switchdev_cmd {
	SWITCHDEV_CMD_UNSPEC,
	SWITCHDEV_CMD_FIB_OFFLOAD_SET,
	SWITCHDEV_CMD_FIB_OFFLOAD_GET,

	__SWITCHDEV_CMD_MAX,
};
#define SWITCHDEV_CMD_MAX (__SWITCHDEV_CMD_MAX - 1)

enum switchdev_attr {
	SWITCHDEV_ATTR_UNSPEC,
	SWITCHDEV_ATTR_FIB_FAMILY,         /* u8: AF_INET or AF_INET6 */
	SWITCHDEV_ATTR_FIB_ADDR,           /* binary: 4 or 16 byte address */
	SWITCHDEV_ATTR_FIB_PREFIXLEN,      /* u8: prefix length */
	SWITCHDEV_ATTR_FIB_TABLE_ID,       /* u32: routing table id (opt) */
	SWITCHDEV_ATTR_FIB_OFFLOAD,        /* u8: 0/1 (opt, default 1) */
	SWITCHDEV_ATTR_FIB_TRAP,           /* u8: 0/1 (opt, default 0) */
	SWITCHDEV_ATTR_FIB_OFFLOAD_FAILED, /* u8: 0/1 (opt, default 0) */

	__SWITCHDEV_ATTR_MAX,
};
#define SWITCHDEV_ATTR_MAX (__SWITCHDEV_ATTR_MAX - 1)

#endif /* _UAPI_LINUX_SWITCHDEV_H */
