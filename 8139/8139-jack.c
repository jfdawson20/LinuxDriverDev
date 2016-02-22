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
	struct pci_dev *pdev;
    unsigned long base_addr;
    void *mmio_addr;
    unsigned long regs_len; 
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


static struct pci_dev* probe_for_realtek8139(void) 
{
    struct pci_dev *pdev = NULL;
    pdev = pci_get_device(REALTEK_VENDER_ID, REALTEK_DEVICE_ID, NULL);
    if(pdev) {
        /* device found, enable it */
        if(pci_enable_device(pdev)) {
            printk("Could not enable the device\n");
            return NULL;
        }
        else
            printk("Device enabled\n");
    }
    else {
        printk("device not found\n");
        return pdev;    
    }
    
    return pdev;
}


int rtl8139_device_init(struct net_device *dev)
{
    unsigned long mmio_start, mmio_end, mmio_len, mmio_flags;
    int i = 0;
    void *ioaddr;
    struct rtl8139_priv *p = netdev_priv(dev);

    p->pdev = probe_for_realtek8139();
    if(!p->pdev)
           return -1;

    /* get PCI memory mapped I/O space base address from BAR1 */
    mmio_start = pci_resource_start(p->pdev, 1);
    mmio_end = pci_resource_end(p->pdev, 1);
    mmio_len = pci_resource_len(p->pdev, 1);
    mmio_flags = pci_resource_flags(p->pdev, 1);

    /* make sure above region is MMI/O */
    if(!(mmio_flags & IORESOURCE_MEM)) {
           printk("region not MMI/O region\n");
           return -1;
    }

    /* get PCI memory space */
    if(pci_request_regions(p->pdev, DRIVER)) {
           printk("Could not get PCI region\n");
           return -1;
    }

    pci_set_master(p->pdev);
    
    /* ioremap MMI/O region */
    ioaddr = ioremap(mmio_start, mmio_len);
    if(!ioaddr) {
           printk("Could not ioremap\n");
           return -1;
    }

    p->base_addr = (long)ioaddr;
    p->mmio_addr = ioaddr;
    p->regs_len = mmio_len;

    /* UPDATE NET_DEVICE */
   for(i = 0; i < 6; i++) {  /* Hardware Address */
           dev->dev_addr[i] = readb((const volatile void *)(p->base_addr+i));
           dev->broadcast[i] = 0xff;
    }

   printk("8139 MAC: %x:%x:%x:%x:%x:%x\n",dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2], dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);
   return 0;
}
void rtl8139_init (struct net_device *dev)
{
    int ret = 0;
	struct rtl8139_priv *priv;
	
	ether_setup(dev); /* assign some of the fields */
	
	dev->watchdog_timeo = TIMEOUT;

	dev->netdev_ops = &rtl8139_netdev_ops;
	//dev->header_ops = &snull_header_ops;
	/* keep the default flags, just add NOARP */
	//dev->flags           |= IFF_NOARP;
	//dev->features        |= NETIF_F_HW_CSUM;

	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct rtl8139_priv));

    ret = rtl8139_device_init(dev);

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
	
    if (ret) 
		rtl8139_cleanup();
	
	return ret;

}
	
void rtl8139_cleanup (void)
{
    struct rtl8139_priv *p;
    
    if (rtl8139_dev) {
        p = netdev_priv(rtl8139_dev);

        iounmap(p->mmio_addr);
        pci_release_regions(p->pdev);

        pci_disable_device(p->pdev);

        unregister_netdev(rtl8139_dev);
	    free_netdev(rtl8139_dev);
    }
    printk("8139: cleanup finished\n");
	return;
}

module_init (rtl8139_init_module);
module_exit (rtl8139_cleanup);

