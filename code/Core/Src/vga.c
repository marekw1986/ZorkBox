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

#define SCANLINE_LEN	60	// TEMP

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
static void vga_dma_error(DMA_HandleTypeDef *hdma);
static void vga_dma_transmit_cplt(DMA_HandleTypeDef *hdma);

HAL_StatusTypeDef vga_transmit_line_DMA(void);
void vga_dma_complete_callback(DMA_HandleTypeDef *hdma);

void vga_init(void) {
	memset(vga_buffer, ' ', sizeof(vga_buffer));
	fill_scanline();

//	hdma_spi1_tx.XferCpltCallback = vga_dma_complete_callback;
//
//	hdma_spi1_tx.Instance->CR &= ~DMA_SxCR_EN;
//	while (hdma_spi1_tx.Instance->CR & DMA_SxCR_EN);
//
//    hdma_spi1_tx.Instance->M0AR = (uint32_t)scanline;
//    hdma_spi1_tx.Instance->NDTR = SCANLINE_LEN+1;
//
//	hdma_spi1_tx.Instance->CR |= DMA_SxCR_EN;
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
			vga_transmit_line_DMA();
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
  const uint16_t size = SCANLINE_LEN+1;

  /* Check tx dma handle */
//  assert_param(IS_SPI_DMA_HANDLE(hspi1.hdmatx));

  /* Check Direction parameter */
//  assert_param(IS_SPI_DIRECTION_2LINES_OR_1LINE(hspi1.Init.Direction));

  if (hspi1.State != HAL_SPI_STATE_READY)
  {
    return HAL_BUSY;
  }

  /* Process Locked */
  __HAL_LOCK(&hspi1);

  /* Set the transaction information */
  hspi1.State       = HAL_SPI_STATE_BUSY_TX;
  hspi1.ErrorCode   = HAL_SPI_ERROR_NONE;
  hspi1.pTxBuffPtr  = (const uint8_t *)scanline[active_scanline];
  hspi1.TxXferSize  = size;
  hspi1.TxXferCount = size;

  /* Init field not used in handle to zero */
  hspi1.pRxBuffPtr  = (uint8_t *)NULL;
  hspi1.TxISR       = NULL;
  hspi1.RxISR       = NULL;
  hspi1.RxXferSize  = 0U;
  hspi1.RxXferCount = 0U;

  /* Configure communication direction : 1Line */
//  if (hspi1.Init.Direction == SPI_DIRECTION_1LINE)
//  {
    /* Disable SPI Peripheral before set 1Line direction (BIDIOE bit) */
    __HAL_SPI_DISABLE(&hspi1);
    SPI_1LINE_TX(&hspi1);
//  }

  /* Set the SPI TxDMA Half transfer complete callback */
  hspi1.hdmatx->XferHalfCpltCallback = NULL;

  /* Set the SPI TxDMA transfer complete callback */
  hspi1.hdmatx->XferCpltCallback = vga_dma_transmit_cplt;

  /* Set the DMA error callback */
  hspi1.hdmatx->XferErrorCallback = vga_dma_error;

  /* Set the DMA AbortCpltCallback */
  hspi1.hdmatx->XferAbortCallback = NULL;

  /* Enable the Tx DMA Stream/Channel */
  if (HAL_OK != HAL_DMA_Start_IT(hspi1.hdmatx, (uint32_t)hspi1.pTxBuffPtr, (uint32_t)&hspi1.Instance->DR,
		  hspi1.TxXferCount))
  {
    /* Update SPI error code */
    SET_BIT(hspi1.ErrorCode, HAL_SPI_ERROR_DMA);
    /* Process Unlocked */
    __HAL_UNLOCK(&hspi1);
    return HAL_ERROR;
  }

  /* Check if the SPI is already enabled */
  if ((hspi1.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE)
  {
    /* Enable SPI peripheral */
    __HAL_SPI_ENABLE(&hspi1);
  }

  /* Process Unlocked */
  __HAL_UNLOCK(&hspi1);

  /* Enable the SPI Error Interrupt Bit */
  __HAL_SPI_ENABLE_IT(&hspi1, (SPI_IT_ERR));

  /* Enable Tx DMA Request */
  SET_BIT(hspi1.Instance->CR2, SPI_CR2_TXDMAEN);

  return HAL_OK;
}

static void vga_dma_error(DMA_HandleTypeDef *hdma)
{
  SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)(((DMA_HandleTypeDef *)hdma)->Parent);

  /* Stop the disable DMA transfer on SPI side */
  CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN);

  SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_DMA);
  hspi->State = HAL_SPI_STATE_READY;
  /* Call user error callback */
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
  hspi->ErrorCallback(hspi);
#else
  HAL_SPI_ErrorCallback(hspi);
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
}

static void vga_dma_transmit_cplt(DMA_HandleTypeDef *hdma)
{
  SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)(((DMA_HandleTypeDef *)hdma)->Parent);
//  uint32_t tickstart;

  /* Init tickstart for timeout management*/
//  tickstart = HAL_GetTick();

  /* DMA Normal Mode */
  if ((hdma->Instance->CR & DMA_SxCR_CIRC) != DMA_SxCR_CIRC)
  {
    /* Disable ERR interrupt */
    __HAL_SPI_DISABLE_IT(hspi, SPI_IT_ERR);

    /* Disable Tx DMA Request */
    CLEAR_BIT(hspi->Instance->CR2, SPI_CR2_TXDMAEN);

    /* Check the end of the transaction */
//    if (SPI_EndRxTxTransaction(hspi, SPI_DEFAULT_TIMEOUT, tickstart) != HAL_OK)
//    {
//      SET_BIT(hspi->ErrorCode, HAL_SPI_ERROR_FLAG);
//    }

    /* Clear overrun flag in 2 Lines communication mode because received data is not read */
    if (hspi->Init.Direction == SPI_DIRECTION_2LINES)
    {
      __HAL_SPI_CLEAR_OVRFLAG(hspi);
    }

    hspi->TxXferCount = 0U;
    hspi->State = HAL_SPI_STATE_READY;

    if (hspi->ErrorCode != HAL_SPI_ERROR_NONE)
    {
      /* Call user error callback */
#if (USE_HAL_SPI_REGISTER_CALLBACKS == 1U)
      hspi->ErrorCallback(hspi);
#else
      HAL_SPI_ErrorCallback(hspi);
#endif /* USE_HAL_SPI_REGISTER_CALLBACKS */
      return;
    }
  }

  /* Call user Tx complete callback */
//  line++;
//  if (line > 480) {
//	  line = vFlag = 0;
//  }
}

inline void vga_end_line_callback(void) {
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
