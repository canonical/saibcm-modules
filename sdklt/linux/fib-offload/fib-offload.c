/*
 * Generic netlink FIB offload control module.
 *
 * Registers a generic netlink family "fib_offload" exposing the
 * FIB_OFFLOAD_CMD_SET command which allows userspace to look up a FIB
 * entry by prefix (IPv4 or IPv6) and set its hardware offload flags via
 * fib_alias_hw_flags_set() / fib6_info_hw_flags_set().
 *
 * Copyright 2024 Broadcom. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include <lkm/lkm.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/genetlink.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/rtnetlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <net/ip6_route.h>
#include <net/fib_rules.h>
#include <uapi/linux/fib-offload.h>

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("FIB offload generic netlink control");
MODULE_LICENSE("GPL");

static const struct nla_policy fib_offload_policy[FIB_OFFLOAD_ATTR_MAX + 1] = {
	[FIB_OFFLOAD_ATTR_FAMILY]         = { .type = NLA_U8 },
	[FIB_OFFLOAD_ATTR_ADDR]          = { .type = NLA_BINARY,
					    .len  = sizeof(struct in6_addr) },
	[FIB_OFFLOAD_ATTR_PREFIXLEN]      = { .type = NLA_U8 },
	[FIB_OFFLOAD_ATTR_TABLE_ID]       = { .type = NLA_U32 },
	[FIB_OFFLOAD_ATTR_OFFLOAD]        = { .type = NLA_U8 },
	[FIB_OFFLOAD_ATTR_TRAP]           = { .type = NLA_U8 },
	[FIB_OFFLOAD_ATTR_OFFLOAD_FAILED] = { .type = NLA_U8 },
};

static int
fib_offload_set_v4(struct net *net, const struct in_addr *addr, u8 prefixlen,
		   u8 offload, u8 trap, u8 offload_failed)
{
	struct flowi4 fl4 = { .daddr = addr->s_addr };
	struct fib_result res = {};
	struct fib_rt_info fri;
	struct fib_table *tb;
	int err;

	if (prefixlen > 32)
		return -EINVAL;

	rcu_read_lock();

	/*
	 * Access the main FIB table directly to avoid the non-exported
	 * fib_get_table(). This mirrors the CONFIG_IP_MULTIPLE_TABLES
	 * inline fib_lookup() but keeps res.fi valid under our RCU lock
	 * for fib_alias_hw_flags_set().
	 */
	tb = rcu_dereference_rtnl(net->ipv4.fib_main);
	if (!tb) {
		err = -ESRCH;
		goto unlock;
	}

	err = fib_table_lookup(tb, &fl4, &res, FIB_LOOKUP_NOREF);
	if (err)
		goto unlock;

	/* Only set flags on an exact prefix match. */
	if (res.prefixlen != prefixlen ||
	    res.prefix != addr->s_addr) {
		err = -ESRCH;
		goto unlock;
	}

	memset(&fri, 0, sizeof(fri));
	fri.fi            = res.fi;
	fri.tb_id         = res.table->tb_id;
	fri.dst           = res.prefix;
	fri.dst_len       = res.prefixlen;
	fri.dscp          = res.dscp;
	fri.type          = res.type;
	fri.offload       = offload ? 1 : 0;
	fri.trap          = trap ? 1 : 0;
	fri.offload_failed = offload_failed ? 1 : 0;

	/* fib_alias_hw_flags_set() takes its own rcu_read_lock(). */
	fib_alias_hw_flags_set(net, &fri);
	err = 0;

unlock:
	rcu_read_unlock();
	return err;
}

static int
fib_offload_set_v6(struct net *net, const struct in6_addr *addr, u8 prefixlen,
		    u8 offload, u8 trap, u8 offload_failed)
{
	struct rt6_info *rt;
	struct fib6_info *f6i;
	int err;

	if (prefixlen > 128)
		return -EINVAL;

	rt = rt6_lookup(net, addr, NULL, 0, NULL, 0);
	if (!rt)
		return -ESRCH;

	rcu_read_lock();
	f6i = rcu_dereference(rt->from);
	if (!f6i) {
		err = -ESRCH;
		goto unlock;
	}

	/* Only set flags on an exact prefix match. */
	if (f6i->fib6_dst.plen != prefixlen ||
	    !ipv6_addr_equal(&f6i->fib6_dst.addr, addr)) {
		err = -ESRCH;
		goto unlock;
	}

	/* fib6_info_hw_flags_set() uses WRITE_ONCE() and is safe under RCU. */
	fib6_info_hw_flags_set(net, f6i,
			      offload   ? true : false,
			      trap      ? true : false,
			      offload_failed ? true : false);
	err = 0;

unlock:
	rcu_read_unlock();
	dst_release(&rt->dst);
	return err;
}

static int
fib_offload_set(struct sk_buff *skb, struct genl_info *info)
{
	struct net *net = genl_info_net(info);
	u8 family, prefixlen;
	const void *addr;
	int addr_len;
	u8 offload = 1, trap = 0, offload_failed = 0;

	if (!info->attrs[FIB_OFFLOAD_ATTR_FAMILY] ||
	    !info->attrs[FIB_OFFLOAD_ATTR_ADDR] ||
	    !info->attrs[FIB_OFFLOAD_ATTR_PREFIXLEN])
		return -EINVAL;

	family = nla_get_u8(info->attrs[FIB_OFFLOAD_ATTR_FAMILY]);
	prefixlen = nla_get_u8(info->attrs[FIB_OFFLOAD_ATTR_PREFIXLEN]);
	addr = nla_data(info->attrs[FIB_OFFLOAD_ATTR_ADDR]);
	addr_len = nla_len(info->attrs[FIB_OFFLOAD_ATTR_ADDR]);

	if (info->attrs[FIB_OFFLOAD_ATTR_OFFLOAD])
		offload = nla_get_u8(info->attrs[FIB_OFFLOAD_ATTR_OFFLOAD]);
	if (info->attrs[FIB_OFFLOAD_ATTR_TRAP])
		trap = nla_get_u8(info->attrs[FIB_OFFLOAD_ATTR_TRAP]);
	if (info->attrs[FIB_OFFLOAD_ATTR_OFFLOAD_FAILED])
		offload_failed = nla_get_u8(
			info->attrs[FIB_OFFLOAD_ATTR_OFFLOAD_FAILED]);

	switch (family) {
	case AF_INET:
		if (addr_len != sizeof(struct in_addr))
			return -EINVAL;
		return fib_offload_set_v4(net, addr, prefixlen,
					  offload, trap, offload_failed);
	case AF_INET6:
		if (addr_len != sizeof(struct in6_addr))
			return -EINVAL;
		return fib_offload_set_v6(net, addr, prefixlen,
					  offload, trap, offload_failed);
	default:
		return -EAFNOSUPPORT;
	}
}

static const struct genl_small_ops fib_offload_ops[] = {
	{
		.cmd      = FIB_OFFLOAD_CMD_SET,
		.validate = GENL_DONT_VALIDATE_STRICT |
			    GENL_DONT_VALIDATE_DUMP,
		.doit     = fib_offload_set,
	},
};

static struct genl_family fib_offload_family = {
	.name       = FIB_OFFLOAD_GENL_NAME,
	.version    = FIB_OFFLOAD_GENL_VERSION,
	.maxattr    = FIB_OFFLOAD_ATTR_MAX,
	.netnsok    = true,
	.module     = THIS_MODULE,
	.policy     = fib_offload_policy,
	.small_ops  = fib_offload_ops,
	.n_small_ops = ARRAY_SIZE(fib_offload_ops),
};

static int __init
fib_offload_init(void)
{
	return genl_register_family(&fib_offload_family);
}

static void __exit
fib_offload_exit(void)
{
	genl_unregister_family(&fib_offload_family);
}

module_init(fib_offload_init);
module_exit(fib_offload_exit);
