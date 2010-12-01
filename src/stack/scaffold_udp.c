/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <scaffold/platform.h>
#include <scaffold/debug.h>
#include <scaffold/netdevice.h>
#include <scaffold/skbuff.h>
#include <scaffold_udp_sock.h>
#include <scaffold_sock.h>
#include <scaffold_ipv4.h>
#include <input.h>

#if defined(OS_LINUX_KERNEL)
#include <linux/ip.h>
#include <net/udp.h>
#endif

#if defined(OS_USER)
#include <netinet/ip.h>
#if defined(OS_BSD)
#include <scaffold/platform_tcpip.h>
#else
#include <netinet/udp.h>
#endif
#endif /* OS_USER */

/* For testing only. Should be removed */
extern const char *fixed_dev_name;
#define FIXED_DEV_NAME fixed_dev_name

static int scaffold_udp_init_sock(struct sock *sk)
{
        struct scaffold_udp_sock *usk = scaffold_udp_sk(sk);
        LOG_DBG("\n");
#if defined(OS_LINUX_KERNEL)
        usk->fake_dev = dev_get_by_name(&init_net, FIXED_DEV_NAME);      
#else
        usk->fake_dev = alloc_netdev(0, FIXED_DEV_NAME, ether_setup);
#endif
        if (!usk->fake_dev) {
                LOG_ERR("could not set fake device\n");
                return -ENOMEM;
        }

        return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
static int scaffold_udp_destroy_sock(struct sock *sk)
#else
static void scaffold_udp_destroy_sock(struct sock *sk)
#endif
{
        struct scaffold_udp_sock *usk = scaffold_udp_sk(sk);
        LOG_DBG("\n");

        if (usk->fake_dev)
                dev_put(usk->fake_dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
        return 0;
#endif
}

static void scaffold_udp_close(struct sock *sk, long timeout)
{
        //struct scaffold_udp_sock *usk = scaffold_udp_sk(sk);
        LOG_DBG("\n");
        sk_common_release(sk);
}

static int scaffold_udp_disconnect(struct sock *sk, int flags)
{
        LOG_DBG("\n");
        return 0;
}

static void scaffold_udp_shutdown(struct sock *sk, int how)
{
        LOG_DBG("\n");
}

static int scaffold_udp_backlog_rcv(struct sock *sk, struct sk_buff *skb)
{
        LOG_DBG("\n");

        FREE_SKB(skb);

        return -1;
}

/* 
   Receive from network
*/
int scaffold_udp_rcv(struct sk_buff *skb)
{
	struct sock *sk;
        struct udphdr *udph = udp_hdr(skb);
        struct sock_id *sockid = (struct sock_id *)&udph->dest;

        LOG_DBG("udp packet len=%u\n", ntohs(udph->len));
        
        sk = scaffold_sock_lookup_sockid(sockid);

        if (!sk) {
                LOG_ERR("No matching scaffold sock\n");
                return INPUT_NO_SOCK;
        }

        return INPUT_OK;
}

/* from fastudpsrc */
static void udp_checksum(uint16_t total_len,
                         struct udphdr *uh, uint32_t src) 
{
        unsigned short len = total_len - 14 - sizeof(struct iphdr);
        unsigned csum = 0; 
        uh->check = 0;
        /* FIXME: Do not assume IP header lacks options */
        csum = ~in_cksum((unsigned char *)uh, len) & 0xFFFF;
        csum += src & 0xffff;
        csum += (src >> 16) & 0xffff;
        csum += htons(SF_PROTO_UDP) + htons(len);
        csum = (csum & 0xFFFF) + (csum >> 16);
        uh->check = ~csum & 0xFFFF;
}

#define EXTRA_HDR (20)
#define UDP_MAX_HDR (MAX_HEADER + 20 + EXTRA_HDR) // payload + LL + IP + extra

static int scaffold_udp_transmit_skb(struct sock *sk, struct sk_buff *skb)        
{
        int err;
        unsigned short tot_len;
        struct udphdr *uh;

        /* Push back to make space for transport header */
        uh = (struct udphdr *)skb_push(skb, sizeof(struct udphdr));
	skb_reset_transport_header(skb);
	skb_set_owner_w(skb, sk);
        
        tot_len = skb->len + 20 + 14;
        
        /* Build UDP header */
        uh->source = scaffold_sk(sk)->local_srvid.s_sid16;
        uh->dest = scaffold_sk(sk)->peer_srvid.s_sid16;
        uh->len = htons(skb->len);
        udp_checksum(tot_len, uh, scaffold_sk(sk)->src_flow.fl_ip.s_addr);

        err = scaffold_ipv4_xmit_skb(sk, skb);
        
        LOG_DBG("udp pkt [s=%u d=%u len=%u]\n",
                ntohs(uh->source),
                ntohs(uh->dest),
                ntohs(uh->len));

        if (err < 0) {
                LOG_ERR("udp xmit failed\n");
        }

        return err;
}

static int scaffold_udp_sendmsg(struct kiocb *iocb, struct sock *sk, 
                                struct msghdr *msg, size_t len)
{
        int err;
        struct sk_buff *skb;
        int ulen = len;
        struct scaffold_sock *ssk = scaffold_sk(sk);

	if (len > 0xFFFF)
		return -EMSGSIZE;

	if (msg->msg_flags & MSG_OOB) /* Mirror BSD error message compatibility */
		return -EOPNOTSUPP;

        LOG_DBG("sending message\n");

	if (msg->msg_name) {
		struct sockaddr_sf *addr = (struct sockaddr_sf *)msg->msg_name;

		if ((unsigned)msg->msg_namelen < sizeof(*addr))
			return -EINVAL;
		if (addr->sf_family != AF_SCAFFOLD) {
			if (addr->sf_family != AF_UNSPEC)
				return -EAFNOSUPPORT;
		}
                lock_sock(sk);
                memcpy(&ssk->peer_srvid, &addr->sf_srvid, sizeof(struct service_id));
                release_sock(sk);
        } else {
                   if (sk->sk_state != SF_BOUND) {
                           return -EDESTADDRREQ;
                   }
        }

        ulen += sizeof(struct udphdr);

        skb = ALLOC_SKB(UDP_MAX_HDR + ulen, GFP_KERNEL);

        if (!skb)
                return -ENOMEM;
        
        skb_reserve(skb, UDP_MAX_HDR);

        skb->dev = scaffold_udp_sk(sk)->fake_dev;

        /* 
           TODO: 
           
           This is an extra copy operation for the user space version
           that we could try to get rid of, i.e., reading the data
           from the file descriptor directly into the socket buffer
        */
        
        err = memcpy_fromiovec(skb_put(skb, len), msg->msg_iov, len);
     
        if (err < 0) {
                LOG_ERR("could not copy user data to skb\n");
                FREE_SKB(skb);
                goto out;
        }

        skb_set_scaffold_packet_type(skb, PKT_TYPE_DATA);

        lock_sock(sk);

        err = scaffold_udp_transmit_skb(sk, skb);

        if (err < 0) {
                LOG_ERR("udp xmit failed\n");
                FREE_SKB(skb);
        }

        release_sock(sk);
out:
        return err;
}

static int scaffold_udp_recvmsg(struct kiocb *iocb, struct sock *sk, struct msghdr *msg,
                                size_t len, int nonblock, int flags, int *addr_len)
{
	struct scaffold_sock *ss = scaffold_sk(sk);
        struct sockaddr_sf *sfaddr = (struct sockaddr_sf *)msg->msg_name;
        int retval = -ENOMEM;
	long timeo;
        
        lock_sock(sk);
        
        if (sk->sk_state != SF_UNBOUND && 
            sk->sk_state != SF_BOUND && 
            sk->sk_state != SF_CLOSED) {
                /* SF_CLOSED is a valid state here because recvmsg
                 * should return 0 and not an error */
		retval = -ENOTCONN;
		goto out;
	}

        if ((unsigned)msg->msg_namelen < sizeof(struct sockaddr_sf)) {
                retval = -EINVAL;
                LOG_DBG("address length is incorrect\n");
                goto out;
        }

	timeo = sock_rcvtimeo(sk, nonblock);

	do {
		struct sk_buff *skb = skb_peek(&sk->sk_receive_queue);

		if (skb)
			goto found_ok_skb;
	
		if (sk->sk_state >= SF_CLOSED) {
                        /*
			if (!sock_flag(sk, SOCK_DONE)) {
				retval = -ENOTCONN;
				break;
			}
                        */
                        retval = 0;
			break;
		}
                
		if (!timeo) {
			retval = -EAGAIN;
			break;
		}

		if (signal_pending(current)) {
			retval = sock_intr_errno(timeo);
			break;
		}
                //LOG_DBG("waiting for data\n");

		sk_wait_data(sk, &timeo);
		continue;
	found_ok_skb:
		if (len >= skb->len) {
			retval = skb->len;
                        len = skb->len;
                } else if (len < skb->len) {
			msg->msg_flags |= MSG_TRUNC;
                        retval = len;
                }
                
                /* Copy service id */
                if (sfaddr) {
                        size_t addrlen = msg->msg_namelen;
                        unsigned short from = udp_hdr(skb)->source;

                        sfaddr->sf_family = AF_SCAFFOLD;
                        *addr_len = sizeof(struct sockaddr_sf);
                        memcpy(&sfaddr->sf_srvid, &from, sizeof(struct service_id));

                        /* Copy also our local service id to the
                         * address buffer if size admits */
                        if (addrlen >= sizeof(struct sockaddr_sf) * 2) {
                                sfaddr = (struct sockaddr_sf *)((char *)msg->msg_name + sizeof(struct sockaddr_sf));
                                sfaddr->sf_family = AF_SCAFFOLD;

                                memcpy(&sfaddr->sf_srvid, &ss->local_srvid, 
                                       sizeof(struct service_id));
                        }
                }
                
                //LOG_DBG("dequeing skb with length %u len=%zu retval=%d\n", skb->len, len, retval);

		if (skb_copy_datagram_iovec(skb, 0, msg->msg_iov, len)) {
			/* Exception. Bailout! */
			retval = -EFAULT;
                        LOG_DBG("could not copy data, len=%zu\n", len);
			break;
		}
		if (!(flags & MSG_PEEK))
			sk_eat_skb(sk, skb, 0);
		break;
	} while (1);
out:
        release_sock(sk);
        
        return retval;
}

struct proto scaffold_udp_proto = {
	.name			= "SCAFFOLD_UDP",
	.owner			= THIS_MODULE,
        .init                   = scaffold_udp_init_sock,
        .destroy                = scaffold_udp_destroy_sock,        
	.close  		= scaffold_udp_close,     
	.disconnect 		= scaffold_udp_disconnect,
	.shutdown		= scaffold_udp_shutdown,
        .sendmsg                = scaffold_udp_sendmsg,
        .recvmsg                = scaffold_udp_recvmsg,
	.backlog_rcv		= scaffold_udp_backlog_rcv,
        .hash                   = scaffold_sock_hash,
        .unhash                 = scaffold_sock_unhash,
	.obj_size		= sizeof(struct scaffold_udp_sock),
};