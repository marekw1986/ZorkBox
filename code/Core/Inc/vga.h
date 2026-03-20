/*
 * vga.h
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#ifndef INC_VGA_H_
#define INC_VGA_H_

void vga_init(void);
void vga_refresh_line_number(void);
void vga_prepare_next_dma(void);

#endif /* INC_VGA_H_ */
