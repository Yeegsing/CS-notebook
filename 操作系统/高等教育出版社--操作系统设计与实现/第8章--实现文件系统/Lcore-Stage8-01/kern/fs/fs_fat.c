//find victim in 512buffer/4k
u32 fs_victim_512(BUF_512 *buf, u32 *clock_head, u32 size)
{
	u32 i;
	u32 index = *clock_head;

	//sweep 1
	for(i = 0; i < size; i++)
	{
		if(((buf[*clock_head].state) & 0x01) == 0)
		{
			if(((buf[*clock_head].state) & 0x02) == 0)
			{
				index = *clock_head;
				goto fs_victim_512_ok;
			}
		}
		else
			buf[*clock_head].state &= 0xfe;

		if((++(*clock_head)) >= size)
			*clock_head = 0;
	}

	//sweep 2
	for(i = 0; i < size; i++)
	{
		if(((buf[*clock_head].state) & 0x02) == 0)
		{
			index = *clock_head;
			goto fs_victim_512_ok;
		}

		if((++(*clock_head)) >= size)
			*clock_head = 0;
	}

	index = *clock_head;

fs_victim_512_ok:
	if((++(clock_head)) >= size)
		*clock_head = 0;
	
	return index;
}

//这是较为简单的写操作
u32 fs_write_512(BUF_512 *f)
{
	if((f->cur != 0xffffffff) && (((f->state) & 0x02) != 0))
	{
		if(fs_write_block(f->buf, f->cur, 1) == 1)
			goto fs_write_512_err;

		f->state &= 0x01;
	}

	return 0;

fs_write_512_err:
	return 1;
}

//实现缓存的读操作
u32 fs_read_512(BUF_512 *f, u32 FirstSectorOfCluster, u32 *clock_head, u32 size)
{
	u32 index;
	for(index = 0; (index < size) && (f[index].cur != FirstSectorOfCluster); index++);

	if(index == size)
	{
		index = fs_victim_512(f, clock_head, size);
		if(fs_write_512(f + index) ==1)
			goto fs_read_512_err;

		if(fs_read_block(f[index].buf, FirstSectorOfCluster, 1) == 1)
			goto fs_read_512_err;
		f[index].cur = FirstSectorOfCluster;
		f[index].state = 1;
	}
	else{
		f[index].state |= 0x01;
	}
	return index;
fs_read_512_err:
	return 0xffffffff;
}

//接下来就应该实现FAT表了
void fs_clus2fat(u32 clus, u32 *ThisFATSecNum, u32 *ThisFATEntOffset)
{

	u32 FATOffset = clus << 2;//这个clus在FAT表中的表项距离FAT表头部的字节数
	*ThisFATSecNum = BPB_RsvdSecCnt + (FATOffset >> 9);//BPB_RsvdSecCnt是FAT表的起始地址，初始化给出
	*ThisFATEntOffset = FATOffset & 511;//这个表项在这个sector内的偏移
}

//这里是获取一个FAT表项的内容
u32 fs_get_fat_entry_val(u32 clus, u32 *ClusEntryVal)
{
	u32 ThisFATSecNum;
	u32 ThisFATEntOffset;
	u32 index;

	fs_clus2fat(clus, &ThisFATSecNum, &ThisFATEntOffset);

	index = fs_read_fat_sector(ThisFATSecNum);
	if(index == 0xffffffff)
		goto fs_get_fat_entry_val_err;

	*ClusEntryVal = fs_u8_to_u32(fat_buf[index].buf + ThisFATEntOffset) & 0x0FFFFFFF;

	return 0;
fs_get_fat_entry_val_err:
	return 1;
}

//修改FAT表
u32 fs_modify_fat(u32 clus, u32 ClusEntryVal)
{
	u32 ThisFATSecNum;
	u32 ThisFATEntOffset;
	u32 fat32_val;
	u32 index;

	fs_clus2fat(clus, &ThisFATSecNum, &ThisFATEntOffset);

	index = fs_read_fat_sector(ThisFATSecNum);
	if(index == 0xffffffff)
		goto fs_modify_fat_err;

	fat_buf[index].state = 3;

	ClusEntryVal &= 0x0FFFFFFF;
	fat32_val = (fs_u8_to_u32(fat_buf[index].buf + ThisFATEntOffset) & 0xF0000000) | ClusEntryVal;
	fs_u32_to_u8(fat_buf[index].buf + ThisFATEntOffset, fat32_val);

	return 0;
fs_modify_fat_err:
	return 1;
}

//文件系统初始化
u32 fs_init()
{
	u16 BPB_RootEntCnt;
	u32 TotSec;
	u32 DataSec;
	u32 i;

	if(sd_init() == 1)
		goto fs_init_err;

	if(sd_read_block(meta_buf, 0, 1) == 1)
		goto fs_init_err;

	base_addr =  fs_u8_to_u32(meta_buf + 446 + 8);

	if(fs_read_block(meta_buf, 0, 1) == 1)
		goto fs_init_err;

	if(fs_u8_to_u32(meta_buf + 11) != 512)
		goto fs_init_err;

	BPB_RootEntCnt = fs_u8_to_u16(meta_buf + 17);
	if(BPB_RootEntCnt != 0)
		goto fs_init_err;

	FATSz = fs_u8_to_u16(meta_buf + 22);

	if(FATSz != 0)
		goto fs_init_err;
}
