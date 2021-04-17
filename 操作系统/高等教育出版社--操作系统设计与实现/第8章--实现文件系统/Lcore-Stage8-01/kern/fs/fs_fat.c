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

	if(sd_init() == 1)//调用SD卡初始化操作
		goto fs_init_err;

	if(sd_read_block(meta_buf, 0, 1) == 1)//读取SD卡位于0地址的第一个块，也就是MBR
		goto fs_init_err;

	base_addr =  fs_u8_to_u32(meta_buf + 446 + 8);//目的就是为了找到第一个分区的偏移量base_addr

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

	else
		FATSz = fs_u8_to_u32(meta_buf + 36);

	TotSec = fs_u8_to_u16(meta_buf + 19);

	if(TotSec != 0)
		goto fs_init_err;
	else
		TotSec = fs_u8_to_u32(meta_buf + 32);

	BPB_RsvdSecCnt(meta_buf + 14);
	FirstDataSector = BPB_RsvdSecCnt + (FATSz << 1);
	//BPB_RsvdSecCnt是指FAT表的起始地址
	//FATSz是指FAT表的大小，用于在写FAT表的同时写备份的FAT表
	DataSec = TotSec - FirstDataSector;//FirstDataSector是指数据区的起始地址

	BPB_SecPerClus = meta_buf[13];

	CountOfClusters = DataSec >> fs_wa(BPB_SecPerClus);
	if(CountOfClusters < 65525)//Cluster的数量
		goto fs_init_err;
	else
		//将FSInfo保持在缓冲区中
		fs_read_block(meta_buf, 1, 1);

	//初始化全局缓冲区
	for(i = 0; i < FAT_BUF_NUM; i++)
	{
		fat_buf[i].cur = 0xffffffff;
		fat_buf[i].state = 0;
	}

	for(i = 0; i < DIR_DATA_BUF_NUM; i++)
	{
		dir_data_buf[i].cur = 0xffffffff;
		dir_data_buf[i].state = 0;
	}

	for(i = 0; i < 4096; i++)
		new_alloc_empty[i] = 0;

	return 0;
fs_init_err:
	return 1;
}


u32 fs_find(FS_FAT_FILE *file)
{
	u8 *f = file->filename;
	u32 next_slash;
	u32 i, k;
	u32 next_clus;
	u32 index;
	u32 sec;

	if(*(f++) != '/')
		goto fs_find_err;

	index = fs_read_512(dir_data_buf, fs_dataclus2sec(2), &dir_data_clock_head, DIR_DATA_BUF_NUM);
	if(index = 0xffffffff)
		goto fs_find_err;

	while(1)
	{
		file->dir_entry_pos = 0x0FFFFFFF;
		next_slash = fs_next_slash(f);

		while(1)
		{
			for(sec = 1; sec <= BPB_SecPerClus; sec++)
			{
				for(i = 0; i < 512; i += 32)
				{
					if(*(dir_data_buf[index].buf + i) == 0)
						goto after_fs_find;

					if((fs_cmp_filename(dir_data_buf[index].buf + i, filename11) == 0) && ((*(dir_data_buf[index].buf + i + 11) & 0x08) == 0))
					{
						file->dir_entry_pos = i;
						file->dir_entry_sector = dir_data_buf[index].cur;

						for(k = 0; k < 32; k++)
						{
							file->dir_entry[k] = *(dir_data_buf[index].buf + i + k);

							goto after_fs_find;
						}
					}
				}

				if(sec < BPB_SecPerClus)
				{
					index = fs_read(dir_data_buf, dir_data_buf[index].cur + sec, &dir_data_clock_head, DIR_DATA_BUF_NUM);
					if(index == 0xffffffff)
						goto fs_find_err;
				}
				else
				{
					if(fs_get_fat_entry_val(dir_data_buf[index].cur - BPB_SecPerClus + 1, &next_clus) == 1)
						goto fs_find_err;

					if(next_clus <= CountOfClusters + 1)
					{
						index = fs_read_512(dir_data_buf, 
								fs_dataclus2sec(next_clus),
								&dir_data_clock_head, 
								DIR_DATA_BUF_NUM);
						if(index == 0xffffffff)
							goto fs_find_err;
					}
				}
			}
		}
after_fs_find:
		if(file->dir_entry_pos == 0xffffffff)
			goto fs_find_ok;
		
		if(f[next_slash] == 0)
			goto fs_find_ok;
		
		if((file->dir_entry[11] & 0x10) == 0)
			goto fs_find_err;
		f += next_slash + 1;

		next_clus = fs_hilo2clus(file->dir_entry);
		if(next_clus <= CountOfClusters + 1)
		{
			index = fs_read_512(dir_data_buf,
					fs_dataclus2sec(next_clus),
					&dir_data_clock_head, DIR_DATA_BUF_NUM);
			if(index == 0xffffffff)
				goto fs_find_err;
		}
		else 
			goto fs_find_err;
	}
fs_find_ok:
	return 0;
fs_find_err:
	return 1;
}

u32 fs_open(FS_FAT_FILE *file, u8 *filename)
{
	u32 i;

	for(i = 0; i < LOCAL_DATA_BUF_NUM; i++)
	{
		file->data_buf[i].cur = 0xffffffff;
		file->data_buf[i].state = 0;
	}

	file->clock_head = 0;

	for(i = 0; i < 256; i++)
		file->filename[i] = 0;
	for(i = 0; i < 256 && filename[i] != 0; i++)
		file->filename[i] = filename[i];

	file->loc = 0;

	if(fs_find(file) == 1)
		goto fs_open_err;

	if(file->dir_entry_pos == 0x0FFFFFFF)
		goto fs_open_err;

	return 0;

fs_open_err:
	return 1;
}

//关闭文件主要有两项操作，一是将各种缓存同步到磁盘上，二是对磁盘上文件的目录进行更新
u32 fs_fflush()
{
	u32 i;

	if(fs_wrtie_block(meta_buf, 1, 1) == 1)
		goto fs_fflush_err;

	if(fs_write_block(meta_buf, 7, 1) == 1)
		goto fs_fflush_err;

	for(i = 0; i < FAT_BUF_NUM; i++)
		if(fs_write_fat_sector(i) == 1)
			goto fs_fflush_err;

	for(i = 0; i < DIR_DATA_BUF_NUM; i++)
		if(fs_write_512(dir_data_buf + i) == 1)
			goto fs_fflush_err;

	return 0;

	fs_fflush_err:
	return 1;
}

u32 fs_close(FS_FAT_FILE *file)
{
	u32 i;
	u32 index;

	index = fs_read_512(dir_data_buf,
			file->dir_entry_sector,
			&dir_data_clock_head, DIR_DATA_BUF_NUM);
	if(index == 0xffffffff)
		goto fs_close_err;
	dir_data_buf[index].state = 3;

	for(i = 0; i < 32; i++)
	{
		*(dir_data_buf[index].buf + file->dir_entry_pos + i)
			= file->dir_entry[i];
	}
	if(fs_fflush() == 1)
		goto fs_close_err;

	for(i = 0; i < LOCAL_DATA_BUF_NUM; i++)
		if(fs_wrtie_4k(file->data_buf + i) == 1)
			goto fs_close_err;

	return 0;
fs_close_err:
	return 1;
}

//文件读取
u32 fs_read(FS_FAT_FILE *file, u8 *buf, u32 count)
{
	u32 start_clus, start_byte;
	u32 end_clus, end_byte;
	u32 filesize = fs_u8_to_u32(file->dir_entry + 28);
	u32 clus = fs_hilo2clus(file->dir_entry);
	u32 next_clus;
	u32 i;
	u32 cc;
	u32 index;

	if(clus == 0)
		return 0;

	if(file->loc + count > filesize)
		count = filesize - file->loc;

	if(count == 0)
		return 0;

	start_clus = file->loc>>fs_wa(BPB_SecPerClus << 9);
	start_byte = file->loc & ((BPB_SecPerClus << 9) - 1);
	end_clus = (file->loc + count - 1) >> fs_wa(BPB_SecPerClus << 9);
	end_byte = (file->loc + count - 1) & ((BPB_SecPerClus << 9) - 1);

	for(i = 0; i < start_clus; i++)
	{
		if(fs_get_fat_entry_val(clus, &next_clus) == 1)
			goto fs_read_err;

		clus = next_clus;
	}

	cc = 0;
	while(start_clus <= end_clus)
	{
		index = fs_read_4k(file->data_buf,
				fs_dataclus2sec(clus), &(file->clock_head),
				LOCAL_DATA_BUF_NUM);
		if(index == 0xffffffff)
			goto fs_read_err;
		if(start_clus == end_clus)
		{
			for(i = start_byte; i <= end_byte; i++)
				buf[cc++] = file->data_buf[index].buf[i];

			break;
		}
		else
		{
			for(i = start_byte; i < (BPB_SecPerClus << 9); i++)
				buf[cc++] = file->data_buf[index].buf[i];

			start_clus++;
			start_byte = 0;

			if(fs_get_fat_entry_val(clus, &next_clus) == 1)
				goto fs_read_err;

			clus = next_clus;
		}
	}

	file->loc += count;
	return cc;
fs_read_err:
	return 0xffffffff;
}

//文件写入
u32 fs_wrtie(FS_FAT_FILE *file, u8 *buf, u32 count)
{
	u32 start_clus, start_byte;
	u32 end_clus, end_byte;
	u32 filesize = fs_u8_to_u32(file->dir_entry + 28);
	u32 new_filesize = filesize;
	u32 clus_hilo2clus(file->dir_entry);
	u32 next_clus;
	u32 i;
	u32 cc;
	u32 index;

	if(count == 0)
		return 0;

	if(file->loc + count > filesize)
		new_filesize = file->loc + account;
	
	start_clus = file->loc>>fs_wa(BPB_SecPerClus << 9);
	start_byte = file->loc & ((BPB_SecPerClus << 9) - 1);
	end_clus = (file->loc + count -1) >> fs_wa(BPB_SecPerClus << 9);
	end_byte = (file->loc + count -1) & ((BPB_SecPerClus << 9) - 1);

	if(clus == 0)
	{
		if(fs_alloc(&clus) == 1)
			goto fs_write_err;

		fs_u16_to_u8(file->dir_entry + 20,
				((clus >> 16) & 0xFFFF));
		fs_u16_to_u8(file->dir_entry + 26, (clus & 0xFFFF));

		if(fs_clr_4k(file->data_buf, &(file->clock_head),
					LOCAL_DATA_BUF_NUM, fs_dataclus2sec(clus)) == 1)
			goto fs_write_err;
	}

	for(i = 0; i < start_clus; i++)
	{
		if(fs_get_fat_entry_val(clus, &next_clus) == 1)
			goto fs_write_err;
		if(next_clus > CountOfClusters + 1)
		{
			if(fs_alloc(&next_clus) == 1)
				goto fs_write_err;
			if(fs_modify_fat(clus, next_clus) == 1)
				goto fs_write_err;

			if(fs_clr_4k(file->data_buf, &(file->clock.head), LOCAL_DATA_BUF_NUM, 
						fs_dataclus2sec(next_clus)) == 1)
				goto fs_write_err;
		}

		clus = next_clus;
	}
	cc = 0;
	while(start_clus <= end_clus)
	{
		index = fs_read_4k(file->data_buf, fs_dataclus2sec(clus), &(file->clock_head), LOCAL_DATA_BUF_NUM);
		if(index == 0xffffffff)
			goto fs_write_err;

		file->data_buf[index].state = 3;
		
		if(start_clus == end_clus)
		{
			for(i = start_byte; i <= end_byte; i++)
				file->data_buf[index].buf[i] = buf[cc++];

			break;
		}

		else
		{
			for(i = start_byte; i < (BPB_SecPerClus << 9); i++)
				file->data_buf[index].buf[i] = buf[cc++];

			start_clus++;
			start_byte = 0;

			if(fs_get_fat_entry_val(clus, &next_clus) == 1)
				goto fs_write_err;

			if(next_clus > CountOfClusters + 1)
			{
				if(fs_alloc(&next_clus) == 1)
					goto fs_write_err;

				if(fs_modify_fat(clus, next_clus) == 1)
					goto fs_write_err;

				if(fs_clr_4k(file->data_buf,
							&(file->clock_head), LOCAL_DATA_BUF_NUM, fs_dataclus2sec(next_clus)) == 1)
					goto fs_write_err;
			}

			clus = next_clus;
		}
	}

	file->loc += count;
	if(filesize != new_filesize)
	{
		fs_u32_to_u8(file->dir_entry + 28, new_filesize);
	}

	return cc;

fs_write_err:
	return 0x0FFFFFFF;
}


//分配新的cluster
u32 fs_alloc(u32 *new_alloc)
{
	u32 clus;
	u32 next_free;

	clus = fs_u8_to_u32(meta_buf + 492) + 1;

	if(clus > fs_u8_to_u32(meta_buf + 488) + 1)
	{
		if(fs_next_free(2, &clus) == 1)
			goto fs_alloc_err;
		if(fs_modify_fat(clus, 0x0FFFFFFF) == 1)
			goto fs_alloc_err;
	}

	if(fs_modify_fat(clus, 0x0FFFFFFF) == 1)
		goto fs_alloc_err;

	if(fs_next_free(clus, &next_free) == 1)
		goto fs_alloc_err;

	if(next_free > CountOfClusters + 1)
		goto fs_alloc_err;

	fs_u32_to_u8(meta_buf + 492, next_free - 1);

	if(fs_write_block(new_alloc_empty,
				fs_dataclus2sec(clus), BPB_SecPerClus) == 1)
		goto fs_alloc_err;

	return 0;
fs_alloc_err:
	return 1;
}

//创建文件
u32 fs_creat(u8 *filename, u8 attr)
{
	u32 i;
	u32 l1, l2;
	u32 empty_entry;
	u32 clus;
	u32 index;
	if(fs_open(&(file_creat, filename) == 0))
		goto fs_creat_err;

	for(i = 255; i >= 0; i--)
		if(file_creat.filename[i] != 0)
		{
			l2 = i;
			break;
		}

	for(i = 255; i >= 0; i--)
		if(file_creat.filename[i] == '/')
		{
			l1 = i;
			break;
		}

	if(l1 = 0)
	{
		for(i = l1; i <= l2; i++)
			file_creat.filename[i] = 0;

		if(fs_find(&file_creat) == 1)
			goto fs_creat_err;

		if(file_creat.dir_entry_pos == 0xFFFFFFFF)
			goto fs_creat_err;

		clus = fs_hilo2clus(file_creat.dir_entry);
		index = fs_read_512(dir_data_buf,
				fs_dataclus2sec(clus), &dir_data_clock_head,
				DIR_DATA_BUF_NUM);
		if(index == 0xffffffff)
			goto fs_creat_err;

		file_creat.dir_entry_pos = clus;
	}
	else
	{
		index = fs_read_512(dir_data_buf, fs_dataclus2sec(2),
				&dir_data_clock_head, DIR_DATA_BUF_NUM);
		if(index == 0xffffffff)
			goto fs_creat_err;

		file_creat.dir_entry_pos = 2;
	}

	index = fs_find_empty_entry(&empty_entry, index);
	if(index == 0xffffffff)
		goto fs_creat_err;

	for(i = l1 + 1; i <= 12; i++)
		file_creat.filename[i - l1 -1] = filename[i];

	file_creat.filename[l2 - l1] = 0;
	fs_next_slash(file_creat.filename);

	dir_data_buf[index].state = 3;

	for(i = 0; i < 11; i++)
		*(dir_data_buf[index].buf + empty_entry + i) =
			filename11[i];

	*(dir_data_buf[index].buf + empty_entry + l1) = attr;

	for(i = l2; i < 32; i++)
		*(dir_data_buf[index].buf + empty_entry + i) = 0;

	if(fs_fflush() == 1)
		goto fs_creat_err;

	return 0;

fs_creat_err:
	return 1;
}
