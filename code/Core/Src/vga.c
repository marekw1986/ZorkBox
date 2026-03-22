/*
 * vga.c
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#include <stdint.h>
#include "main.h"
#include "vga.h"
#include "vga_font.h"

#define VISIBLE_START	35
#define VISIBLE_END		514

#define SCANLINE_LEN	80

#define VGA_COLS	80
#define VGA_ROWS	30

extern TIM_HandleTypeDef htim2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim4;
extern DMA_HandleTypeDef hdma_spi1_tx;

char vga_buffer[VGA_COLS * VGA_ROWS];

const uint8_t null_byte = 0x00;
volatile uint16_t line = 0;
volatile uint8_t vFlag = 0x00;
uint8_t scanline[SCANLINE_LEN+1];

void fill_scanline(void);

void vga_init(void) {
	fill_scanline();

//    // Make sure SPI DMA is enabled
//    SPI1->CR2 |= SPI_CR2_TXDMAEN;
//
//    // Prepare first transfer
//    hdma_spi1_tx.Instance->CR &= ~DMA_SxCR_EN;
//    while (hdma_spi1_tx.Instance->CR & DMA_SxCR_EN) {}
//
//    hdma_spi1_tx.Instance->M0AR = (uint32_t)scanline;
//    hdma_spi1_tx.Instance->NDTR = SCANLINE_LEN + 1;
//
//    SPI1->CR1 |= SPI_CR1_SPE;   // ensure SPI enabled
//    SPI1->DR = 0x00;
}

inline void vga_refresh_line_number(void) {
	line = __HAL_TIM_GET_COUNTER(&htim4);
	vFlag = (line < VISIBLE_END);
}

inline void fill_scanline(void) {
//	for(int i=0; i<SCANLINE_LEN; i++) {
//		scanline[i] = 0xF0;;
//	}
//	scanline[SCANLINE_LEN] = 0x00;
	scanline[0] = 0xF0;
	scanline[1] = 0x00;
}

static inline void fill_scanline2(void) {
	// First determine which line from the buffer we need
	const uint16_t visible_line = line - VISIBLE_START;
	const uint8_t vga_buf_y = visible_line / 16;
	const uint8_t vga_buf_glyph = visible_line % 16;
	const uint8_t* vga_buf_row = (uint8_t*)&vga_buffer[vga_buf_y * VGA_COLS];
	for (int x=0; x<SCANLINE_LEN; x++) {
		const uint8_t current_char = vga_buf_row[x];
		if (current_char < 32 || current_char > 126) {
			scanline[x] = 0x00;
		}
		else {
			const uint8_t* glyph_ptr = fonts[current_char-32];
			scanline[x] = glyph_ptr[vga_buf_glyph];
		}
	}
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2 &&
       htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
//        fill_scanline();
//    	if (line >= VISIBLE_START && line < VISIBLE_END) {
		if (vFlag) {
//    		hdma_spi1_tx.Instance->CR |= DMA_SxCR_EN;
    		HAL_SPI_Transmit_DMA(&hspi1, scanline, 2);
    	}
    }

    if(htim->Instance == TIM4 &&
       htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
    	vFlag = 0x01;
    }
}

//void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
//{
//    if (hspi->Instance == SPI1)
//    {
//        fill_scanline();
//        hdma_spi1_tx.Instance->M0AR = (uint32_t)scanline;
//        hdma_spi1_tx.Instance->NDTR = SCANLINE_LEN + 1;
//    }
//}

void vga_prepare_next_dma(void) {
    fill_scanline();
    hdma_spi1_tx.Instance->M0AR = (uint32_t)scanline;
    hdma_spi1_tx.Instance->NDTR = SCANLINE_LEN + 1;
}
