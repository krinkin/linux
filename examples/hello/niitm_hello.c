#include <linux/module.h>
#include "niitm_hello.h"

#define NIITM_NAME "niitm hello"

static int niitm_init(void)
{
	printk(KERN_INFO "Loaded %s current = %p\n", NIITM_NAME,current);
	niitm_print_hello();
	return 0;
}

static void niitm_exit(void)
{
	printk(KERN_INFO "Unloaded %s\n", NIITM_NAME);
}

module_init(niitm_init);
module_exit(niitm_exit);


