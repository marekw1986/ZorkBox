
/* Jzip V2.1 Infocom/Inform Zcode Format Interpreter
 * --------------------------------------------------------------------
 * Copyright (c) 2000  John D. Holder.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *       
 * --------------------------------------------------------------------
 */
 
 /*
  * Modified by Louis Davis April, 2012
  * Arduino support.
  *
  */

 /*
  * fileio.c
  *
  * File manipulation routines. Should be generic.
  *
  */
  
//#include <SdFat.h>
//#include <SdFatUtil.h>
//#include "fatfs.h"
#include "ztypes.h"
#include "zork1_dat.h"
#include "string.h"
#include "vga.h"

#define MAX_DYNAMIC_MEMORY	64*1024

/* Static data */

extern int GLOBALVER;

uint16_t dynamic_size = 0;
uint8_t dynamic_memory[MAX_DYNAMIC_MEMORY];

static uint16_t flash_get_word(uint32_t addr);

/*
 * open_story
 *
 * Open game file for read.
 *
 */

//void verify(void)
//{
//    FRESULT res;
//    UINT br;
//    const char game_name[]   = "GAME.DAT";
//
//    res = f_open(&game, game_name, FA_READ);
//    if (res != FR_OK) {
//    	printf("Can't open GAME.DAT for verification");
//    }
//
//    uint8_t failed = 0x00;
//    uint32_t flash_ptr = 0;
//    f_lseek(&game, 0);  // start at beginning
//    uint8_t dat;
//    for (flash_ptr = 0; flash_ptr < zork1_dat_len; flash_ptr++) {
//    	f_read(&game, &dat, 1, &br);
//    	if (dat != zork1_dat[flash_ptr]) {
//    		printf("Mismatch detected at flash_ptr %lu, flash: %d, sd: %d\n", flash_ptr, zork1_dat[flash_ptr], dat);
//    		failed |= 0x01;
////    		break;
//    	}
//    }
//
//    printf("Failed: %d\n", failed);
//
//    f_read(&game, dynamic_memory, dynamic_size, &br);
//    f_close(&game);  // start at beginning
//
//    return;
//}

void open_story(void)
{
    dynamic_size = flash_get_word(0x0E);

    if (dynamic_size > sizeof(dynamic_memory)) {
        printf("Not enough memory for dynamic data\r\n");
        fatal();
    }

    /* Copy dynamic (read-write) segment into RAM */
    memcpy(dynamic_memory, &zork1_dat[0], dynamic_size);
}                             /* open_story */


/*
 * close_story
 *
 * Close game file if open.
 *
 */

void close_story( void )
{

}                               /* close_story */

/*
 * get_story_size
 *
 * Calculate the size of the game file. Only used for very old games that do not
 * have the game file size in the header.
 *
 */

unsigned int get_story_size(void)
{
    return (unsigned int)zork1_dat_len;
}                               /* get_story_size */


/*
 * z_verify
 *
 * Verify game ($verify verb). Add all bytes in game file except for bytes in
 * the game file header.
 *
 */

void z_verify(void)
{
    conditional_jump(TRUE);
}                              /* z_verify */


/*
 * z_save
 *
 * Saves data to disk. Returns:
 *     0 = save failed
 *     1 = save succeeded
 *
 */

int z_save( int argc, zword_t table, zword_t bytes, zword_t name )
{
    int status = 0;

    /* Get the file name */
    status = 1;

    /* Return result of save to Z-code */

    if ( h_type < V4 )
    {
        conditional_jump( status == 0 );
    }
    else
    {
        store_operand( (zword_t)(( status == 0 ) ? 1 : 0) );
    }

    vga_puts("\nSaving not supported in this build.\n");

    return ( status );
}                               /* z_save */


/*
 * z_restore
 *
 * Restore game state from disk. Returns:
 *     0 = restore failed
 *     2 = restore succeeded
 */

int z_restore( int argc, zword_t table, zword_t bytes, zword_t name )
{
    int status;

    status = 1;

    /* Return result of save to Z-code */

    if ( h_type < V4 )
    {
        conditional_jump( status == 0 );
    }
    else
    {
        store_operand( (zword_t)(( status == 0 ) ? 2 : 0) );
    }

    vga_puts("\nSaving not supported in this build.\n");

    return ( status );
}                               /* z_restore */

/*
 * z_save_undo
 *
 * Save the current Z machine state in memory for a future undo. Returns:
 *    -1 = feature unavailable
 *     0 = save failed
 *     1 = save succeeded
 *
 */

void z_save_undo( void )
{
    /* If no memory for data area then say undo is not available */
    store_operand( ( zword_t ) - 1 );

}                               /* z_save_undo */

/*
 * z_restore_undo
 *
 * Restore the current Z machine state from memory. Returns:
 *    -1 = feature unavailable
 *     0 = restore failed
 *     2 = restore succeeded
 *
 */

void z_restore_undo( void )
{
    /* If no memory for data area then say undo is not available */
    store_operand( ( zword_t ) - 1 );

}                               /* z_restore_undo */


/*
* read_code_word
*
* Read a word from the instruction stream.
*
*/

zword_t read_code_word( void )
{
    zword_t w;

    w = ( zword_t ) read_code_byte(  ) << 8;
    w |= ( zword_t ) read_code_byte(  );

    return ( w );

}                               /* read_code_word */

/*
* read_code_byte
*
* Read a byte from the instruction stream.
*
*/

zbyte_t read_code_byte(void)
{
    zbyte_t value = zork1_dat[pc];
    pc++;
    return value;
}                               /* read_code_byte */

/*
* read_data_word
*
* Read a word from the data area.
*
*/

zword_t read_data_word( unsigned long *addr )
{
    zword_t w;

    w = ( zword_t ) read_data_byte( addr ) << 8;
    w |= ( zword_t ) read_data_byte( addr );

    return ( w );

}                               /* read_data_word */

void write_data_word( unsigned long *addr, zword_t value)
{
    write_data_byte(addr, (zbyte_t)(value >> 8));
    write_data_byte(addr, (zbyte_t)(value));

}                               /* write_data_word */

/*
* read_data_byte
*
* Read a byte from the data area.
*
*/

zbyte_t read_data_byte(unsigned long *addr)
{
    zbyte_t value;

    if (*addr < dynamic_size)
        value = dynamic_memory[*addr];
    else
        value = zork1_dat[*addr];

    (*addr)++;
    return value;
}                            /* read_data_byte */

void write_data_byte(unsigned long *addr, zbyte_t value)
{
    if (*addr < dynamic_size)
    {
        dynamic_memory[*addr] = value;
    }

    (*addr)++;
}                              /* write_data_byte */

zbyte_t get_byte(unsigned long offset){ unsigned long addr = offset; return read_data_byte(&addr); }
zword_t get_word(unsigned long offset){ unsigned long addr = offset; return read_data_word(&addr);}
void set_byte(unsigned long offset, zbyte_t value){ unsigned long addr = offset; write_data_byte(&addr, value);}
void set_word(unsigned long offset, zword_t value){ unsigned long addr = offset; write_data_word(&addr, value);}

static uint16_t flash_get_word(uint32_t addr)
{
    return ((uint16_t)zork1_dat[addr] << 8) | zork1_dat[addr + 1];
}

