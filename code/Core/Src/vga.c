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

#define VISIBLE_START   35
#define VISIBLE_END     514

#define SCANLINE_LEN    80  // TEMP

#define VGA_COLS        80
#define VGA_ROWS        30

#define CHUNK_LINES     32  // scanlines per buffer chunk

uint32_t update_buffer = 0x00;

char vga_buffer[VGA_COLS * VGA_ROWS];
volatile uint16_t vga_cursor = 0;
//volatile uint32_t cursor_timer = 0;
//volatile uint8_t cursor_visible = 1;

const uint8_t null_byte = 0x00;
volatile uint16_t line = 0;
volatile uint8_t vFlag = 0x00;
volatile uint8_t active_scanline = 0x00;

static volatile uint8_t scroll_pending = 0;

// Each buffer now holds CHUNK_LINES scanlines
uint8_t scanline[2][CHUNK_LINES][SCANLINE_LEN + 1];

void vga_prepare_line_DMA(void);
static void fill_scanline(uint8_t buf_idx, uint16_t start_line);

void vga_init(void) {
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);         // HSYNC (highest)
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 2, 1); // SPI DMA
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);         // VSYNC

    memset(vga_buffer, ' ', sizeof(vga_buffer));

    // Pre-fill both chunks so DMA has valid data immediately
    fill_scanline(0, 0);
    fill_scanline(1, CHUNK_LINES);

    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    /* disable transfer error interrupt */
    DMA2_Stream2->CR &= ~DMA_SxCR_TEIE;

    /* Configure DMA Stream destination address */
    DMA2_Stream2->PAR = (uint32_t)&SPI1->DR;
    /* Configure DMA Stream source address — first scanline of active chunk */
    DMA2_Stream2->M0AR = (uint32_t)scanline[active_scanline][0];
    const uint16_t size = SCANLINE_LEN + 1;
    DMA2_Stream2->NDTR = size;
}

void vga_handle(void) {
    // Only scroll during vertical blanking — DMA is idle, safe to memmove
    if (scroll_pending && !vFlag) {
        memmove(vga_buffer, vga_buffer + VGA_COLS, VGA_COLS * (VGA_ROWS - 1));
        memset(vga_buffer + VGA_COLS * (VGA_ROWS - 1), ' ', VGA_COLS);
        scroll_pending = 0;
    }

    if (update_buffer) {
        // Snapshot volatile line once — prevents ISR from changing it mid-calculation
        uint16_t current_line = line;
        uint8_t  fill_buf     = active_scanline ^ 1;
        uint16_t next_start   = current_line + CHUNK_LINES;
        if (next_start >= 480) next_start = 0;

        fill_scanline(fill_buf, next_start);
        update_buffer = 0x00;
    }

//    if (!vFlag && (uint8_t)((HAL_GetTick() - cursor_timer) >= 500)) {
//    	cursor_visible = !cursor_visible;
//    	vga_buffer[vga_cursor] = cursor_visible ? '_' : ' ';
//        cursor_timer = HAL_GetTick();
//    }
}

void vga_putc(const char c) {
    // Always clear cursor at current position before doing anything
    if (vga_buffer[vga_cursor] == '_')
        vga_buffer[vga_cursor] = ' ';
    switch (c) {
        case '\r':
        {
            uint16_t row = vga_cursor / VGA_COLS;
            vga_cursor = row * VGA_COLS;
        }
        break;

        case '\n':
            vga_cursor = ((vga_cursor / VGA_COLS) + 1) * VGA_COLS;
            if (vga_cursor >= VGA_COLS * VGA_ROWS) {
                scroll_pending = 1;
                vga_cursor = VGA_COLS * (VGA_ROWS - 1);
            }
        break;

        case '\b':
            if (vga_cursor > 0) {
                // If cursor is on '_', clear it first
                vga_buffer[vga_cursor] = ' ';
                vga_cursor--;
                vga_buffer[vga_cursor] = ' ';
            }
        break;

        default:
            if (c < 32 || c > 126) break;
//            while (vFlag) { vga_handle(); }
            vga_buffer[vga_cursor] = c;
            vga_cursor++;
            if (vga_cursor >= VGA_COLS * VGA_ROWS) {
                scroll_pending = 1;
                vga_cursor = VGA_COLS * (VGA_ROWS - 1);
            }
            vga_buffer[vga_cursor] = '_';
//            cursor_visible = 1;
//            cursor_timer = HAL_GetTick();
        break;
    }
}

// Fill all CHUNK_LINES scanlines into buf_idx, starting from start_line
static void fill_scanline(uint8_t buf_idx, uint16_t start_line) {
    for (uint8_t i = 0; i < CHUNK_LINES; i++) {
        uint16_t current_line = start_line + i;
        if (current_line >= 480) break;  // past visible area

        const uint8_t vga_buf_y      = current_line / 16;
        const uint8_t vga_buf_glyph  = current_line % 16;
        const uint8_t *vga_buf_row   = (const uint8_t *)&vga_buffer[vga_buf_y * VGA_COLS];
        uint8_t *dst                 = scanline[buf_idx][i];

        for (int x = 0; x < SCANLINE_LEN; x++) {
            const uint8_t ch = vga_buf_row[x];
            if (ch < 32 || ch > 126) {
                dst[x] = 0x00;
            } else {
                dst[x] = fonts[ch - 32][vga_buf_glyph];
            }
        }
        dst[SCANLINE_LEN] = 0x00;  // explicit line terminator — pull data low
    }
}

__attribute__((section(".RamFunc"))) void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_CC2IF)
    {
        if (vFlag) {
            /* Enable Common interrupts */
            DMA2_Stream2->CR |= DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_SxCR_EN;
            /* Enable SPI DMA request */
            SET_BIT(SPI1->CR2, SPI_CR2_TXDMAEN);
        }
        TIM2->SR &= ~TIM_SR_CC2IF;
    }
}

__attribute__((section(".RamFunc"))) void TIM4_IRQHandler(void)
{
    if (TIM4->SR & TIM_SR_CC3IF) {
        TIM4->SR &= ~TIM_SR_CC3IF;
        vFlag = 0x01;
    }
}

__attribute__((section(".RamFunc"))) void DMA2_Stream2_IRQHandler(void)
{
    uint32_t isr = DMA2->LISR;
    if ((isr & DMA_LISR_TCIF2) || (isr & DMA_LISR_TEIF2)) {
        DMA2->LIFCR = DMA_LIFCR_CTEIF2 | DMA_LIFCR_CTCIF2;

        line++;
        if (line >= 480) {
            line = 0;
            vFlag = 0;
        }

        uint8_t line_in_chunk = line % CHUNK_LINES;

        if (line_in_chunk == 0) {
            // Chunk boundary: swap buffers and ask main loop to refill the
            // one we just finished transmitting
            active_scanline ^= 1;
            update_buffer = 0x01;
        }

        // Always index into active_scanline — it now stays stable for the
        // entire CHUNK_LINES duration
        DMA2_Stream2->M0AR = (uint32_t)scanline[active_scanline][line_in_chunk];
        DMA2_Stream2->NDTR = SCANLINE_LEN + 1;
    }
}
