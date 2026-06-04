/*
 * tmp_spi_dma.c
 *
 *  Created on: Apr 20, 2026
 *      Author: marek
 */

#include "tmp_spi_dma.h"

void TMP_DMA_IRQHandler(DMA_HandleTypeDef *hdma)
{
	/* disable transfer error interrupt */
	DMA2_Stream2->CR &= ~DMA_SxCR_TEIE;
	/* clear transfer error flag */
	DMA2->LIFCR = DMA_LIFCR_CTEIF2;

	DMA2->LIFCR = DMA_LIFCR_CTCIF2;
	DMA2_Stream2->CR  &= ~(DMA_IT_TC);

	CLEAR_BIT(SPI1->CR2, SPI_IT_ERR);
	CLEAR_BIT(SPI1->CR2, SPI_CR2_TXDMAEN);
}
