//512 byte buffer
typedef struct buf_512
{
	unsigned char buf[512];
	unsigned long cur;
	unsigned long state;
}BUF_512;



//file struct
typedef struct fs_fat_file
{
	unsigned char filename[256];//存储文件的完整路径
	unsigned long loc;//文件位置指针，用来记录当前读写到的位置距离文件头部的字节数，用于支持文件的随机读写
	unsigned long dir_entry_pos;//目录信息
	unsigned long dir_entry_sector;
	unsigned char dir_entry[32];//这个目录项本身的内容，用于获取文件的各种信息
	unsigned unsigned long clock_head;
	BUF_4K data_buf[LOCAL_DATA_BUF_NUM];//用于缓存机制
}FS_FAT_FILE;
//缓存算法用于当缓冲区满时，选取一个缓存块作为vicitm，并将其内容同步到磁盘里
