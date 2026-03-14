/*
 * vga.c
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#include <stdint.h>
#include "vga.h"

#define SCANLINE_LEN	640

#define VGA_COLS	80
#define VGA_ROWS	30

const uint32_t vga_colors[] = {
	0b111111 << 3,		// WHITE
	0x0000,				// BLACK
	0b000011 << 3,		// RED
	0b001100 << 3,		// GREEN
	0b110000 << 3		// BLUE
};

vga_char_t vga_buffer[VGA_COLS * VGA_ROWS];

uint16_t scanline[SCANLINE_LEN];

void fill_scanline(void) {
	for(int i=0; i<SCANLINE_LEN; i++) {
		scanline[i] = 0xFFFF;
	}
}

