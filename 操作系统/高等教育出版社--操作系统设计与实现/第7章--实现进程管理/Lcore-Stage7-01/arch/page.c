unsigned int do_one_mapping(unsigned int *pgd, unsigned int va, unsigned int pa, unsigned int attr)
{
	//pgd表示要操作的这套页表的页目录基址，其值指向第一级页表
	//va表示要添加映射关系的线性地址
	//pa表示要添加映射关系的物理地址
	//attr表示要添加映射关系的属性
	unsigned int pde_index = (va >> PGD_SHIFT) & INDEX_MASK;
	unsigned int pte_index = (va >> PTE_SHIFT) & INDEX_MASK;
	unsigned int *pt;

	pt = (unsigned int *)kmalloc(PAGE_SIZE);
	if(pt == NULL)
		return 1;

	pgd[pde_index] = (unsigned int)pt & (~OFFSET_MASK);
	pgd[pde_index] |= attr;
	pa &= (~OFFSET_MASK);
	pa |= attr;
	pt[pte_index] = pa;

	return 0;
}
