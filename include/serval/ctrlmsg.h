/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef _SERVAL_CTRLMSG_H
#define _SERVAL_CTRLMSG_H

#include <netinet/serval.h>
#if !defined(__KERNEL__)
#include <net/if.h>
#include <netinet/in.h>
#endif

#define CTRLMSG_ASSERT(predicate) _CASSERT(predicate, __LINE__)

#define _PASTE(a,b) a##b
#define _CASSERT(predicate,line)                                 \
        typedef char _PASTE(ctrlmsg_assertion_failed_,line)[2*!!(predicate)-1];

#define CTRLMSG_PACKED __attribute__((packed))

/*
  Control message types.

  NOTE: when adding a new type, also make sure the size array in
  ctrlmsg.c is updated accordingly.
*/
enum ctrlmsg_type {
        CTRLMSG_TYPE_REGISTER = 0,
        CTRLMSG_TYPE_UNREGISTER,
        CTRLMSG_TYPE_RESOLVE,
        CTRLMSG_TYPE_ADD_SERVICE,
        CTRLMSG_TYPE_DEL_SERVICE,
        CTRLMSG_TYPE_MOD_SERVICE,
        CTRLMSG_TYPE_GET_SERVICE,
        CTRLMSG_TYPE_SERVICE_STAT,
        CTRLMSG_TYPE_CAPABILITIES,
        CTRLMSG_TYPE_MIGRATE,
        CTRLMSG_TYPE_DUMMY,
        _CTRLMSG_TYPE_MAX,
};

struct service_info {
        uint16_t type; /* Type of service table entry? DMX, FWD, DLY, etc. */
        uint8_t  srvid_prefix_bits;
        uint8_t  srvid_flags;
        uint32_t if_index;
        uint32_t priority; /* Priority level of flow entry. */
        uint32_t weight;
        uint32_t idle_timeout; /* Idle time before discarding (seconds). */
        uint32_t hard_timeout; /* Max time before discarding (seconds). */
        struct in_addr address;
        struct service_id srvid;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct service_info) == 60)

struct service_info_stat {
        struct service_info service;
        uint32_t duration_sec;
        uint32_t duration_nsec;
        uint32_t packets_resolved;
        uint32_t bytes_resolved;
        uint32_t packets_dropped;
        uint32_t bytes_dropped;
        uint32_t tokens_consumed;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct service_info_stat) == 88)

struct service_stat {
        uint32_t capabilities;
        uint32_t services;
        uint32_t instances;
        uint32_t packets_resolved;
        uint32_t bytes_resolved;
        uint32_t bytes_dropped;
        uint32_t packets_dropped;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct service_stat) == 28)

enum sv_stack_capabilities {
        SVSTK_TRANSIT = 1 << 0, /* Can perform resolution/redireciton
                                 * - if not set, then the SR is
                                 * terminal for non-specified
                                 * prefixes*/

};

enum ctrlmsg_retval {
        CTRLMSG_RETVAL_OK = 0,
        CTRLMSG_RETVAL_ERROR,
        CTRLMSG_RETVAL_NOENTRY,
        CTRLMSG_RETVAL_MALFORMED,
};
 
struct ctrlmsg {
        uint8_t type;
        uint8_t retval;
        uint16_t len; /* Length, including header and payload */
        uint32_t xid; /* Transaction ID */
        unsigned char payload[0];
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg) == 8)

#define CTRLMSG_SIZE (sizeof(struct ctrlmsg))

struct ctrlmsg_register {
        struct ctrlmsg cmh;
        uint8_t flags;
        uint8_t pad;
        uint8_t srvid_prefix_bits;
        uint8_t srvid_flags;
        struct in_addr addr; /* When reregistering, this is the old address */
        struct service_id srvid;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg_register) == 48)

enum ctrlmsg_register_flags {
        REG_FLAG_REREGISTER = 1,
};

#define CTRLMSG_REGISTER_SIZE (sizeof(struct ctrlmsg_register))
#define CTRLMSG_UNREGISTER_SIZE (sizeof(struct ctrlmsg_register))

/* resolution up call for service router process to resolve
 * the response should be a ctrlmsg with a resolution and either
 * a buffer (skb) ID or the packet data
 */
struct ctrlmsg_resolve {
        struct ctrlmsg cmh;
        uint32_t xid;
        uint8_t src_flags;
        uint8_t src_prefix_bits;
        uint8_t dst_flags;
        uint8_t dst_prefix_bits;
        struct service_id src_srvid;
        struct service_id dst_srvid;
        struct in_addr src_address;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg_resolve) == 84)

#define CTRLMSG_RESOLVE_SIZE (sizeof(struct ctrlmsg_resolve))

/* resolution lookup for a service id (prefix), returns all
 * matching resolutions
 */
struct ctrlmsg_service {
        struct ctrlmsg cmh;
        uint32_t xid;
        struct service_info service[0];
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg_service) == 12)

#define CTRLMSG_GET_SERVICE_SIZE (sizeof(struct ctrlmsg_service))
#define CTRLMSG_ADD_SERVICE_SIZE (sizeof(struct ctrlmsg_service))
#define CTRLMSG_DEL_SERVICE_SIZE (sizeof(struct ctrlmsg_service))
#define CTRLMSG_MOD_SERVICE_SIZE (sizeof(struct ctrlmsg_service))

#define CTRLMSG_SERVICE_LEN(cmsg) \
        (cmsg)->cmh.len

#define CTRLMSG_SERVICE_NUM_LEN(num)                                    \
        (sizeof(struct ctrlmsg_service) +                               \
         (num * sizeof(struct service_info)))

#define CTRLMSG_SERVICE_NUM(cmsg)                                   \
        (((cmsg)->cmh.len - sizeof(struct ctrlmsg_service)) /       \
         sizeof(struct service_info))

struct ctrlmsg_service_info_stat {
        struct ctrlmsg cmh;
        uint32_t xid;
        struct service_info_stat service[0];
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg_service_info_stat) == 12)

#define CTRLMSG_SERVICE_INFO_STAT_LEN(cmsg)     \
        (cmsg)->cmh.len

#define CTRLMSG_SERVICE_INFO_STAT_NUM_LEN(num)                          \
        (sizeof(struct ctrlmsg_service_info_stat) +                     \
         (num * sizeof(struct service_info_stat)))

#define CTRLMSG_SERVICE_INFO_STAT_NUM(cmsg)                            \
        (((cmsg)->cmh.len - sizeof(struct ctrlmsg)) /                  \
         sizeof(struct service_info_stat))

struct ctrlmsg_service_stat {
        struct ctrlmsg cmh;
        uint32_t xid;
        struct service_stat stats;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg_service_stat) == 40)

#define CTRLMSG_SERVICE_STAT_SIZE (sizeof(struct ctrlmsg_service_stat))

#define CTRLMSG_SERVICE_STAT_LEN(cmsg)          \
        (cmsg)->cmh.len

#define CTRLMSG_SERVICE_STAT_NUM_LEN(num)                          \
        (sizeof(struct ctrlmsg_service_stat) +                     \
         (num * sizeof(struct service_stat)))

#define CTRLMSG_SERVICE_STAT_NUM(cmsg)                                 \
        (((cmsg)->cmh.len - sizeof(struct ctrlmsg)) /                  \
         sizeof(struct service_stat))

struct ctrlmsg_capabilities {
        struct ctrlmsg cmh;
        uint32_t capabilities;
} CTRLMSG_PACKED;

CTRLMSG_ASSERT(sizeof(struct ctrlmsg_capabilities) == 12)

#define CTRLMSG_CAPABILITIES_SIZE (sizeof(struct ctrlmsg_capabilities))

enum {
        CTRL_MIG_IFACE = 0,
        CTRL_MIG_FLOW,
        CTRL_MIG_SERVICE
};

struct ctrlmsg_migrate {
	struct ctrlmsg cmh;
	uint8_t migrate_type;
	union  {
	        char from_if[IFNAMSIZ];
	        struct flow_id from_flow;
	        struct service_id from_service;
	} from;
#define from_i from.from_if
#define from_f from.from_flow
#define from_s from.from_service
	char to_i[IFNAMSIZ];
} CTRLMSG_PACKED;

/* CTRLMSG_ASSERT(sizeof(struct ctrlmsg_migrate) == ) */

#define CTRLMSG_MIGRATE_SIZE (sizeof(struct ctrlmsg_migratee))

enum {
        CTRL_MODE_NET = 0, 
        CTRL_MODE_HOST = 1
};

#if defined(__linux__)
#include <linux/netlink.h>
#define NETLINK_SERVAL 17
#define NLMSG_SERVAL NLMSG_MIN_TYPE
#endif /* __linux__ */

#if defined(OS_ANDROID)
#define SERVAL_STACK_CTRL_PATH "/data/local/tmp/serval-stack-ctrl.sock"
#else
#define SERVAL_STACK_CTRL_PATH "/tmp/serval-stack-ctrl.sock"
#endif

#endif /* _SERVAL_CTRLMSG_H */
