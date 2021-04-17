/*
*初始化内核页表
*返回最后一个页表的末尾（其后的一段空间将作为vga缓存）
*/
int init_pgtable()
{
	int *pt = (int *)_KERNEL_PT_START;
	int vaddr = 0;
	int paddr = 0;
	int index, n;
	int max_mm = get_phymn_size();
	pgd = (int *)_KERNEL_PT_START;

	/*
	*内核的常规内存页表
	*/
	while((paddr < max_mm) && (vaddr < _KERNEL_VIRT_END)) {
		set_pgd_entry(pgd, (int)pt, vaddr, 1, 0, 1);
		for (index = 0; index < 1024; ++index) {
			set_pgd_entry(pt, paddr, paddr, 1, 0, 1);//后三位分别对应写权限、用户态标志和有效位
			paddr += PAGE_SIZE;
		}
		pt += 1024;
		vaddr += (PAGE_SIZE * 1024);
	}

	/*
	* ROM、I/O地址空间页表
	*/
	vaddr = ROM_START;
	paddr = ROM_START;
	n = 4;
	while(n--) {
		set_pgd_entry(pgd, (int)pt, vaddr, 1, 0, 1);
		for (index = 0; index < 1024; ++index) {
			set_pt_entry(pt, paddr, paddr, 1, 0, 1);
			paddr += PAGE_SIZE;
		}
		pt += 1024;
		vaddr += ( PAGE_SIZE * 1024);
	}
	return (int)pt;
} 

void enable_paging(int *pgd)
{
	int val = ((int)pgd |0x1);//最低位置位表示开启分页功能
	asm volatile (
		"mtc0 %0, $6"
		:
		:"r"(val)
		);
}

void disable_paging(int *pgd)
{
	int val;
	asm volatile (
		"mfc0 %0, $6"
		:"=r"(val)
		);
	val &= 0xfffffffe;//清楚最低为表示关闭分页功能
	asm volatile (
		"mtc0 %0, $6"
		:
		:"r"(val)
		);
}

void set_pgd_entry(int *pgd, int p_pt, int vaddr, int w, int u, int p)
{
	int index = ((vaddr >> PGD_SHIFT) & INDEX_MASK);
	int *pde = pgd + index;
	clean(pde);
	set_pde(pde, p_pt);
	set_X(pde);
	if(p)
		set_P(pde);
	if(W)
		set_W(pde);
	if(u)
		set_u(pde);
}
