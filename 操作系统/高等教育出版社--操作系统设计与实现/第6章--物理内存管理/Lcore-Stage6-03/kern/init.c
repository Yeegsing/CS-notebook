void machine_info()
{
	printk("About this machine:\n");
	printk("\tCPU: %x MHz \n", get_cpu_hz());
	printk("\tRAM size is %x Bytes \n", get_phymm_size());
	printk("\tDisk size is %x Bytes \n", get_sd_size());
}

void init_kernel()
{
	init_exint();
	init_bootmm();
	init_vga();
	init_keyboard();
	init_time(_CLOCK_INTERVAL);
	init_pgtable();
	enable_paging(pgd);
	bootmap_info("BootMM info");
	init_buddy();
	init_slub();
	machine_info();
	enable_intr(_INTR_GLOBAL | _INtR_CLOCK | _INTR_KEYB | _INTR_SPI);
}
