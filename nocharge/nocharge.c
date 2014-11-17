#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("aschulm, 2014");
MODULE_DESCRIPTION("Fake out USB charging disabled");

int init_module(void)
{
	printk("<1>Hello world\n");
	return 0;
}

void cleanup_module(void)
{
  printk(KERN_ALERT "Goodbye world 1.\n");
}
