/*
*page table item ops
*/
#define set_val(p, paddr) (*p = (paddr&0xfffff000))
#define get_val(p) ((*p)&0xfffff000)

#define set_pde(p, paddr) set_val((p), paddr)
#define get_pde(p) get_val((p))

#define set_pte(p, paddr) set_val((p), paddr)
#define get_pte(p) get_val((p))

#define clean(p) (*(p) = 0)
#define clean_attr(p) (*(p) &= 0xfffff000)

#define	set_X(p) (*(p) |= 0x00000008)
#define clean_X(p) (*(p) &= 0xfffffff7)

#define	set_W(p) (*(p) |= 0x00000004)
#define clean_W(p) (*(p) &= 0xfffffffb)

#define	set_U(p) (*(p) |= 0x00000002)
#define clean_U(p) (*(p) &= 0xfffffffd)

#define	set_P(p) (*(p) |= 0x00000001)
#define clean_P(p) (*(p) &= 0xfffffffe)