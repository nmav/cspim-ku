void __start(void)
{
	asm volatile("jal main ; break 0");
}
