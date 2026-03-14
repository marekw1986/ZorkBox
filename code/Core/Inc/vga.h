/*
 * vga.h
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#ifndef INC_VGA_H_
#define INC_VGA_H_

enum {VGA_COLOR_BLACK = 0, VGA_COLOR_WHITE, VGA_COLOR_RED, VGA_COLOR_GREEN, VGA_COLOR_BLUE};

typedef struct {
	char c;
	uint8_t attr;
} vga_char_t;

#endif /* INC_VGA_H_ */
