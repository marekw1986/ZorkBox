/*
 * vga.c
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#include <stdint.h>
#include <string.h>
#include "main.h"
#include "vga.h"
#include "vga_font.h"

#define VISIBLE_START	35
#define VISIBLE_END		514

#define SCANLINE_LEN	80	// TEMP

#define VGA_COLS	80
#define VGA_ROWS	30

char vga_buffer[VGA_COLS * VGA_ROWS];
volatile uint16_t vga_cursor = 0;
volatile uint8_t cursor_timer = 0;

const uint8_t null_byte = 0x00;
volatile uint16_t line = 0;
volatile uint8_t vFlag = 0x00;
volatile uint8_t active_scanline = 0x00;
uint8_t scanline[2][SCANLINE_LEN+1];

void vga_prepare_line_DMA(void);
static void fill_scanline(void);

void vga_init(void) {
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);  // HSYNC (highest)
	HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 1); // SPI DMA
	HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);  // VSYNC

	memset(vga_buffer, ' ', sizeof(vga_buffer));
	fill_scanline();

	SET_BIT(SPI1->CR1, SPI_CR1_SPE);

	/* disable transfer error interrupt */
	DMA2_Stream2->CR &= ~DMA_SxCR_TEIE;

    /* Configure DMA Stream destination address */
	DMA2_Stream2->PAR = (uint32_t)&SPI1->DR;
    /* Configure DMA Stream source address */
	DMA2_Stream2->M0AR = (uint32_t)&scanline[active_scanline];
	const uint16_t size = SCANLINE_LEN + 1;
	DMA2_Stream2->NDTR = size;
}

void vga_putc(const char c) {
	switch (c) {
		case '\r':
		{
			uint16_t row = vga_cursor / VGA_COLS;
			vga_cursor = row * VGA_COLS;
		}
		break;

		case '\n':
		vga_cursor = ((vga_cursor / VGA_COLS) + 1) * VGA_COLS;
		break;

		default:
		if (c < 32 || c > 126) break;
		while (vFlag);
		vga_buffer[vga_cursor] = c;
		vga_cursor++;
		if (vga_cursor >= VGA_COLS * VGA_ROWS) {
		    // scroll up
		    memmove(vga_buffer, vga_buffer + VGA_COLS, VGA_COLS * (VGA_ROWS - 1));
		    // clear last line
		    memset(vga_buffer + VGA_COLS * (VGA_ROWS - 1), ' ', VGA_COLS);
		    vga_cursor = VGA_COLS * (VGA_ROWS - 1);
		}
		break;
	}
}

inline static void fill_scanline(void) {
	// First determine which line from the buffer we need
	const uint8_t vga_buf_y = line / 16;
	const uint8_t vga_buf_glyph = line % 16;
	const uint8_t* vga_buf_row = (uint8_t*)&vga_buffer[vga_buf_y * VGA_COLS];
	uint8_t* dst = scanline[active_scanline];
	for (int x=0; x<SCANLINE_LEN; x++) {
		const uint8_t current_char = vga_buf_row[x];
		if (current_char < 32 || current_char > 126) {
		   dst[x] = 0x00;
		}
	   else {
		   const uint8_t* glyph_ptr = fonts[current_char-32];
		   dst[x] = glyph_ptr[vga_buf_glyph];
	   }
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM2->SR & TIM_SR_CC2IF)
	{
	    TIM2->SR &= ~TIM_SR_CC2IF;
		if (vFlag) {
		    /* Enable Common interrupts*/
			DMA2_Stream2->CR  |= DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_SxCR_EN;
		    /* Enable SPI DMA request */
		    SET_BIT(SPI1->CR2, SPI_CR2_TXDMAEN);

    		active_scanline ^= 1;
    		fill_scanline();
    	}
	}
}

void TIM4_IRQHandler(void)
{
	if (TIM4->SR & TIM_SR_CC3IF) {
		TIM4->SR &= ~TIM_SR_CC3IF;
		vFlag = 0x01;
	}
}

void DMA2_Stream2_IRQHandler(void)
{
	uint32_t isr = DMA2->LISR;
	if ((isr & DMA_LISR_TCIF2) || (isr & DMA_LISR_TEIF2)) {
		/* clear transfer error and transfer complete flags */
		DMA2->LIFCR = DMA_LIFCR_CTEIF2 | DMA_LIFCR_CTCIF2;

	//	DMA2_Stream2->CR  &= ~(DMA_IT_TC);

	//	CLEAR_BIT(SPI1->CR2, SPI_CR2_ERRIE);
		CLEAR_BIT(SPI1->CR2, SPI_CR2_TXDMAEN);

		DMA2_Stream2->CR &= ~DMA_SxCR_EN;
		while (DMA2_Stream2->CR & DMA_SxCR_EN);

		/* Configure DMA Stream source address */
		DMA2_Stream2->M0AR = (uint32_t)&scanline[active_scanline];
		const uint16_t size = SCANLINE_LEN + 1;
		DMA2_Stream2->NDTR = size;

		line++;
		if (line > 480) {
			line = vFlag = 0;
			cursor_timer++;
			if (cursor_timer > 30) {
				if (vga_buffer[vga_cursor] == ' ') {
					vga_buffer[vga_cursor] = '_';
				}
				else {
					vga_buffer[vga_cursor] = ' ';
				}
			  cursor_timer = 0;
			}
		}
	}
}
