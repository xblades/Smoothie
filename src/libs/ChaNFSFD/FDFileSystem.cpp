/* mbed SDFileSystem Library, for providing file access to SD cards
 * Copyright (c) 2008-2010, sford
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Introduction
 * ------------
 * SD and MMC cards support a number of interfaces, but common to them all
 * is one based on SPI. This is the one I'm implmenting because it means
 * it is much more portable even though not so performant, and we already 
 * have the mbed SPI Interface!
 *
 * The main reference I'm using is Chapter 7, "SPI Mode" of: 
 *  http://www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf
 *
 * SPI Startup
 * -----------
 * The SD card powers up in SD mode. The SPI interface mode is selected by
 * asserting CS low and sending the reset command (CMD0). The card will 
 * respond with a (R1) response.
 *
 * CMD8 is optionally sent to determine the voltage range supported, and 
 * indirectly determine whether it is a version 1.x SD/non-SD card or 
 * version 2.x. I'll just ignore this for now.
 *
 * ACMD41 is repeatedly issued to initialise the card, until "in idle"
 * (bit 0) of the R1 response goes to '0', indicating it is initialised.
 *
 * You should also indicate whether the host supports High Capicity cards,
 * and check whether the card is high capacity - i'll also ignore this
 *
 * SPI Protocol
 * ------------
 * The SD SPI protocol is based on transactions made up of 8-bit words, with
 * the host starting every bus transaction by asserting the CS signal low. The
 * card always responds to commands, data blocks and errors.
 * 
 * The protocol supports a CRC, but by default it is off (except for the 
 * first reset CMD0, where the CRC can just be pre-calculated, and CMD8)
 * I'll leave the CRC off I think! 
 * 
 * Standard capacity cards have variable data block sizes, whereas High 
 * Capacity cards fix the size of data block to 512 bytes. I'll therefore
 * just always use the Standard Capacity cards with a block size of 512 bytes.
 * This is set with CMD16.
 *
 * You can read and write single blocks (CMD17, CMD25) or multiple blocks 
 * (CMD18, CMD25). For simplicity, I'll just use single block accesses. When
 * the card gets a read command, it responds with a response token, and then 
 * a data token or an error.
 * 
 * SPI Command Format
 * ------------------
 * Commands are 6-bytes long, containing the command, 32-bit argument, and CRC.
 *
 * +---------------+------------+------------+-----------+----------+--------------+
 * | 01 | cmd[5:0] | arg[31:24] | arg[23:16] | arg[15:8] | arg[7:0] | crc[6:0] | 1 |
 * +---------------+------------+------------+-----------+----------+--------------+
 *
 * As I'm not using CRC, I can fix that byte to what is needed for CMD0 (0x95)
 *
 * All Application Specific commands shall be preceded with APP_CMD (CMD55).
 *
 * SPI Response Format
 * -------------------
 * The main response format (R1) is a status byte (normally zero). Key flags:
 *  idle - 1 if the card is in an idle state/initialising 
 *  cmd  - 1 if an illegal command code was detected
 *
 *    +-------------------------------------------------+
 * R1 | 0 | arg | addr | seq | crc | cmd | erase | idle |
 *    +-------------------------------------------------+
 *
 * R1b is the same, except it is followed by a busy signal (zeros) until
 * the first non-zero byte when it is ready again.
 *
 * Data Response Token
 * -------------------
 * Every data block written to the card is acknowledged by a byte 
 * response token
 *
 * +----------------------+
 * | xxx | 0 | status | 1 |
 * +----------------------+
 *              010 - OK!
 *              101 - CRC Error
 *              110 - Write Error
 *
 * Single Block Read and Write
 * ---------------------------
 *
 * Block transfers have a byte header, followed by the data, followed
 * by a 16-bit CRC. In our case, the data will always be 512 bytes.
 *  
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 * | 0xFE | data[0] | data[1] |        | data[n] | crc[15:8] | crc[7:0] | 
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 */
 
#include "FDFileSystem.h"
#include "disk.h"
#include "sbl_config.h"

FDFileSystem::FDFileSystem(PinName sda, PinName scl,  const char* name) :
  FATFileSystem(name), _i2c(sda, scl), BootSect({0xEB,0x3C,0x90,0x4D,0x53,0x44,0x4F,0x53,0x35,0x2E,0x30,0x00,0x02,BLOCKS_PER_CLUSTER,0x01,0x00,0x01,0x10,0x00,0xEC,0x03,0xF8,0x02,0x00,0x01,0x00,0x01,0x00,0x00,0x00}), 
	RootDirEntry({'R', 'D', 'B', '1', '7', '6', '9',' ', ' ', ' ', ' ', 0x28,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 		'D', 'E', 'L', 'E', 'T', 'E', ' ', ' ', 'M', 'E', ' ',0x20,0x18,0xbc,0x41,0x97,
		0x37,0x38,0x37,0x38,0x00,0x00,0x3d,0x6e,0x2b,0x38,0x02,0x00,0x00,0xD0,0x07,0x00})
{
//	printf("FD init\r\n");
}

int FDFileSystem::disk_initialize() {
    char buf[2];
	unsigned int n, m , next_cluster;
	char * firmware;
	firmware = (char *)USER_FLASH_START;

    buf[0]=0x00;
    buf[1]=0x00;

    _i2c.frequency(400000);
    _i2c.write(0xA0, (char *)&buf,2);
    _i2c.read(0xA0, (char*)&Fat_RootDir, FAT_SIZE + ROOT_DIR_SIZE);
    _sectors = 1024 - (sector_start_map[USER_START_SECTOR] >> 9); // div 512)
    _user_flash_erased = false;
    
	bool initialized = true;
	bool empty = true;
	// check initialization
	for (n = 0; n < 10 ; n++) {             /* Copy Initial Disk Image */
//		printf("FD FAT %d \r\n",Fat_RootDir[(FAT_SIZE+n)]);
		if (Fat_RootDir[(FAT_SIZE+n)]!= RootDirEntry[n]) {
			initialized = false;
		}
	}

//	initialized = false;
//	printf("check pin %d \r\n",((*(volatile unsigned *)ISP_ENTRY_GPIO_REG) & (0x1<<ISP_ENTRY_PIN)) );
	for (n = 0; n < USER_FLASH_SIZE ; n++) {
		if (*firmware!=0xFF) empty = false;
	}

	if ((!initialized) || (empty) || (!((*(volatile unsigned *)ISP_ENTRY_GPIO_REG) & (0x1<<ISP_ENTRY_PIN) )))
	{
//		printf("FD generate FAT \r\n");
		// Generate File Allocation Table to save Flash space
		// First Two FAT entries are reserved
		for (n=0; n<FAT_SIZE + ROOT_DIR_SIZE; n++) {
			Fat_RootDir[n]= 0xFF;
		}
		Fat_RootDir[0]= 0xF8;
		Fat_RootDir[1]= 0xFF;
		Fat_RootDir[2]= 0xFF;
		/* Start cluster of a file is indicated by the Directory entry = 2 */
		m = 3;
		for ( n = 3;n < NO_OF_CLUSTERS+2;n+=2) {
			if( n == ((NO_OF_CLUSTERS+2)-1) )
			{
			  next_cluster = 0xFFF;
			}
			else
			{
			  next_cluster = n + 1;
			}
		  	Fat_RootDir[m] = (BYTE)n & 0xFF;
		  	Fat_RootDir[m+1] = (((BYTE)next_cluster & 0xF) << 4) | ((BYTE)(n>>8)&0xF);
		  	Fat_RootDir[m+2] = (BYTE)(next_cluster >> 4) & 0xFF;
			m = m+3;
		}
	
		/* Copy root directory entries */
		for (n = 0; n < DIR_ENTRY ; n++) {             /* Copy Initial Disk Image */
			Fat_RootDir[(FAT_SIZE+n)] = RootDirEntry[n];  /*   from Flash to RAM     */
		}
	
		/* Correct file size entry for file firmware.bin */
		Fat_RootDir[FAT_SIZE+60] = (BYTE)(USER_FLASH_SIZE & 0xFF);
		Fat_RootDir[FAT_SIZE+61] = (BYTE)(USER_FLASH_SIZE >> 8);
		Fat_RootDir[FAT_SIZE+62] = (BYTE)(USER_FLASH_SIZE >> 16);
		Fat_RootDir[FAT_SIZE+63] = (BYTE)(USER_FLASH_SIZE >> 24);
	}

    return 0;
}

int FDFileSystem::disk_write(const char *buffer, int block_number) {
	__disable_irq();
//	printf("FD Write %d\r\n",block_number);

    char buf[34];
	unsigned int i,j;
	unsigned long offset;
	offset = 512* block_number;

    if (offset < BOOT_SECT_SIZE)
	{
	    __enable_irq();
	  /* Can't write boot sector */
	}
	else if (offset < (BOOT_SECT_SIZE + FAT_SIZE + ROOT_DIR_SIZE))
    {
	    for ( i = 0; i<512; i++)
	    {
			Fat_RootDir[(offset - BOOT_SECT_SIZE)+i] = buffer[i];
		    
			if ( buffer[i] == 0xe5 )
			{
				if ( (offset+i) == BOOT_SECT_SIZE + FAT_SIZE + 32 )
				{
					// Delete user flash when firmware.bin is erased
					if( _user_flash_erased == false )
					{
//						printf("FD Erase Flash\r\n");
						// erase flash
					    _prepare_sector(USER_START_SECTOR,MAX_USER_SECTOR,SystemCoreClock/1000);
    					if (!_erase_sector(USER_START_SECTOR,MAX_USER_SECTOR,SystemCoreClock/1000))
    					{
    						// error
    					}
						_user_flash_erased = true;
					}
				}
			}
		}
	    __enable_irq();

	    for ( j = 0; j<48; j++) {
		    buf[0]= (j*32) >> 8;
    		buf[1]= (j*32) & 0xFF;
		    for ( i = 0; i<32; i++){
	    		buf[2+i]= Fat_RootDir[j*32+i];
			}
		    _i2c.write(0xA0,(char *)&buf,34);
		    _i2c.stop();
		    wait_ms(5);
		}				

/*	    for ( j = 0; j<16; j++) {
			uiAdress = (0x0000 + (offset- BOOT_SECT_SIZE) +j*32);
		    buf[0]= uiAdress >>8;
    		buf[1]= uiAdress & 0xFF;
		    for ( i = 0; i<32; i++){
	    		buf[2+i]= buffer[j*32+i];
			}
		    _i2c.write(0xA0,(char *)&buf,34);
		    _i2c.stop();
		    wait_ms(5);
		}		
*/
	}
	else
	{
		offset += USER_FLASH_START;
		offset -= BOOT_SECT_SIZE;
		offset -= FAT_SIZE;
		offset -= ROOT_DIR_SIZE;
		if (!_write_data((unsigned)offset,(unsigned *)buffer,SystemCoreClock/1000))
		{
			// error
		}
	    __enable_irq();
	}
    return 0;    
}

int FDFileSystem::disk_read(char *buffer, int block_number) {        
//	printf("FD Read %d\r\n",block_number);
	unsigned int i;
	unsigned long offset;
	__disable_irq();
	offset = 512* block_number;
	
	char * firmware;
	firmware = (char *)USER_FLASH_START;

	for ( i = 0; i<512; i++)
	{
		if (offset < BOOT_SECT_SIZE)
		{
			switch (offset)
			{
				case 19:
					buffer[i] = (BYTE)(MSC_BlockCount & 0xFF);
					break;
				case 20:
					buffer[i] = (BYTE)((MSC_BlockCount >> 8) & 0xFF);
					break;
				case 54:
					buffer[i] = 0x46;
					break;
				case 55:
					buffer[i] = 0x41;
					break;
				case 56:
					buffer[i] = 0x54;
					break;
				case 510:
					buffer[i] = 0x55;
					break;
				case 511:
					buffer[i] = 0xAA;
					break;
				default:
					if ( offset > 29 )
					{
						buffer[i] = 0x0;
					} else
					{
						buffer[i] = BootSect[offset];
					}
					break;
			}
		}
		else if (offset < (BOOT_SECT_SIZE + FAT_SIZE + ROOT_DIR_SIZE))
		{
			buffer[i] = Fat_RootDir[offset - BOOT_SECT_SIZE];
		} else {
			buffer[i] = *(firmware + (offset - (BOOT_SECT_SIZE + FAT_SIZE + ROOT_DIR_SIZE)));
		}
//	printf("%4d",buffer[i]);
		offset++;
	}
	__enable_irq();
//	printf("\r\n");
	return 0;
}

int FDFileSystem::disk_status() { return 0; }
int FDFileSystem::disk_sync() { return 0; }
int FDFileSystem::disk_sectors() { return _sectors; }

// PRIVATE FUNCTIONS

const unsigned sector_start_map[MAX_FLASH_SECTOR] = {SECTOR_0_START,             \
SECTOR_1_START,SECTOR_2_START,SECTOR_3_START,SECTOR_4_START,SECTOR_5_START,      \
SECTOR_6_START,SECTOR_7_START,SECTOR_8_START,SECTOR_9_START,SECTOR_10_START,     \
SECTOR_11_START,SECTOR_12_START,SECTOR_13_START,SECTOR_14_START,SECTOR_15_START, \
SECTOR_16_START,SECTOR_17_START,SECTOR_18_START,SECTOR_19_START,SECTOR_20_START, \
SECTOR_21_START,SECTOR_22_START,SECTOR_23_START,SECTOR_24_START,SECTOR_25_START, \
SECTOR_26_START,SECTOR_27_START,SECTOR_28_START,SECTOR_29_START					 };

const unsigned sector_end_map[MAX_FLASH_SECTOR] = {SECTOR_0_END,SECTOR_1_END,    \
SECTOR_2_END,SECTOR_3_END,SECTOR_4_END,SECTOR_5_END,SECTOR_6_END,SECTOR_7_END,   \
SECTOR_8_END,SECTOR_9_END,SECTOR_10_END,SECTOR_11_END,SECTOR_12_END,             \
SECTOR_13_END,SECTOR_14_END,SECTOR_15_END,SECTOR_16_END,SECTOR_17_END,           \
SECTOR_18_END,SECTOR_19_END,SECTOR_20_END,SECTOR_21_END,SECTOR_22_END,           \
SECTOR_23_END,SECTOR_24_END,SECTOR_25_END,SECTOR_26_END,                         \
SECTOR_27_END,SECTOR_28_END,SECTOR_29_END										 };

unsigned param_table[5];
unsigned result_table[5];

void FDFileSystem::_iap_entry(unsigned param_tab[],unsigned result_tab[])
{
    void (*iap)(unsigned [],unsigned []);

    iap = (void (*)(unsigned [],unsigned []))IAP_ADDRESS;
    iap(param_tab,result_tab);
}

bool FDFileSystem::_erase_sector(unsigned start_sector,unsigned end_sector,unsigned cclk)
{
	param_table[0] = 52;	//ERASE_SECTOR
	param_table[1] = start_sector;
	param_table[2] = end_sector;
	param_table[3] = cclk;
	_iap_entry(param_table,result_table);
	return (result_table[0] == CMD_SUCCESS);
}

bool FDFileSystem::_prepare_sector(unsigned start_sector,unsigned end_sector,unsigned cclk)
{
	param_table[0] = 50;	//PREPARE_SECTOR_FOR_WRITE
	param_table[1] = start_sector;
	param_table[2] = end_sector;
	param_table[3] = cclk;
	_iap_entry(param_table,result_table);
	return (result_table[0] == CMD_SUCCESS);
}

bool FDFileSystem::_write_data(unsigned flash_address,unsigned * flash_data_buf,unsigned cclk)
{
//	printf("FD Adress %d\r\n",flash_address);
	
	unsigned i;
	for(i=USER_START_SECTOR;i<=MAX_USER_SECTOR;i++)
	{
		if(flash_address < sector_end_map[i])
		{
			_prepare_sector(i,i,cclk);
		}
	}

    param_table[0] = 51;	//COPY_RAM_TO_FLASH
    param_table[1] = flash_address;
    param_table[2] = (unsigned)flash_data_buf;
    param_table[3] = 512;
    param_table[4] = cclk;
    _iap_entry(param_table,result_table);
//    wait_ms(5);
	return (result_table[0] == CMD_SUCCESS);
}

