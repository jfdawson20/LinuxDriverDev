#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kernel.h>    /* printk() */
#include <linux/slab.h>      /* kmalloc() */
#include <linux/errno.h>     /* error codes */
#include <linux/types.h>     /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>
#include <linux/pci.h>

#include "8139-jack.h"

#include <linux/in6.h>
#include <asm/checksum.h>

MODULE_AUTHOR("Jack Dawson");
MODULE_LICENSE("Dual BSD/GPL");

struct rtl8139_priv {
	struct net_device_stats stats;
	int status;
	int rx_int_enabled;
	int tx_packetlen;
	u8 *tx_packetdata;
	struct sk_buff *skb;
	spinlock_t lock;
	struct net_device *dev;
	struct napi_struct napi;
};

static const struct net_device_ops rtl8139_netdev_ops = {
	.ndo_open            = rtl8139_open,
	.ndo_stop            = rtl8139_release,
	.ndo_start_xmit      = rtl8139_xmit
	//.ndo_do_ioctl        = snull_ioctl,
	//.ndo_set_config      = snull_config,
	//.ndo_get_stats       = snull_stats,
	//.ndo_change_mtu      = snull_change_mtu,
	//.ndo_tx_timeout      = snull_tx_timeout
};

struct net_device *rtl8139_dev;

int rtl8139_open (struct net_device *dev)
{
	printk("rtl8139_open called\n");
	return 0;
}

int rtl8139_release (struct net_device *dev)
{
	printk ("rtl8139_release called\n");
	return 0;
}

static int rtl8139_xmit (struct sk_buff *skb, struct net_device *dev)
{
	printk ("dummy xmit function called....\n");
	return 0;
}

void rtl8139_init (struct net_device *dev)
{
	struct rtl8139_priv *priv;
	
	ether_setup(dev); /* assign some of the fields */
	
	dev->watchdog_timeo = TIMEOUT;

	dev->netdev_ops = &rtl8139_netdev_ops;
	//dev->header_ops = &snull_header_ops;
	/* keep the default flags, just add NOARP */
	//dev->flags           |= IFF_NOARP;
	//dev->features        |= NETIF_F_HW_CSUM;

	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	
	//netif_napi_add(dev, &priv->napi, snull_poll,2);

	memset(priv, 0, sizeof(struct rtl8139_priv));
	spin_lock_init(&priv->lock);
	//snull_rx_ints(dev, 1);		/* enable receive interrupts */
	//snull_setup_pool(dev);
}

int rtl8139_init_module (void)
{
	int result, ret = -ENOMEM;

	//snull_interrupt = use_napi ? snull_napi_interrupt : snull_regular_interrupt;

	/* Allocate the devices */
	rtl8139_dev = alloc_netdev(sizeof(struct rtl8139_priv), "eth%d", rtl8139_init);
	
	if (rtl8139_dev == NULL)
		goto out;

	ret = -ENODEV;
	result = register_netdev(rtl8139_dev);
	if (result)
        printk("8139-jack: error %i registering device \"%s\"\n", result, rtl8139_dev->name);
    else
        ret = 0;
out:
	
    if (ret){ 
		rtl8139_cleanup();
	}
	return ret;

}
	
void rtl8139_cleanup (void)
{
    if (rtl8139_dev) {
        unregister_netdev(rtl8139_dev);
	    free_netdev(rtl8139_dev);
    }
	
	return;
}
	
module_init (rtl8139_init_module);
module_exit (rtl8139_cleanup);

