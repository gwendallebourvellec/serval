/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#include <serval/netdevice.h>
#include <serval/skbuff.h>
#include <serval/debug.h>
#include <netinet/serval.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/socket.h>
#if !defined(OS_ANDROID)
#include <net/ethernet.h> 
#endif
#include "packet.h"

extern int serval_ipv4_rcv(struct sk_buff *skb);

#define RCVLEN 1500 /* Should be more than enough for normal MTUs */
#define get_priv(dev) ((struct packet_raw_priv *)dev_get_priv(dev))

static int packet_raw_init(struct net_device *dev)
{
        struct sockaddr_in addr;
	int ret, val = 1;

	dev->fd = socket(AF_INET, SOCK_RAW, IPPROTO_SERVAL);

        if (dev->fd == -1) {
                LOG_ERR("packet socket: %s\n", strerror(errno));
                return -1;
        }

        /* Bind the raw IP socket to the device */
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
	dev_get_ipv4_addr(dev, &addr.sin_addr);
	addr.sin_port = 0;
	       
        ret = bind(dev->fd, (struct sockaddr *)&addr, sizeof(addr));

        if (ret == -1) {
                LOG_ERR("bind failure: %s\n",
                        strerror(errno));
                close(dev->fd);
		dev->fd = -1;
        }

	ret = setsockopt(dev->fd, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));

	if (ret == -1) {
                LOG_ERR("setsockopt failure: %s\n",
                        strerror(errno));
                close(dev->fd);
		dev->fd = -1;
	}

	ret = setsockopt(dev->fd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));

	if (ret == -1) {
                LOG_ERR("setsockopt failure: %s\n",
                        strerror(errno));
                close(dev->fd);
		dev->fd = -1;
	}

	return ret;
}

static void packet_raw_destroy(struct net_device *dev)
{
	if (dev->fd != -1) {
		close(dev->fd);
		dev->fd = -1;
	}
}

static int packet_raw_recv(struct net_device *dev)
{
	struct sk_buff *skb = NULL;
	struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
	int ret;

	skb = alloc_skb(RCVLEN);
        
	if (!skb) {
		LOG_ERR("could not allocate skb\n");
		return -1;
	}
        
	ret = recvfrom(dev->fd, skb->data, RCVLEN, 0, 
                       (struct sockaddr *)&addr, &addrlen);
	
	if (ret == -1) {
		LOG_ERR("recv: %s\n", 
			strerror(errno));
		free_skb(skb);
		return -1;
	} else if (ret == 0) {
		/* Should not happen */
                LOG_ERR("recv return 0\n");
		free_skb(skb);
		return -1;
	}
        

        /* LOG_DBG("Received %d bytes IP packet\n", ret); */

        skb_put(skb, ret);
	skb->dev = dev;
        /* Set network header offset */
	skb_reset_network_header(skb);
        /* skb->pkt_type = */
	skb->protocol = IPPROTO_IP;

	/* Packet should be freed by upper layers */
	return serval_ipv4_rcv(skb);
}

static int packet_raw_xmit(struct sk_buff *skb)
{
	struct iphdr *iph = ip_hdr(skb);
	struct sockaddr_in addr;
	int err;

	memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, &iph->daddr, sizeof(iph->daddr));

	if (!skb->dev) {
                LOG_ERR("No device set in skb\n");
		free_skb(skb);
		return -1;
	}

        LOG_DBG("sending message len=%u iph=%p skb->data=%p\n", 
                skb->len,
                iph,
                skb->data); 

	err = sendto(skb->dev->fd, skb->data, skb->len, 0, 
		     (struct sockaddr *)&addr, sizeof(addr));

	if (err == -1) {
		LOG_ERR("send error: %s\n", 
			strerror(errno));
                err = NET_XMIT_DROP;
	} else {
                err = NET_XMIT_SUCCESS;
        }

	free_skb(skb);

	return err;
}

static struct packet_ops pack_ops = {
	.init = packet_raw_init,
	.destroy = packet_raw_destroy,
	.recv = packet_raw_recv,
	.xmit = packet_raw_xmit
};

static void dev_setup(struct net_device *dev)
{
	dev->pack_ops = &pack_ops;
	ether_setup(dev);
}

int packet_init(void)
{
        int ret;

        ret = netdev_init();

        if (ret < 0) {
                return ret;
        }

        ret = netdev_populate_table(0, dev_setup);

        if (ret < 0)
                netdev_fini();

        return ret;
}

void packet_fini(void)
{
        netdev_fini();
}