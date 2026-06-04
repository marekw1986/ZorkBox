/*
 * tmp_spi_dma.h
 *
 *  Created on: Apr 20, 2026
 *      Author: marek
 */

#ifndef INC_TMP_SPI_DMA_H_
#define INC_TMP_SPI_DMA_H_

#include "stm32f4xx_hal.h"

void TMP_DMA_IRQHandler(DMA_HandleTypeDef *hdma);

#endif /* INC_TMP_SPI_DMA_H_ */
