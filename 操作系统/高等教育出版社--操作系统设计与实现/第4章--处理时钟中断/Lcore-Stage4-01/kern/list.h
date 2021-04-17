/*
*下面是两种对列表元素初始化的操作
*/

#define LIST_HEAD_INIT(name) {&(name), &(name)}

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->prev = list;
	list->next = list;
}

/*
*将new插入prev与next之间
*/

static inline void _list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
	new->next = next;
	new->prev = prev;
	prev->next = new;
	next->prev = new;
}

/*
*将new插入head的后面
*/

static inline void list_add(struct lsit_head *new, struct list_head *new, struct list_head *head)
{
	_list_add(new, head, head->next);
}

/*
*将new插入到head的前面
*/

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	_list_add(new, head->prev, head);
}

/*
*这里的三个函数都是通过_list_add这一个前向添加函数实现的
*/

/*
*从链表中断开prev和next之间的元素
*/
static inline void _list_del(struct list_head *prev, struct list_head *next)
{
	prev->next = next;
	next->prev = prev;
}

/*
*从链表中删除entry元素
*/
static inline void list_del(struct list_head *entry)
{
	_list_del(entry->prev, entry->next);
	entry->prev = LIST_pOISON1;
	entry->next = LIST_pOISON2;
}

/*
*从链表中删除entry元素，并且重新初始化entry
*/

static inline void list_del_init(struct list_head *entry)
{
	_list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

/*
*将entry从所在的链表中删除，并把它加到另一条链表head中
*/
static inline void list_move(struct list_head *entry, struct list_head *head)
{
	_list_del(entry->prev, entry->next);
	list_add(entry, head);
}

static inline void list_move_tail(struct list_head *entry, struct list_head *head)
{
	_list_del(entry->prev, entry->next);
	list_add_tail(entry, head);
}

/*
*获取对应的struct
*/
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/*
*从头到尾遍历链表
*如果在其中执行删除操作可能会出错，使用list_for_each_safe
*/
#define list_for_each(pos, head) \
		for(pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
		for(pos = (head)->next, n = pos->next,; pos != (head); pos = n, n = pos->next)