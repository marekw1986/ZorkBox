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

#define SCANLINE_LEN	70	// TEMP

#define VGA_COLS	80
#define VGA_ROWS	30

extern TIM_HandleTypeDef htim2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim4;
extern DMA_HandleTypeDef hdma_spi1_tx;

char vga_buffer[VGA_COLS * VGA_ROWS];
uint16_t vga_cursor = 0;
uint8_t cursor_timer = 0;

const uint8_t null_byte = 0x00;
volatile uint16_t line = 0;
volatile uint8_t vFlag = 0x00;
volatile uint8_t active_scanline = 0x00;
uint8_t scanline[2][SCANLINE_LEN+1];

static void fill_scanline(void);

HAL_StatusTypeDef vga_transmit_line_DMA(void);
void vga_dma_complete_callback(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef vga_prepare_line_DMA(void);
void vga_start_line_DMA(void);

void vga_init(void) {
	memset(vga_buffer, ' ', sizeof(vga_buffer));
	fill_scanline();
	vga_prepare_line_DMA();

//	hdma_spi1_tx.XferCpltCallback = vga_dma_complete_callback;
//
//	hdma_spi1_tx.Instance->CR &= ~DMA_SxCR_EN;
//	while (hdma_spi1_tx.Instance->CR & DMA_SxCR_EN);
//
//    hdma_spi1_tx.Instance->M0AR = (uint32_t)scanline;
//    hdma_spi1_tx.Instance->NDTR = SCANLINE_LEN+1;
//
//	hdma_spi1_tx.Instance->CR |= DMA_SxCR_EN;
	__HAL_SPI_ENABLE(&hspi1);
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

static void fill_scanline(void) {
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

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2 &&
       htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
//        fill_scanline();
//    	if (line >= VISIBLE_START && line < VISIBLE_END) {
		if (vFlag) {
//    		HAL_SPI_Transmit_DMA(&hspi1, scanline[active_scanline], SCANLINE_LEN+1);
			//vga_transmit_line_DMA();
			vga_start_line_DMA();
    		active_scanline ^= 1;
    		fill_scanline();
//			hdma_spi1_tx.Instance->CR |= DMA_SxCR_EN;

    	}
    }

    if(htim->Instance == TIM4 &&
       htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
    	vFlag = 0x01;
    }
}

HAL_StatusTypeDef vga_transmit_line_DMA(void)
{
	vga_prepare_line_DMA();
	vga_start_line_DMA();
	return HAL_OK;
}

HAL_StatusTypeDef vga_prepare_line_DMA(void)
{
//    const uint16_t size = SCANLINE_LEN + 1;

//    if (hspi1.State != HAL_SPI_STATE_READY)
//        return HAL_BUSY;

//    __HAL_LOCK(&hspi1);

//    hspi1.State       = HAL_SPI_STATE_BUSY_TX;
//    hspi1.ErrorCode   = HAL_SPI_ERROR_NONE;
//    hspi1.pTxBuffPtr  = (const uint8_t *)scanline[active_scanline];
//    hspi1.TxXferSize  = size;
//    hspi1.TxXferCount = size;

//    __HAL_SPI_DISABLE(&hspi1);
//    SPI_1LINE_TX(&hspi1);

//    __HAL_UNLOCK(&hspi1);

    return HAL_OK;
}

void vga_start_line_DMA(void)
{
	const uint16_t size = SCANLINE_LEN + 1;

//	HAL_DMA_Start_IT(
//		&hdma_spi1_tx,
//		(uint32_t)scanline[active_scanline],
//		(uint32_t)&hspi1.Instance->DR,
//		size);


    /* Configure the source, destination address and the data length */
//    DMA_SetConfig(hdma, SrcAddress, DstAddress, DataLength);

	  /* Clear DBM bit */
	hdma_spi1_tx.Instance->CR &= (uint32_t)(~DMA_SxCR_DBM);

	  /* Configure DMA Stream data length */
	hdma_spi1_tx.Instance->NDTR = size;

    /* Configure DMA Stream destination address */
	hdma_spi1_tx.Instance->PAR = (uint32_t)&hspi1.Instance->DR;

    /* Configure DMA Stream source address */
	hdma_spi1_tx.Instance->M0AR = (uint32_t)scanline[active_scanline];

//    DMA_Base_Registers *regs = (DMA_Base_Registers *)hdma_spi1_tx.StreamBaseAddress;
    /* Clear all interrupt flags at correct offset within the register */
//	hdma_spi1_tx.StreamBaseAddress.IFCR = 0x3FU << hdma_spi1_tx.StreamIndex;

    /* Enable Common interrupts*/
    hdma_spi1_tx.Instance->CR  |= DMA_IT_TC | DMA_IT_TE | DMA_IT_DME;

    /* Enable the Peripheral */
    __HAL_DMA_ENABLE(&hdma_spi1_tx);



    /* Enable SPI DMA request */
    SET_BIT(hspi1.Instance->CR2, SPI_CR2_TXDMAEN);

    /* Enable error interrupt */
//    __HAL_SPI_ENABLE_IT(&hspi1, SPI_IT_ERR);
}

inline void vga_end_line_callback(void) {

	/* Disable ERR interrupt */
	__HAL_SPI_DISABLE_IT(&hspi1, SPI_IT_ERR);

	/* Disable Tx DMA Request */
	CLEAR_BIT(hspi1.Instance->CR2, SPI_CR2_TXDMAEN);

	hspi1.TxXferCount = 0U;
	hspi1.State = HAL_SPI_STATE_READY;

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
	vga_prepare_line_DMA();
}
