//实现spi通信
unsigned char sd_spi_comm(unsigned char data)
{
	volatile unsigned int *SPI_ADDR = (unsigned int *)(0xFFFF0500);
	
	while((SPI_ADDR[1] & 65535) == 0)
		SPI_ADDR[3] = data;

	while(((SPI_ADDR[1] >> 16) & 65535) == 0)
		return SPI_ADDR[3];
}

//指令处理
//根据index和args生成指令
void sd_calc_cmd(unsigned char *cmd, unsigned char index, unsigned int arg)
{
	cmd[0] = index | 0x40;
	cmd[1] = (arg >> 24) & 0xff;
	cmd[2] = (arg >> 16) & 0xff;
	cmd[3] = (arg >> 8) & 0xff;
	cmd[4] = arg & 0xff;
	cmd[5] = sd_crc7(cmd) | 1;//用于计算指令的CRC7校验码
}

//发送指令
void sd_cmd(unsigned char *cmd, unsigned char *resp, unsigned int resp_count)
{
	unsigned int i;

	sd_spi_comm(0xff);

	//every command has 6 bytes
	for(i = 0; i < 6; i++)
		sd_spi_comm(cmd[i]);

	//get the response
	i = 0;
	while(i < resp_count)
	{
		resp[i] = sd_spi_comm(0xff);
		if(resp[i] != 0xff)
			i++;
	}
}

void sd_acmd(unsigned char *cmd, unsigned char *resp, unsigned int resp_count)
{
	sd_cmd(cmd55, resp, 1);
	sd_cmd(cmd, resp, resp_count);
}

//初始化sd
u32 sd_init()
{
	volatile u32 *SPI_ADDR = (u32 *)(0xFFFF0500);
	u32 i;

	//Initialize SPI
	SPI_ADDR[2] = 0x00010001;
	
	//send cmd0
	sd_cmd(cmd0, resp, 1);
	if(resp[0] != 0x01)
		goto sd_init_err;

	//send cmd8
	sd_cmd(cmd8, resp, 5);
	if(resp[0] != 0x01 | resp[3] != 0x01 | resp[4] != 0x01)
		goto sd_init_err;

	//send acmd41
	for(i = 0; i < MAX_TRY && resp[0] != 0x00; i++)
		sd_acmd(acmd41, resp, 1);

	if(resp[0] != 0x00)
		goto sd_init_err;

	return 0;

sd_init_err:
	return 1;
}

//sd卡的读操作有两种不同的指令，分别用于读取单个块与读取多个块

//read single block
u32 sd_read_single(u8 *buf, u32 addr)
{
	u32 i;
	u16 crc16 = 0;

	sd_calc_cmd(cmd, 17, addr);
	sd_cmd(cmd, resp, 1);
	if(resp[0] != 0x00)
		goto sd_read_single_err;

	//wait for start token
	while(1)
	{
		resp[0] = sd_spi_comm(0xff);

		//start token
		if(resp[0] == START_TOKEN_SINGLE)
			break;

		//data error token
		if(((resp[0] >> 4) & 0x07) == 0x00)
			goto sd_read_single_err;
	}

	//read data
	for(i = 0; i < 512; i++)
		buf[i] = sd_spi_comm(0xff);

	//read crc16
	for(i = 0; i < 2; i++)
		crc = (crc16 << 8) + sd_spi_comm(0xff);

	//check crc
	if(crc16 != sd_crc16(buf))
		goto sd_read_single_err;

	return 0;

sd_read_single_err:
	return 1;
}

//read multiple block
u32 sd_read_multiple(u8 *buf, u32 addr, u32 count)
{
	u32 i, c;
	u16 crc16;

	//read cmd18
	sd_calc_cmd(cmd, 18, addr);
	sd_cmd(cmd, resp, 1);
	if(resp[0] != 0x00)
		goto sd_read_multiple_err;

	for(c = 0; c < count; c++)
	{
	//wait for start token
	while(1)
	{
		resp[0] = sd_spi_comm(0xff);

		//start token
		if(resp[0] == START_TOKEN_SINGLE)
			break;

		//data error token
		if(((resp[0] >> 4) & 0x07) == 0x00)
			goto sd_read_multiple_err;
	}

	//read data
	for(i = 0; i < 512; i++)
		buf[(c << 9) + i] = sd_spi_comm(0xff);
	
	crc16 = 0;
	//read crc16
	for(i = 0; i < 2; i++)
		crc = (crc16 << 8) + sd_spi_comm(0xff);

	//check crc
	if(crc16 != sd_crc16(buf))
		goto sd_read_multiple_err;

	//send cmd12
	sd_cmd(cmd12, resp, 1);
	while(sd_spi_comm(0xff) == 0x00);

	return 0;

sd_read_multiple_err:
	//send cmd12
	sd_cmd(cmd12, resp, 1);

	//wait for busy
	while(sd_spi_comm(0xff) == 0x00);

	return 1;
	}
}

//实现写操作
//发送完了cmd24后，重新首先应该发送0xfe，告诉sd卡开始传输
//write single block
u32 sd_write_single(u8 *buf, u32 *addr)
{
	u32 i;
	u16 crc16;

	//send cmd24
	sd_calc_cmd(cmd, 24, addr);
	sd_cmd(cmd, resp, 1);
	if(resp[0] != 0x00)
		goto sd_write_single_err;

	sd_spi_comm(START_TOKEN_SINGLE);
	for(i = 0; i < 512; i++)
		sd_spi_comm(buf[i]);

	//calculate crc16 and send
	crc16 = sd_crc16(buf);
	sd_spi_comm((crc >> 8) & 0xff);
	sd_spi_comm(crc16 & 0xff);

	//check data response
	resp[0] = sd_spi_comm(0xff);

	if((resp[0] & 0x07) != 0x05)
		goto sd_write_single_err;

	//wait for busy
	while(sd_spi_comm(0xff) == 0x00);

	return 0;

sd_write_single_err:
	return 1;
}
