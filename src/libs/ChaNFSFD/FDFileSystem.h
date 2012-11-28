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

#ifndef MBED_FDFILESYSTEM_H
#define MBED_FDFILESYSTEM_H

#include "mbed.h"
#include "FATFileSystem.h"
#include "disk.h"

/** Access the filesystem on internal flash - only erase and copy to, FAT is stored in eeprom
 *
 * @code
 * #include "mbed.h"
 * #include "FDFileSystem.h"
 *
 * FDFileSystem sd(p9, p10, "fd"); // sda, scl
 *  
 * int main() {
 *     FILE *fp = fopen("/fd/myfile.txt", "w");
 *     fprintf(fp, "Hello World!\n");
 *     fclose(fp);
 * }
 */
class FDFileSystem : public FATFileSystem {
public:

    /** Create the File System for accessing internal Flash
     *
     * @param sda I2C sda pin connected to eeprom
     * @param scl I2C scl pin conencted to eeprom
     * @param name The name used to access the virtual filesystem
     */
    FDFileSystem(PinName sda, PinName scl, const char* name);

    virtual int disk_initialize();
    virtual int disk_write(const char *buffer, int block_number);
    virtual int disk_read(char *buffer, int block_number);    
    virtual int disk_status();
    virtual int disk_sync();
    virtual int disk_sectors();

protected:

	bool _erase_sector(unsigned start_sector,unsigned end_sector,unsigned cclk);
	bool _prepare_sector(unsigned start_sector,unsigned end_sector,unsigned cclk);
	bool _write_data(unsigned flash_address,unsigned * flash_data_buf,unsigned cclk);
	void _iap_entry(unsigned param_tab[],unsigned result_tab[]);
	
    int _sectors;
    bool _user_flash_erased;
    
    I2C _i2c;
 
	/* FAT12 Boot sector constants */
	unsigned char BootSect[BOOT_SECT_SIZE];

	/* FAT12 Root directory entry constants */
	unsigned char RootDirEntry[DIR_ENTRY];

	/* RAM to store the file allocation table */
	unsigned char  Fat_RootDir[FAT_SIZE + ROOT_DIR_SIZE];    

};

#endif
