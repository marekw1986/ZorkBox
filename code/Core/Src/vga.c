/*
 * vga.c
 *
 *  Created on: Mar 14, 2026
 *      Author: marek
 */

#include <stdint.h>
#include "main.h"
#include "vga.h"

#define VISIBLE_START	36
#define VISIBLE_END		515

#define SCANLINE_LEN	80

#define VGA_COLS	80
#define VGA_ROWS	30

extern TIM_HandleTypeDef htim2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim4;

char vga_buffer[VGA_COLS * VGA_ROWS];

uint8_t scanline[SCANLINE_LEN];

void fill_scanline(void);

void vga_init(void) {
	fill_scanline();
}

void fill_scanline(void) {
	for(int i=0; i<SCANLINE_LEN; i++) {
		scanline[i] = 0xF0;;
	}
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2 &&
       htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
//        fill_scanline();
    	const uint16_t line = __HAL_TIM_GET_COUNTER(&htim4);
    	if (line >= VISIBLE_START && line < VISIBLE_END) {
    		HAL_SPI_Transmit_DMA(&hspi1, scanline, SCANLINE_LEN);
    	}
    }
}

