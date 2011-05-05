/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
#ifndef _SERVAL_TCP_SOCK_H
#define _SERVAL_TCP_SOCK_H

#include <serval/platform.h>
#include <serval/skbuff.h>
#include <serval_sock.h>

struct tcp_congestion_ops;

struct serval_tcp_options_received {
/*	PAWS/RTTM data	*/
	long	ts_recent_stamp;/* Time we stored ts_recent (for aging) */
	u32	ts_recent;	/* Time stamp to echo next		*/
	u32	rcv_tsval;	/* Time stamp value             	*/
	u32	rcv_tsecr;	/* Time stamp echo reply        	*/
	u16 	saw_tstamp : 1,	/* Saw TIMESTAMP on last packet		*/
		tstamp_ok : 1,	/* TIMESTAMP seen on SYN packet		*/
		dsack : 1,	/* D-SACK is scheduled			*/
		wscale_ok : 1,	/* Wscale seen on SYN packet		*/
		sack_ok : 4,	/* SACK seen on SYN packet		*/
		snd_wscale : 4,	/* Window scaling received from sender	*/
		rcv_wscale : 4;	/* Window scaling to send to receiver	*/
	u8	cookie_plus:6,	/* bytes in authenticator/cookie option	*/
		cookie_out_never:1,
		cookie_in_always:1;
	u8	num_sacks;	/* Number of SACK blocks		*/
	u16	user_mss;	/* mss requested by user in ioctl	*/
	u16	mss_clamp;	/* Maximal mss, negotiated at connection setup */
};

/* The AF_SERVAL socket */
struct serval_tcp_sock {
	/* NOTE: serval_sock has to be the first member */
	struct serval_sock sk;
	u16	tcp_header_len;	/* Bytes of tcp header to send		*/
        struct sk_buff_head out_of_order_queue;

	__be32	pred_flags;
/*
 *	RFC793 variables by their proper names. This means you can
 *	read the code and the spec side by side (and laugh ...)
 *	See RFC793 and RFC1122. The RFC writes these in capitals.
 */
 	u32	rcv_nxt;	/* What we want to receive next 	*/
	u32	copied_seq;	/* Head of yet unread data		*/
	u32	rcv_wup;	/* rcv_nxt on last window update sent	*/
 	u32	snd_nxt;	/* Next sequence we send		*/

 	u32	snd_una;	/* First byte we want an ack for	*/
 	u32	snd_sml;	/* Last byte of the most recently transmitted small packet */
	u32	rcv_tstamp;	/* timestamp of last received ACK (for keepalives) */
	u32	lsndtime;	/* timestamp of last sent data packet (for restart window) */
        	/* Data for direct copy to user */
	struct {
		struct sk_buff_head	prequeue;
		struct task_struct	*task;
		struct iovec		*iov;
		int			memory;
		int			len;
#ifdef CONFIG_NET_DMA
		/* members for async copy */
		struct dma_chan		*dma_chan;
		int			wakeup;
		struct dma_pinned_list	*pinned_list;
		dma_cookie_t		dma_cookie;
#endif
	} ucopy;

	u32 snd_wl1; /* Sequence for window update		*/
	u32 snd_wnd; /* The window we expect to receive	*/
	u32 max_window; /* Maximal window ever seen from peer	*/
	u32 mss_cache; /* Cached effective mss, not including SACKS */
        
	u32 window_clamp;	/* Maximal window to advertise		*/
	u32 rcv_ssthresh;	/* Current window clamp			*/

	u32	frto_highmark;	/* snd_nxt when RTO occurred */
	u16	advmss;		/* Advertised MSS			*/
	u8	frto_counter;	/* Number of new acks after RTO */
	u8 nonagle     : 4,/* Disable Nagle algorithm?             */
		thin_lto    : 1,/* Use linear timeouts for thin streams */
		thin_dupack : 1,/* Fast retransmit on first dupack      */
		unused      : 2;
        /* RTT measurement */
	u32 srtt;		/* smoothed round trip time << 3	*/
	u32 mdev;		/* medium deviation			*/
	u32 mdev_max;	/* maximal mdev for the last rtt period	*/
	u32 rttvar;		/* smoothed mdev_max			*/
	u32 rtt_seq;	/* sequence number to update rttvar	*/

	u32 packets_out;	/* Packets which are "in flight"	*/
	u32 retrans_out;	/* Retransmitted packets out		*/

	u16 urg_data;	/* Saved octet of OOB data and control flags */
	u8 ecn_flags;	/* ECN status bits.			*/
	u8 reordering;	/* Packet reordering metric.		*/
	u32 snd_up;		/* Urgent pointer		*/
/*
 *      Options received (usually on last packet, some only on SYN packets).
 */
	struct serval_tcp_options_received rx_opt;
	const struct tcp_congestion_ops *ca_ops;
/*
 *	Slow start and congestion control (see also Nagle, and Karn & Partridge)
 */
 	u32     snd_ssthresh;	/* Slow start size threshold		*/
 	u32	snd_cwnd;	/* Sending congestion window		*/
	u32	snd_cwnd_cnt;	/* Linear increase counter		*/
	u32	snd_cwnd_clamp; /* Do not allow snd_cwnd to grow above this */
	u32	snd_cwnd_used;
	u32	snd_cwnd_stamp;

 	u32 rcv_wnd;  /* Current receiver window		*/
	u32 write_seq; /* Tail(+1) of data held in tcp send buffer */
	u32 pushed_seq; /* Last pushed seq, required to talk to windows */
        
	u32	lost_out;	/* Lost packets			*/
	u32	sacked_out;	/* SACK'd packets			*/
	u32	fackets_out;	/* FACK'd packets			*/
	u32	tso_deferred;
	u32	bytes_acked;	/* Appropriate Byte Counting - RFC3465 */


	u32	prior_ssthresh; /* ssthresh saved at recovery start	*/
	u32	high_seq;	/* snd_nxt at onset of congestion	*/
	u32	retrans_stamp;	/* Timestamp of the last retransmit,
				 * also used in SYN-SENT to remember stamp of
				 * the first SYN. */
	u32	undo_marker;	/* tracking retrans started here. */
	int	undo_retrans;	/* number of undoable retransmissions. */
	u32	total_retrans;	/* Total retransmits for entire connection */

	u32 urg_seq;	/* Seq of received urgent pointer */
        
 	struct timer_list	  retransmit_timer;
 	struct timer_list	  delack_timer;
	unsigned long		  timeout;

        /* Receiver side RTT estimation */
	struct {
		u32	rtt;
		u32	seq;
		u32	time;
	} rcv_rtt_est;

        /* Receiver queue space */
	struct {
		int	space;
		u32	seq;
		u32	time;
	} rcvq_space;

        /* TCP-specific MTU probe information. */
	struct {
		u32     probe_seq_start;
		u32     probe_seq_end;
	} mtu_probe;

        /* From inet_connection_sock */
        __u8			  ca_state;
	__u8			  retransmits;
	__u8			  pending;
	__u8			  backoff;
	__u8			  syn_retries;
	__u8			  probes_out;
	__u16			  ext_hdr_len;
	__u32                     rto;
	__u32			  pmtu_cookie;

	struct {
		__u8		  pending;	 /* ACK is pending			   */
		__u8		  quick;	 /* Scheduled number of quick acks	   */
		__u8		  pingpong;	 /* The session is interactive		   */
		__u8		  blocked;	 /* Delayed ACK was blocked by socket lock */
		__u32		  ato;		 /* Predicted tick of soft clock	   */
		unsigned long	  timeout;	 /* Currently scheduled timeout		   */
		__u32		  lrcvtime;	 /* timestamp of last received data packet */
		__u16		  last_seg_size; /* Size of last incoming segment	   */
		__u16		  rcv_mss;	 /* MSS used for delayed ACK decisions	   */ 
	} tp_ack;
	struct {
		int		  enabled;

		/* Range of MTUs to search */
		int		  search_high;
		int		  search_low;

		/* Information on the current probe. */
		int		  probe_size;
	} tp_mtup;

};

static inline struct serval_tcp_sock *serval_tcp_sk(const struct sock *sk)
{
	return (struct serval_tcp_sock *)sk;
}


/* urg_data states */
#define TCP_URG_VALID	0x0100
#define TCP_URG_NOTYET	0x0200
#define TCP_URG_READ	0x0400


#endif /* _SERVAL_TCP_SOCK_H */
