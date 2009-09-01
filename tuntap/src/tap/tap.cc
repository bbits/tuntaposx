/*
 * ethertap device for macosx.
 *
 * tap_interface class definition
 */
/*
 * Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009 Mattias Nissler <mattias.nissler@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *      conditions and the following disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tap.h"

extern "C" {

#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/random.h>

#include <net/if_types.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/dlil.h>
#include <net/ethernet.h>

}

#if 0
#define dprintf(...)			log(LOG_INFO, __VA_ARGS__)
#else
#define dprintf(...)
#endif

static unsigned char ETHER_BROADCAST_ADDR[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/* members */
bool
tap_interface::initialize(unsigned short major, unsigned short unit)
{
	this->unit = unit;
	this->family_name = TAP_FAMILY_NAME;
	this->family = IFNET_FAMILY_ETHERNET;
	this->type = IFT_ETHER;
	bzero(unique_id, UIDLEN);
	snprintf(unique_id, UIDLEN, "%s%d", family_name, unit);

	dprintf("tap: starting interface %s%d\n", TAP_FAMILY_NAME, unit);

	/* register character device */
	if (!tuntap_interface::register_chardev(major))
		return false;

	return true;
}

void
tap_interface::shutdown()
{
	dprintf("tap: shutting down tap interface %s%d\n", TAP_FAMILY_NAME, unit);

	unregister_chardev();
}

int
tap_interface::initialize_interface()
{
	struct sockaddr_dl lladdr;
	lladdr.sdl_len = sizeof(lladdr);
	lladdr.sdl_family = AF_LINK;
	lladdr.sdl_alen = ETHER_ADDR_LEN;
	lladdr.sdl_nlen = lladdr.sdl_slen = 0;

	/* generate a random MAC address */
	read_random(LLADDR(&lladdr), ETHER_ADDR_LEN);

	/* clear multicast bit and set local assignment bit (see IEEE 802) */
	(LLADDR(&lladdr))[0] &= 0xfe;
	(LLADDR(&lladdr))[0] |= 0x02;

	dprintf("tap: random tap address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			(LLADDR(&lladdr))[0] & 0xff,
			(LLADDR(&lladdr))[1] & 0xff,
			(LLADDR(&lladdr))[2] & 0xff,
			(LLADDR(&lladdr))[3] & 0xff,
			(LLADDR(&lladdr))[4] & 0xff,
			(LLADDR(&lladdr))[5] & 0xff);

	/* register interface */
	if (!tuntap_interface::register_interface(&lladdr, ETHER_BROADCAST_ADDR, ETHER_ADDR_LEN))
		return EIO;

	/* Set link level address. Yes, we need to do that again. Darwin sucks. */
	errno_t err = ifnet_set_lladdr(ifp, LLADDR(&lladdr), ETHER_ADDR_LEN);
	if (err)
		dprintf("tap: failed to set lladdr on %s%d: %d\n", family_name, unit, err);

	/* set mtu */
	ifnet_set_mtu(ifp, TAP_MTU);
	/* set header length */
	ifnet_set_hdrlen(ifp, sizeof(struct ether_header));
	/* add the broadcast flag */
	ifnet_set_flags(ifp, IFF_BROADCAST, IFF_BROADCAST);

	/* we must call bpfattach(). Otherwise we deadlock BPF while unloading. Seems to be a bug in
	 * the kernel, see bpfdetach() in net/bpf.c, it will return without releasing the lock if
	 * the interface wasn't attached. I wonder what they were smoking while writing it ;-)
	 */
	bpfattach(ifp, DLT_EN10MB, ifnet_hdrlen(ifp));

	return 0;
}

void
tap_interface::shutdown_interface()
{
	dprintf("tap: shutting down network interface of device %s%d\n", TAP_FAMILY_NAME, unit);

	/* detach all protocols */
	for (unsigned int i = 0; i < MAX_ATTACHED_PROTOS; i++) {
		if (attached_protos[i].used) {
			errno_t err = ifnet_detach_protocol(ifp, attached_protos[i].proto);
			if (err)
				log(LOG_WARNING, "tap: could not detach protocol %d from %s%d\n",
						attached_protos[i].proto, TAP_FAMILY_NAME, unit);
		}
	}

	cleanup_interface();
	unregister_interface();
}

errno_t
tap_interface::if_ioctl(u_int32_t cmd, void *arg)
{
	dprintf("tap: if_ioctl cmd: %d (%x)\n", cmd & 0xff, cmd);

	switch (cmd) {
		case SIOCSIFLLADDR:
			{
				/* set ethernet address */
				struct sockaddr *ea = &(((struct ifreq *) arg)->ifr_addr);

				dprintf("tap: SIOCSIFLLADDR family %d len %d\n",
						ea->sa_family, ea->sa_len);

				/* check if it is really an ethernet address */
				if (ea->sa_family != AF_LINK || ea->sa_len != ETHER_ADDR_LEN)
					return EINVAL;

				/* ok, copy */

				errno_t err = ifnet_set_lladdr(ifp, ea->sa_data, ETHER_ADDR_LEN);
				if (err)
					dprintf("tap: failed to set lladdr on %s%d: %d\n",
							family_name, unit, err);

				return 0;
			}

		default:
			/* let our superclass handle it */
			return tuntap_interface::if_ioctl(cmd, arg);
	}
			
	return EOPNOTSUPP;
}

errno_t
tap_interface::if_demux(mbuf_t m, char *header, protocol_family_t *proto)
{
	struct ether_header *eh = (struct ether_header *) header;
	unsigned char lladdr[ETHER_ADDR_LEN];

	dprintf("tap: if_demux\n");

	/* size check */
	if (mbuf_len(m) < sizeof(struct ether_header))
		return ENOENT;

	/* catch broadcast and multicast (stolen from bsd/net/ether_if_module.c) */
	if (eh->ether_dhost[0] & 1) {
		if (memcmp(ETHER_BROADCAST_ADDR, eh->ether_dhost, ETHER_ADDR_LEN) == 0) {
			/* broadcast */
			dprintf("tap: broadcast packet.\n");
			mbuf_setflags_mask(m, MBUF_BCAST, MBUF_BCAST);
		} else {
			/* multicast */
			dprintf("tap: multicast packet.\n");
			mbuf_setflags_mask(m, MBUF_MCAST, MBUF_MCAST);
		}
	} else {
		/* check wether the packet has our address */
		ifnet_lladdr_copy_bytes(ifp, lladdr, ETHER_ADDR_LEN);
		if (memcmp(lladdr, eh->ether_dhost, ETHER_ADDR_LEN) != 0)
			mbuf_setflags_mask(m, MBUF_PROMISC, MBUF_PROMISC);
	}

	/* find the protocol */
	for (unsigned int i = 0; i < MAX_ATTACHED_PROTOS; i++) {
		if (attached_protos[i].used && attached_protos[i].type == eh->ether_type) {
			*proto = attached_protos[i].proto;
			return 0;
		}
	}

	dprintf("tap: if_demux() failed to find proto.\n");

	/* no matching proto found */
	return ENOENT;
}

errno_t
tap_interface::if_framer(mbuf_t *m, const struct sockaddr *dest, const char *dest_linkaddr,
		const char *frame_type)
{
	struct ether_header *eh;
	mbuf_t nm = *m;
	errno_t err;

	dprintf("tap: if_framer\n");

	/* prepend the ethernet header */
	err = mbuf_prepend(&nm, sizeof (struct ether_header), MBUF_WAITOK);
	if (err) {
		dprintf("tap: could not prepend data to mbuf: %d\n", err);
		return err;
	}
	*m = nm;

	/* fill the header */
	eh = (struct ether_header *) mbuf_data(*m);
	memcpy(eh->ether_dhost, dest_linkaddr, ETHER_ADDR_LEN);
	ifnet_lladdr_copy_bytes(ifp, eh->ether_shost, ETHER_ADDR_LEN);
	eh->ether_type = *((u_int16_t *) frame_type);

	return 0;
}

errno_t
tap_interface::if_add_proto(protocol_family_t proto, const struct ifnet_demux_desc *desc,
		u_int32_t ndesc)
{
	errno_t err;

	dprintf("tap: if_add_proto proto %d\n", proto);

	for (unsigned int i = 0; i < ndesc; i++) {
		/* try to add the protocol */
		err = add_one_proto(proto, desc[i]);
		if (err != 0) {
			/* if that fails, remove everything stored so far */
			if_del_proto(proto);
			return err;
		}
	}

	return 0;
}

errno_t
tap_interface::if_del_proto(protocol_family_t proto)
{
	dprintf("tap: if_del_proto proto %d\n", proto);

	/* delete all matching entries in attached_protos */
	for (unsigned int i = 0; i < MAX_ATTACHED_PROTOS; i++) {
		if (attached_protos[i].proto == proto)
			attached_protos[i].used = false;
	}

	return 0;
}

errno_t
tap_interface::if_check_multi(const struct sockaddr *maddr)
{
	dprintf("tap: if_check_multi family %d\n", maddr->sa_family);

	/* see whether it is a ethernet address with the multicast bit set */
	if (maddr->sa_family == AF_LINK) {
		struct sockaddr_dl *dlmaddr = (struct sockaddr_dl *) maddr;
		if (LLADDR(dlmaddr)[0] & 0x01)
			return 0;
		else
			return EADDRNOTAVAIL;
	}

	return EOPNOTSUPP;
}

errno_t
tap_interface::add_one_proto(protocol_family_t proto, const struct ifnet_demux_desc &dd)
{
	int free = -1;
	u_int16_t dt;

	/* we only support DLIL_DESC_ETYPE2 */
	if (dd.type != DLIL_DESC_ETYPE2 || dd.datalen != 2) {
		log(LOG_WARNING, "tap: tap only supports DLIL_DESC_ETYPE2 protocols.\n");
		return EINVAL;
	}

	dt = *((u_int16_t *) (dd.data));

	/* see if the protocol is already registered */
	for (unsigned int i = 0; i < MAX_ATTACHED_PROTOS; i++) {
		if (attached_protos[i].used) {
			if (dt == attached_protos[i].type) {
				/* already registered */
				if (attached_protos[i].proto == proto) {
					/* matches the old entry */
					return 0;
				} else
					return EEXIST;
			}
		} else if (free == -1)
			free = i;
	}

	/* did we find a free entry? */
	if (free == -1)
		/* is ENOBUFS correct? */
		return ENOBUFS;

	/* ok, save information */
	attached_protos[free].used = true;
	attached_protos[free].type = dt;
	attached_protos[free].proto = proto;

	return 0;
}

/* tap_manager members */
tuntap_interface *
tap_manager::create_interface()
{
	return new tap_interface();
}

