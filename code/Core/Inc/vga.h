/*
 * vga.h
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#ifndef INC_VGA_H_
#define INC_VGA_H_

extern volatile uint8_t vFlag;
extern volatile uint8_t active_scanline;

void vga_init(void);
void vga_putc(const char c);
void vga_end_line_callback(void);
void vga_start_line_DMA(void);
void fill_scanline(void);

static inline void vga_handle_hsync(void) {
	if (TIM2->SR & TIM_SR_CC2IF)
	{
	    TIM2->SR &= ~TIM_SR_CC2IF;  // clear flag ASAP
		if (vFlag) {
			vga_start_line_DMA();
    		active_scanline ^= 1;
    		fill_scanline();
    	}
	}
}

static inline void vga_handle_vsync(void) {
	if (TIM4->SR & TIM_SR_CC3IF) {
		TIM4->SR &= ~TIM_SR_CC3IF;
		vFlag = 0x01;
	}
}

#endif /* INC_VGA_H_ */
