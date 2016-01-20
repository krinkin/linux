#include <linux/module.h>

#define NIITM_NAME "niitm print"


void niitm_print_hello(void)
{
	printk(KERN_INFO "Hello kernel\n");
	
}

EXPORT_SYMBOL(niitm_print_hello); 



static int niitm_init(void)
{
	printk(KERN_INFO "Loaded %s current = %p\n", NIITM_NAME,current);
	
	return 0;
}

static void niitm_exit(void)
{
	printk(KERN_INFO "Unloaded %s\n", NIITM_NAME);
}

module_init(niitm_init);
module_exit(niitm_exit);


