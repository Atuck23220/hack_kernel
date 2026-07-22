#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE0(aaron_tuck)
{

	printk(KERN_INFO "Hello from the Aaron Tuck syscall! You've reached kernel space.\n");
	return 0;
}
