#include <linux/module.h>

#define NIITM_NAME "niitm hello"

static void __init niitm_init()
{
	printk(KERN_INFO "Loaded %s\n", NIITM_NAME);
}

static void __exit niitm_exit()
{
	printk(KERN_INFO "Unloaded %s\n", NIITM_NAME);
}

module_init(niitm_init);
module_exit(niitm_exit);


