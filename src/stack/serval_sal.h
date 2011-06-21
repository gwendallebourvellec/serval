/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef _SERVAL_SAL_H_
#define _SERVAL_SAL_H_

#include <serval/skbuff.h>
#include <serval/sock.h>
#include <netinet/serval.h>

int serval_sal_xmit_skb(struct sk_buff *skb);

struct service_entry;

/* 
   WARNING:
   
   We must be careful that this struct does not overflow the 48 bytes
   that the skb struct gives us in the cb field.

   NOTE: Currently adds up to 44 bytes (non packed) on 64-bit platform.
   Should probably find another solution for storing a reference to
   the service id instead of a copy.
*/
struct serval_skb_cb {
        enum serval_packet_type pkttype;
	struct net_addr addr;
        uint32_t seqno;
        struct service_id srvid;
};

static inline struct serval_skb_cb *__serval_skb_cb(struct sk_buff *skb)
{
	struct serval_skb_cb * sscb = 
		(struct serval_skb_cb *)&(skb)->cb[0];
#if defined(ENABLE_DEBUG)
        if (sizeof(struct serval_skb_cb) > sizeof(skb->cb)) {
                LOG_WARN("serval_skb_cb (%zu bytes) > skb->cb (%zu bytes). "
                         "skb->cb may overflow!\n", 
                         sizeof(struct serval_skb_cb), 
                         sizeof(skb->cb));
        } /*
            else {
                LOG_WARN("serval_skb_cb (%zu bytes) skb->cb (%zu bytes).\n", 
                         sizeof(struct serval_skb_cb), 
                         sizeof(skb->cb));
                 } 
          */
#endif
	return sscb;
}

#define SERVAL_SKB_CB(__skb) __serval_skb_cb(__skb)

#define MAX_CTRL_QUEUE_LEN 20

/* control queue abstraction */
static inline void serval_sal_ctrl_queue_purge(struct sock *sk)
{
	struct sk_buff *skb;

	while ((skb = __skb_dequeue(&serval_sk(sk)->ctrl_queue)) != NULL) {
		FREE_SKB(skb);
	}
	/* serval_sal_clear_all_retrans_hints(serval_sal_sk(sk)); */
}

static inline struct sk_buff *serval_sal_ctrl_queue_head(struct sock *sk)
{
	return skb_peek(&serval_sk(sk)->ctrl_queue);
}

static inline struct sk_buff *serval_sal_ctrl_queue_tail(struct sock *sk)
{
	return skb_peek_tail(&serval_sk(sk)->ctrl_queue);
}

static inline struct sk_buff *serval_sal_ctrl_queue_next(struct sock *sk, 
							 struct sk_buff *skb)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
	return skb_queue_next(&serval_sk(sk)->ctrl_queue, skb);
#else
        return skb->next;
#endif
}

static inline struct sk_buff *serval_sal_ctrl_queue_prev(struct sock *sk, 
							 struct sk_buff *skb)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
	return skb_queue_prev(&serval_sk(sk)->ctrl_queue, skb);
#else
        return skb->prev;
#endif
}

#define serval_sal_for_ctrl_queue(skb, sk)	\
	skb_queue_walk(&(sk)->ctrl_queue, skb)

#define serval_sal_for_ctrl_queue_from(skb, sk)		\
	skb_queue_walk_from(&(sk)->ctrl_queue, skb)

#define serval_sal_for_ctrl_queue_from_safe(skb, tmp, sk)	\
	skb_queue_walk_from_safe(&(sk)->ctrl_queue, skb, tmp)

static inline struct sk_buff *serval_sal_send_head(struct sock *sk)
{
	return serval_sk(sk)->ctrl_send_head;
}

static inline int serval_sal_skb_is_last(const struct sock *sk,
					 const struct sk_buff *skb)
{

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28))
	return skb_queue_is_last(&serval_sk(sk)->ctrl_queue, skb);
#else
        return (skb->next == (struct sk_buff *)&serval_sk(sk)->ctrl_queue);
#endif
}

static inline void serval_sal_advance_send_head(struct sock *sk, 
						struct sk_buff *skb)
{
	if (serval_sal_skb_is_last(sk, skb))
		serval_sk(sk)->ctrl_send_head = NULL;
	else
		serval_sk(sk)->ctrl_send_head = 
			serval_sal_ctrl_queue_next(sk, skb);
}

static inline void serval_sal_check_send_head(struct sock *sk, 
					      struct sk_buff *skb_unlinked)
{
	if (serval_sk(sk)->ctrl_send_head == skb_unlinked)
		serval_sk(sk)->ctrl_send_head = NULL;
}

static inline void serval_sal_init_send_head(struct sock *sk)
{
	serval_sk(sk)->ctrl_send_head = NULL;
}

static inline void serval_sal_init_ctrl_queue(struct sock *sk)
{
        skb_queue_head_init(&serval_sk(sk)->ctrl_queue);
        serval_sal_init_send_head(sk);
}

static inline void __serval_sal_add_ctrl_queue_tail(struct sock *sk, 
						    struct sk_buff *skb)
{
	__skb_queue_tail(&serval_sk(sk)->ctrl_queue, skb);
}

static inline void serval_sal_add_ctrl_queue_tail(struct sock *sk, 
						  struct sk_buff *skb)
{
	__serval_sal_add_ctrl_queue_tail(sk, skb);

	/* Queue it, remembering where we must start sending. */
	if (serval_sk(sk)->ctrl_send_head == NULL) {
		serval_sk(sk)->ctrl_send_head = skb;
		/*
		if (serval_sal_sk(sk)->highest_sack == NULL)
			serval_sal_sk(sk)->highest_sack = skb;
		*/
	}
}

static inline void __serval_sal_add_ctrl_queue_head(struct sock *sk, 
						    struct sk_buff *skb)
{
	__skb_queue_head(&serval_sk(sk)->ctrl_queue, skb);
}

/* Insert buff after skb on the control queue of serval sk.  */
static inline void serval_sal_insert_ctrl_queue_after(struct sk_buff *skb,
						      struct sk_buff *buff,
						      struct sock *sk)
{
	__skb_queue_after(&serval_sk(sk)->ctrl_queue, skb, buff);
}

/* Insert new before skb on the control queue of serval_sk.  */
static inline void serval_sal_insert_ctrl_queue_before(struct sk_buff *new,
						       struct sk_buff *skb,
						       struct sock *sk)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	__skb_queue_before(&serval_sk(sk)->ctrl_queue, skb, new);
#else
        __skb_insert(new, skb->prev, skb, &serval_sk(sk)->ctrl_queue);
#endif
	if (serval_sk(sk)->ctrl_send_head == skb)
		serval_sk(sk)->ctrl_send_head = new;
}

static inline void serval_sal_unlink_ctrl_queue(struct sk_buff *skb, 
						struct sock *sk)
{
	__skb_unlink(skb, &serval_sk(sk)->ctrl_queue);
}

static inline int serval_sal_ctrl_queue_empty(struct sock *sk)
{
	return skb_queue_empty(&serval_sk(sk)->ctrl_queue);
}

static inline unsigned int serval_sal_ctrl_queue_len(struct sock *sk)
{
        return skb_queue_len(&serval_sk(sk)->ctrl_queue);
}

int serval_sal_connect(struct sock *sk, struct sockaddr *uaddr, int addr_len);
void serval_sal_close(struct sock *sk, long timeout);
int serval_sal_do_rcv(struct sock *sk, struct sk_buff *skb);
void serval_sal_rexmit_timeout(unsigned long data);
void serval_sal_timewait_timeout(unsigned long data);

#endif /* _SERVAL_SAL_H_ */