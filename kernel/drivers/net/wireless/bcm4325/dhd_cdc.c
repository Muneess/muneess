/*
 * DHD Protocol Module for CDC and BDC.
 *
 * Copyright (C) 1999-2009, Broadcom Corporation
 * 
 *         Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_cdc.c,v 1.22.4.2.4.8.2.29 2009/10/05 05:54:04 Exp $
 *
 * BDC is like CDC, except it includes a header for data packets to convey
 * packet priority over the bus, and flags (e.g. to indicate checksum status
 * for dongle offload).
 */

#include <typedefs.h>
#include <osl.h>

#include <bcmutils.h>
#include <bcmcdc.h>
#include <bcmendian.h>

#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_proto.h>
#include <dhd_bus.h>
#include <dhd_dbg.h>

#include <mach/austin_hwid.h>

#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i

#define BTCOEXIST_SETTING	1

#ifdef CONFIG_WLAN_ALLOC_STATIC_MEM
    #define DHD_PORT_LEN		SZ_16K
    extern void *addr_4325_dhd_prot;
#endif

/* Packet alignment for most efficient SDIO (can change based on platform) */
#ifndef DHD_SDALIGN
#define DHD_SDALIGN	32
#endif
#if !ISPOWEROF2(DHD_SDALIGN)
#error DHD_SDALIGN is not a power of 2!
#endif

#define RETRIES 2		/* # of retries to retrieve matching ioctl response */
#define BUS_HEADER_LEN	(16+DHD_SDALIGN)	/* Must be atleast SDPCM_RESERVE
				 * defined in dhd_sdio.c (amount of header tha might be added)
				 * plus any space that might be needed for alignment padding.
				 */
#define ROUND_UP_MARGIN	2048 	/* Biggest SDIO block size possible for
				 * round off at the end of buffer
				 */

extern int q_wlan_flag;
extern void msm_get_wlan_mac_addr(uint8_t *mac_addr, uint8_t length);

typedef struct dhd_prot {
	uint16 reqid;
	uint8 pending;
	uint32 lastcmd;
	uint8 bus_header[BUS_HEADER_LEN];
	cdc_ioctl_t msg;
	unsigned char buf[WLC_IOCTL_MAXLEN + ROUND_UP_MARGIN];
} dhd_prot_t;

static int
dhdcdc_msg(dhd_pub_t *dhd)
{
	dhd_prot_t *prot = dhd->prot;
	int len = ltoh32(prot->msg.len) + sizeof(cdc_ioctl_t);

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	/* NOTE : cdc->msg.len holds the desired length of the buffer to be
	 *        returned. Only up to CDC_MAX_MSG_SIZE of this buffer area
	 *	  is actually sent to the dongle
	 */
	if (len > CDC_MAX_MSG_SIZE)
		len = CDC_MAX_MSG_SIZE;

	/* Send request */
	return dhd_bus_txctl(dhd->bus, (uchar*)&prot->msg, len);
}

static int
dhdcdc_cmplt(dhd_pub_t *dhd, uint32 id, uint32 len)
{
	int ret;
	dhd_prot_t *prot = dhd->prot;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	do {
		ret = dhd_bus_rxctl(dhd->bus, (uchar*)&prot->msg, len+sizeof(cdc_ioctl_t));
		if (ret < 0)
			break;
	} while (CDC_IOC_ID(ltoh32(prot->msg.flags)) != id);

	return ret;
}

int
dhdcdc_query_ioctl(dhd_pub_t *dhd, int ifidx, uint cmd, void *buf, uint len)
{
	dhd_prot_t *prot = dhd->prot;
	cdc_ioctl_t *msg = &prot->msg;
	void *info;
	int ret = 0, retries = 0;
	uint32 id, flags = 0;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));
	DHD_CTL(("%s: cmd %d len %d\n", __FUNCTION__, cmd, len));


	/* Respond "bcmerror" and "bcmerrorstr" with local cache */
	if (cmd == WLC_GET_VAR)
	{
		if (!strcmp((char *)buf, "bcmerrorstr"))
		{
			strncpy((char *)buf, bcmerrorstr(dhd->dongle_error), BCME_STRLEN);
			goto done;
		}
		else if (!strcmp((char *)buf, "bcmerror"))
		{
			*(int *)buf = dhd->dongle_error;
			goto done;
		}
	}

	memset(msg, 0, sizeof(cdc_ioctl_t));

	msg->cmd = htol32(cmd);
	msg->len = htol32(len);
	flags = (++prot->reqid << CDCF_IOC_ID_SHIFT);
	msg->flags = htol32(flags);
	CDC_SET_IF_IDX(msg, ifidx);

	if (buf)
		memcpy(prot->buf, buf, len);

	if ((ret = dhdcdc_msg(dhd)) < 0) {
		DHD_ERROR(("dhdcdc_query_ioctl: dhdcdc_msg failed w/status %d\n", ret));
		goto done;
	}

retry:
	/* wait for interrupt and get first fragment */
	if ((ret = dhdcdc_cmplt(dhd, prot->reqid, len)) < 0)
		goto done;

	flags = ltoh32(msg->flags);
	id = (flags & CDCF_IOC_ID_MASK) >> CDCF_IOC_ID_SHIFT;

	if ((id < prot->reqid) && (++retries < RETRIES))
		goto retry;
	if (id != prot->reqid) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, ifidx), __FUNCTION__, id, prot->reqid));
		ret = -EINVAL;
		goto done;
	}

	/* Check info buffer */
	info = (void*)&msg[1];

	/* Copy info buffer */
	if (buf)
	{
		if (ret < (int)len)
			len = ret;
		memcpy(buf, info, len);
	}

	/* Check the ERROR flag */
	if (flags & CDCF_IOC_ERROR)
	{
		ret = ltoh32(msg->status);
		/* Cache error from dongle */
		dhd->dongle_error = ret;
	}

done:
	return ret;
}

static int
dhdcdc_set_ioctl(dhd_pub_t *dhd, int ifidx, uint cmd, void *buf, uint len)
{
	dhd_prot_t *prot = dhd->prot;
	cdc_ioctl_t *msg = &prot->msg;
	int ret = 0;
	uint32 flags, id;

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));
	DHD_CTL(("%s: cmd %d len %d\n", __FUNCTION__, cmd, len));

	memset(msg, 0, sizeof(cdc_ioctl_t));

	msg->cmd = htol32(cmd);
	msg->len = htol32(len);
	flags = (++prot->reqid << CDCF_IOC_ID_SHIFT) | CDCF_IOC_SET;
	msg->flags |= htol32(flags);
	CDC_SET_IF_IDX(msg, ifidx);

	if (buf)
		memcpy(prot->buf, buf, len);

	if ((ret = dhdcdc_msg(dhd)) < 0)
		goto done;

	if ((ret = dhdcdc_cmplt(dhd, prot->reqid, len)) < 0)
		goto done;

	flags = ltoh32(msg->flags);
	id = (flags & CDCF_IOC_ID_MASK) >> CDCF_IOC_ID_SHIFT;

	if (id != prot->reqid) {
		DHD_ERROR(("%s: %s: unexpected request id %d (expected %d)\n",
		           dhd_ifname(dhd, ifidx), __FUNCTION__, id, prot->reqid));
		ret = -EINVAL;
		goto done;
	}

	/* Check the ERROR flag */
	if (flags & CDCF_IOC_ERROR)
	{
		ret = ltoh32(msg->status);
		/* Cache error from dongle */
		dhd->dongle_error = ret;
	}

done:
	return ret;
}

extern int dhd_bus_interface(struct dhd_bus *bus, uint arg, void* arg2);
int
dhd_prot_ioctl(dhd_pub_t *dhd, int ifidx, wl_ioctl_t * ioc, void * buf, int len)
{
	dhd_prot_t *prot = dhd->prot;
	int ret = -1;

	if (dhd->busstate == DHD_BUS_DOWN) {
		DHD_ERROR(("%s : bus is down. we have nothing to do\n", __FUNCTION__));
		return ret;
	}
	dhd_os_proto_block(dhd);

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

	ASSERT(len <= WLC_IOCTL_MAXLEN);

	if (len > WLC_IOCTL_MAXLEN)
		goto done;

	if (prot->pending == TRUE) {
		DHD_TRACE(("CDC packet is pending!!!! cmd=0x%x (%lu) lastcmd=0x%x (%lu)\n",
			ioc->cmd, (unsigned long)ioc->cmd, prot->lastcmd,
			(unsigned long)prot->lastcmd));
		if ((ioc->cmd == WLC_SET_VAR) || (ioc->cmd == WLC_GET_VAR)) {
			DHD_TRACE(("iovar cmd=%s\n", (char*)buf));
		}
		goto done;
	}

	prot->pending = TRUE;
	prot->lastcmd = ioc->cmd;
	if (ioc->set)
		ret = dhdcdc_set_ioctl(dhd, ifidx, ioc->cmd, buf, len);
	else {
		ret = dhdcdc_query_ioctl(dhd, ifidx, ioc->cmd, buf, len);
		if (ret > 0)
			ioc->used = ret - sizeof(cdc_ioctl_t);
	}

	/* Too many programs assume ioctl() returns 0 on success */
	if (ret >= 0)
		ret = 0;
	else {
		cdc_ioctl_t *msg = &prot->msg;
		CDC_SET_IF_IDX(msg, ifidx);
		ioc->needed = ltoh32(msg->len); /* len == needed when set/query fails from dongle */
	}

	/* Intercept the wme_dp ioctl here */
	if ((!ret) && (ioc->cmd == WLC_SET_VAR) && (!strcmp(buf, "wme_dp"))) {
		int slen, val = 0;

		slen = strlen("wme_dp") + 1;
		if (len >= (int)(slen + sizeof(int)))
			bcopy(((char *)buf + slen), &val, sizeof(int));
		dhd->wme_dp = (uint8) ltoh32(val);
	}

	prot->pending = FALSE;

done:
	dhd_os_proto_unblock(dhd);

	return ret;
}

int
dhd_prot_iovar_op(dhd_pub_t *dhdp, const char *name,
                  void *params, int plen, void *arg, int len, bool set)
{
	return BCME_UNSUPPORTED;
}

void
dhd_prot_dump(dhd_pub_t *dhdp, struct bcmstrbuf *strbuf)
{
	bcm_bprintf(strbuf, "Protocol CDC: reqid %d\n", dhdp->prot->reqid);
}

#ifdef APSTA_PINGTEST
extern struct ether_addr guest_eas[MAX_GUEST];
#endif

void
dhd_prot_hdrpush(dhd_pub_t *dhd, int ifidx, void *pktbuf)
{
#ifdef BDC
	struct bdc_header *h;
#ifdef APSTA_PINGTEST
	struct	ether_header *eh;
	int i;
#ifdef DHD_DEBUG
	char eabuf1[ETHER_ADDR_STR_LEN];
	char eabuf2[ETHER_ADDR_STR_LEN];
#endif /* DHD_DEBUG */
#endif /* APSTA_PINGTEST */
#endif /* BDC */

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

#ifdef BDC
	/* Push BDC header used to convey priority for buses that don't */

#ifdef APSTA_PINGTEST
	eh = (struct ether_header *)PKTDATA(dhd->osh, pktbuf);
#endif

	PKTPUSH(dhd->osh, pktbuf, BDC_HEADER_LEN);

	h = (struct bdc_header *)PKTDATA(dhd->osh, pktbuf);

	h->flags = (BDC_PROTO_VER << BDC_FLAG_VER_SHIFT);
	if (PKTSUMNEEDED(pktbuf))
		h->flags |= BDC_FLAG_SUM_NEEDED;


	h->priority = (PKTPRIO(pktbuf) & BDC_PRIORITY_MASK);
	h->flags2 = 0;
#ifdef APSTA_PINGTEST
	for (i = 0; i < MAX_GUEST; ++i) {
		if (!ETHER_ISNULLADDR(eh->ether_dhost) &&
		    bcmp(eh->ether_dhost, guest_eas[i].octet, ETHER_ADDR_LEN) == 0) {
			DHD_TRACE(("send on if 1; sa %s, da %s\n",
			       bcm_ether_ntoa((struct ether_addr *)(eh->ether_shost), eabuf1),
			       bcm_ether_ntoa((struct ether_addr *)(eh->ether_dhost), eabuf2)));
			/* assume all guest STAs are on interface 1 */
			h->flags2 = 1;
			break;
		}
	}
#endif /* APSTA_PINGTEST */
	h->rssi = 0;
#endif /* BDC */
	BDC_SET_IF_IDX(h, ifidx);
}

int
dhd_prot_hdrpull(dhd_pub_t *dhd, uint *ifidx, void *pktbuf)
{
#ifdef BDC
	struct bdc_header *h;
#endif

	DHD_TRACE(("%s: Enter\n", __FUNCTION__));

#ifdef BDC
	/* Pop BDC header used to convey priority for buses that don't */

	if (PKTLEN(dhd->osh, pktbuf) < BDC_HEADER_LEN) {
		DHD_ERROR(("%s: rx data too short (%d < %d)\n", __FUNCTION__,
		           PKTLEN(dhd->osh, pktbuf), BDC_HEADER_LEN));
		return BCME_ERROR;
	}

	h = (struct bdc_header *)PKTDATA(dhd->osh, pktbuf);

	if (ifidx) {
		*ifidx = (int)BDC_GET_IF_IDX(h);
		ASSERT(*ifidx < DHD_MAX_IFS);
	}

	if (((h->flags & BDC_FLAG_VER_MASK) >> BDC_FLAG_VER_SHIFT) != BDC_PROTO_VER) {
		DHD_ERROR(("%s: non-BDC packet received, flags 0x%x\n",
		           dhd_ifname(dhd, *ifidx), h->flags));
		return BCME_ERROR;
	}

	if (h->flags & BDC_FLAG_SUM_GOOD) {
		DHD_INFO(("%s: BDC packet received with good rx-csum, flags 0x%x\n",
		          dhd_ifname(dhd, *ifidx), h->flags));
		PKTSETSUMGOOD(pktbuf, TRUE);
	}

	PKTSETPRIO(pktbuf, (h->priority & BDC_PRIORITY_MASK));

	PKTPULL(dhd->osh, pktbuf, BDC_HEADER_LEN);
#endif /* BDC */

	return 0;
}

int
dhd_prot_attach(dhd_pub_t *dhd)
{
	dhd_prot_t *cdc;

#ifdef CONFIG_WLAN_ALLOC_STATIC_MEM
	if (addr_4325_dhd_prot!=NULL &&  sizeof(dhd_prot_t)<=DHD_PORT_LEN)  {
	   cdc = (dhd_prot_t *)addr_4325_dhd_prot;
	}
	else
#endif
    {
		DHD_ERROR(("%s: static alloc of %d-byte cdc failed, addr_4325_dhd_prot=0x%p,DHD_PORT_LEN=%d\n",
				__FUNCTION__,sizeof(dhd_prot_t),addr_4325_dhd_prot,DHD_PORT_LEN));
	    if (!(cdc = (dhd_prot_t *)MALLOC(dhd->osh, sizeof(dhd_prot_t)))) {
		    DHD_ERROR(("%s: kmalloc failed\n", __FUNCTION__));
		    goto fail;
	    }
	}
	memset(cdc, 0, sizeof(dhd_prot_t));
	

	/* ensure that the msg buf directly follows the cdc msg struct */
	if ((uintptr)(&cdc->msg + 1) != (uintptr)cdc->buf) {
		DHD_ERROR(("dhd_prot_t is not correctly defined\n"));
		goto fail;
	}

	dhd->prot = cdc;
#ifdef BDC
	dhd->hdrlen += BDC_HEADER_LEN;
#endif
	dhd->maxctl = WLC_IOCTL_MAXLEN + sizeof(cdc_ioctl_t) + ROUND_UP_MARGIN;
	return 0;

fail:
#ifdef CONFIG_WLAN_ALLOC_STATIC_MEM
	if (addr_4325_dhd_prot==NULL || sizeof(dhd_prot_t)>DHD_PORT_LEN)
#endif	
	if (cdc != NULL)
		MFREE(dhd->osh, cdc, sizeof(dhd_prot_t));
	return BCME_NOMEM;
}

/* ~NOTE~ What if another thread is waiting on the semaphore?  Holding it? */
void
dhd_prot_detach(dhd_pub_t *dhd)
{
#ifdef CONFIG_WLAN_ALLOC_STATIC_MEM
    if (addr_4325_dhd_prot==NULL || sizeof(dhd_prot_t)>DHD_PORT_LEN)
#endif	
	MFREE(dhd->osh, dhd->prot, sizeof(dhd_prot_t));
	dhd->prot = NULL;
}

void
dhd_prot_dstats(dhd_pub_t *dhd)
{
	/* No stats from dongle added yet, copy bus stats */
	dhd->dstats.tx_packets = dhd->tx_packets;
	dhd->dstats.tx_errors = dhd->tx_errors;
	dhd->dstats.rx_packets = dhd->rx_packets;
	dhd->dstats.rx_errors = dhd->rx_errors;
	dhd->dstats.rx_dropped = dhd->rx_dropped;
	dhd->dstats.multicast = dhd->rx_multicast;
	return;
}


int dhd_set_suspend(int value, dhd_pub_t *dhd)
{
	int power_mode = PM_MAX;
	wl_pkt_filter_enable_t	enable_parm;
	char iovbuf[32];
	int bcn_li_dtim = 3;
#ifdef CONFIG_KT
	uint roamvar = 1;
#endif /* CONFIG_KT */

#define htod32(i) i

	if (dhd && dhd->up) {
		if (value) {
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_PM,
				(char *)&power_mode, sizeof(power_mode));
			/* Enable packet filter, only allow unicast packet to send up */
			enable_parm.id = htod32(100);
			enable_parm.enable = htod32(1);
			bcm_mkiovar("pkt_filter_enable", (char *)&enable_parm,
				sizeof(wl_pkt_filter_enable_t), iovbuf, sizeof(iovbuf));
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
			/* set bcn_li_dtim */
			bcm_mkiovar("bcn_li_dtim", (char *)&bcn_li_dtim,
				4, iovbuf, sizeof(iovbuf));
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
#ifdef CONFIG_KT
			/* Disable build-in roaming to allowed ext supplicant to take of romaing */
			bcm_mkiovar("roam_off", (char *)&roamvar, 4, iovbuf, sizeof(iovbuf));
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
#endif /* CONFIG_KT */
		} else {
			power_mode = PM_FAST;
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_PM, (char *)&power_mode,
				sizeof(power_mode));
			/* disable pkt filter */
			enable_parm.id = htod32(100);
			enable_parm.enable = htod32(0);
			bcm_mkiovar("pkt_filter_enable", (char *)&enable_parm,
				sizeof(wl_pkt_filter_enable_t), iovbuf, sizeof(iovbuf));
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
			/* set bcn_li_dtim */
			bcn_li_dtim = 0;
			bcm_mkiovar("bcn_li_dtim", (char *)&bcn_li_dtim,
				4, iovbuf, sizeof(iovbuf));
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
#ifdef CONFIG_KT
			roamvar = 0;
			bcm_mkiovar("roam_off", (char *)&roamvar, 4, iovbuf, sizeof(iovbuf));
			dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
#endif /* CONFIG_KT */
		}
	}

	return 0;
}


#define strtoul(nptr, endptr, base) bcm_strtoul((nptr), (endptr), (base))


/* Convert user's input in hex pattern to byte-size mask */
static int
wl_pattern_atoh(char *src, char *dst)
{
	int i;
	if (strncmp(src, "0x", 2) != 0 &&
	    strncmp(src, "0X", 2) != 0) {
		printf("Mask invalid format. Needs to start with 0x\n");
		return -1;
	}
	src = src + 2; /* Skip past 0x */
	if (strlen(src) % 2 != 0) {
		printf("Mask invalid format. Needs to be of even length\n");
		return -1;
	}
	for (i = 0; *src != '\0'; i++) {
		char num[3];
		strncpy(num, src, 2);
		num[2] = '\0';
		dst[i] = (uint8)strtoul(num, NULL, 16);
		src += 2;
	}
	return i;
}




#if 1
void
dhd_keep_alive_enable(dhd_pub_t * dhd)
{
        wl_keep_alive_pkt_t keep_alive_pkt;
        wl_keep_alive_pkt_t *keep_alive_pktp;
        char buf[256];
        char *str;
        int buf_len;
        int str_len;
        str="keep_alive";
        str_len=strlen(str);
        strncpy(buf,str,str_len);
        buf[str_len]='\0';
        buf_len=str_len+1;

	keep_alive_pktp=(wl_keep_alive_pkt_t *) (buf+str_len+1);
        keep_alive_pkt.period_msec=htod32(30000);

	str="0x6e756c6c207061636b657400";
        printk("%s:str=%s\n",__FUNCTION__,str);

	keep_alive_pkt.len_bytes=htod16(wl_pattern_atoh(str,(char*)keep_alive_pktp->data));
        buf_len+=(WL_KEEP_ALIVE_FIXED_LEN+keep_alive_pkt.len_bytes);
        memcpy((char*)keep_alive_pktp,&keep_alive_pkt,WL_KEEP_ALIVE_FIXED_LEN);
        dhdcdc_set_ioctl(dhd,0,WLC_SET_VAR,buf,buf_len);
}
#endif

int
dhd_preinit_ioctls(dhd_pub_t *dhd)
{
	char eventmask[WL_EVENTING_MASK_LEN];
	char iovbuf[WLC_IOCTL_SMLEN];	/*  Room for "event_msgs" + '\0' + bitvec  */
	uint up = 0;
	uint power_mode = PM_FAST;
	uint32 dongle_align = DHD_SDALIGN;
	uint32 glom = 0;
	int ret;

	int arpoe = 1;
	int arp_ol = 0xf;
	int scan_assoc_time = 40;
	int scan_unassoc_time = 80;
#if defined(CONFIG_KT)
	int roamvar = 0;
	int roam_scan_period = 20;
	int roam_trigger = -80;
	int roam_delta = 10;
#else
	uint roamvar = 1;
#endif

	const char *str;
	wl_pkt_filter_t	pkt_filter;
	wl_pkt_filter_t	*pkt_filterp;
	int buf_len;
	int str_len;
	uint32 mask_size;
	uint32 pattern_size;
        char mac_buf[16];
	char buf[256];
	uint filter_mode = 1;
#define htod32(i) i

    uint bcn_timeout = 3;
#if BTCOEXIST_SETTING
    char buf_reg6va_coex[8] = { 6, 00, 00, 00, 0xa, 0x00, 0x00, 0x00 };
#endif

    int i;
    unsigned int o0, o1, o2, o3, o4 ,o5;
    uint8_t smem_mac_addr[20]={0};
    char tmp_mac[20]={0};
    char custom_mac[]={0x00,0x00,0x00,0x00,0x00,0x00};
    memset(smem_mac_addr, 0, 20); 
    msm_get_wlan_mac_addr(smem_mac_addr, 12);
    sprintf(tmp_mac, "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", smem_mac_addr[0], smem_mac_addr[1],
                 smem_mac_addr[2], smem_mac_addr[3], smem_mac_addr[4], smem_mac_addr[5],
                 smem_mac_addr[6], smem_mac_addr[7], smem_mac_addr[8], smem_mac_addr[9],
                 smem_mac_addr[10], smem_mac_addr[11]);
    i=sscanf(tmp_mac, "%x:%x:%x:%x:%x:%x", &o0, &o1, &o2, &o3, &o4, &o5);
    
    if(i == 6)
    {
       custom_mac[0]=o0; custom_mac[1]=o1; custom_mac[2]=o2;
       custom_mac[3]=o3; custom_mac[4]=o4; custom_mac[5]=o5;
       printk("#########Customizing WLAN MAC: %s\n", tmp_mac);
       bcm_mkiovar("cur_etheraddr", custom_mac, ETHER_ADDR_LEN, iovbuf, sizeof(iovbuf));
       dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
    }
    else
    {
       printk("#########Failed of getting MAC\n");
       if(system_rev>=EVT3_Band125)
       {
           bcm_mkiovar("cur_etheraddr", custom_mac, ETHER_ADDR_LEN, iovbuf, sizeof(iovbuf));
           dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
       }
       else
           printk("#########Using Default MAC\n");
    }

	/* Get the device MAC address */
	strcpy(iovbuf, "cur_etheraddr");
	if ((ret = dhdcdc_query_ioctl(dhd, 0, WLC_GET_VAR, iovbuf, sizeof(iovbuf))) < 0) {
		DHD_ERROR(("%s: can't get MAC address , error=%d\n", __FUNCTION__, ret));
		return BCME_NOTUP;
	}
	memcpy(dhd->mac.octet, iovbuf, ETHER_ADDR_LEN);

#if defined(CONFIG_O2)
        strcpy(dhd->country_code,"EU");
#elif defined(CONFIG_KT)
        strcpy(dhd->country_code,"EU");
#endif

	/* Set Country code */
	if (dhd->country_code[0] != 0) {
		if (dhdcdc_set_ioctl(dhd, 0, WLC_SET_COUNTRY,
			dhd->country_code, sizeof(dhd->country_code)) < 0) {
			DHD_ERROR(("%s: country code setting failed\n", __FUNCTION__));
		}
	}

	/* Set PowerSave mode */
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_PM, (char *)&power_mode, sizeof(power_mode));

	/* Match Host and Dongle rx alignment */
	bcm_mkiovar("bus:txglomalign", (char *)&dongle_align, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));

	/* disable glom option per default */
	bcm_mkiovar("bus:txglom", (char *)&glom, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
	/* Setup timeout if Beacons are lost to report link down */
	if (roamvar) {
		bcm_mkiovar("bcn_timeout", (char *)&bcn_timeout, 4, iovbuf, sizeof(iovbuf));
		dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
	}

#if defined(CONFIG_KT)
	bcm_mkiovar("roam_off", (char *)&roamvar, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_ROAM_SCAN_PERIOD, (char *)&roam_scan_period, sizeof(roam_scan_period));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_ROAM_DELTA, (char *)&roam_delta, sizeof(roam_delta));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_ROAM_TRIGGER, (char *)&roam_trigger, sizeof(roam_trigger));
#else
	bcm_mkiovar("roam_off", (char *)&roamvar, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
#endif

	/* Force STA UP */
	dhdcdc_set_ioctl(dhd, 0, WLC_UP, (char *)&up, sizeof(up));


	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	dhdcdc_query_ioctl(dhd, 0, WLC_GET_VAR, iovbuf, sizeof(iovbuf));
	bcopy(iovbuf, eventmask, WL_EVENTING_MASK_LEN);

	dhd_keep_alive_enable(dhd);

	/* Setup event_msgs */
	setbit(eventmask, WLC_E_SET_SSID);
	setbit(eventmask, WLC_E_PRUNE);
	setbit(eventmask, WLC_E_AUTH);
	setbit(eventmask, WLC_E_REASSOC);
	setbit(eventmask, WLC_E_REASSOC_IND);
	setbit(eventmask, WLC_E_DEAUTH_IND);
	setbit(eventmask, WLC_E_DISASSOC_IND);
	setbit(eventmask, WLC_E_DISASSOC);
	setbit(eventmask, WLC_E_JOIN);
	setbit(eventmask, WLC_E_ASSOC_IND);
	setbit(eventmask, WLC_E_PSK_SUP);
	setbit(eventmask, WLC_E_LINK);
	setbit(eventmask, WLC_E_NDIS_LINK);
	setbit(eventmask, WLC_E_MIC_ERROR);
	setbit(eventmask, WLC_E_PMKID_CACHE);
	setbit(eventmask, WLC_E_TXFAIL);
	setbit(eventmask, WLC_E_JOIN_START);
	setbit(eventmask, WLC_E_SCAN_COMPLETE);

	bcm_mkiovar("event_msgs", eventmask, WL_EVENTING_MASK_LEN, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));

	dhdcdc_set_ioctl(dhd, 0, WLC_SET_SCAN_CHANNEL_TIME, (char *)&scan_assoc_time,
		sizeof(scan_assoc_time));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_SCAN_UNASSOC_TIME, (char *)&scan_unassoc_time,
		sizeof(scan_unassoc_time));
#if BTCOEXIST_SETTING
 	bcm_mkiovar("btc_params", (char *)&buf_reg6va_coex[0], sizeof(buf_reg6va_coex), iovbuf, sizeof(iovbuf));
        dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
#endif
	/* Set ARP offload */
	bcm_mkiovar("arpoe", (char *)&arpoe, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));
	bcm_mkiovar("arp_ol", (char *)&arp_ol, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));

	/* add a default packet filter pattern */
	str = "pkt_filter_add";
	str_len = strlen(str);
	strncpy(buf, str, str_len);
	buf[ str_len ] = '\0';
	buf_len = str_len + 1;

	pkt_filterp = (wl_pkt_filter_t *) (buf + str_len + 1);

	/* Parse packet filter id. */
	pkt_filter.id = htod32(100);

	/* Parse filter polarity. */
	pkt_filter.negate_match = htod32(0);

	/* Parse filter type. */
	pkt_filter.type = htod32(0);

	/* Parse pattern filter offset. */
	pkt_filter.u.pattern.offset = htod32(0);

        mask_size = htod32(wl_pattern_atoh("0xffffffffffff",
                (char *) pkt_filterp->u.pattern.mask_and_pattern));

        sprintf( mac_buf, "0x%02x%02x%02x%02x%02x%02x",
                            dhd->mac.octet[0], dhd->mac.octet[1], dhd->mac.octet[2],
                            dhd->mac.octet[3], dhd->mac.octet[4], dhd->mac.octet[5]);

        pattern_size = htod32(wl_pattern_atoh(mac_buf,
                (char *) &pkt_filterp->u.pattern.mask_and_pattern[mask_size]));

#if 0
	/* Parse pattern filter mask. */
	mask_size = htod32(wl_pattern_atoh("0x01",
		(char *) pkt_filterp->u.pattern.mask_and_pattern));

	/* Parse pattern filter pattern. */
	pattern_size = htod32(wl_pattern_atoh("0x00",
		(char *) &pkt_filterp->u.pattern.mask_and_pattern[mask_size]));
#endif

	if (mask_size != pattern_size) {
		printk("Mask and pattern not the same size\n");
		return -1;
	}

	pkt_filter.u.pattern.size_bytes = mask_size;
	buf_len += WL_PKT_FILTER_FIXED_LEN;
	buf_len += (WL_PKT_FILTER_PATTERN_FIXED_LEN + 2 * mask_size);

	/* Keep-alive attributes are set in local	variable (keep_alive_pkt), and
	** then memcpy'ed into buffer (keep_alive_pktp) since there is no
	** guarantee that the buffer is properly aligned.
	*/
printk("Before memmove\n");
pkt_filterp = memmove((void *)pkt_filterp,(void *) &pkt_filter,
		WL_PKT_FILTER_FIXED_LEN + WL_PKT_FILTER_PATTERN_FIXED_LEN);
printk("After memmove\n");
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, buf, buf_len);

	/* set mode to allow pattern */
	bcm_mkiovar("pkt_filter_mode", (char *)&filter_mode, 4, iovbuf, sizeof(iovbuf));
	dhdcdc_set_ioctl(dhd, 0, WLC_SET_VAR, iovbuf, sizeof(iovbuf));

	return 0;
}

int
dhd_prot_init(dhd_pub_t *dhd)
{
	int ret = 0;
	DHD_TRACE(("%s: Enter\n", __FUNCTION__));


	ret = dhd_preinit_ioctls(dhd);

	/* Always assumes wl for now */
	dhd->iswl = TRUE;

    q_wlan_flag = 1;

	return ret;
}

void
dhd_prot_stop(dhd_pub_t *dhd)
{
	/* Nothing to do for CDC */
}
