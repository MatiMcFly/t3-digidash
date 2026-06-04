/**
 * QspiFlash - Dual MT25TL01G memory-mapped initialization
 *
 * Target: STM32H747I-DISCO. The board carries one MT25TL01G dual-die
 * package; CubeMX only exposes Bank1 as Quad-capable with the current
 * pin allocation, so QUADSPI is configured for Bank1 single-flash mode.
 * That gives 64 MB of external NOR XIP-readable at 0x90000000.
 *
 * After QspiFlash_InitMemoryMapped() returns HAL_OK, TouchGFX assets
 * placed in ExtFlashSection / FontFlashSection are accessible like
 * normal .rodata.
 *
 * Call AFTER MX_QUADSPI_Init() and BEFORE any code touches assets.
 */

#ifndef QSPI_FLASH_H
#define QSPI_FLASH_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MT25TL01G geometry (one die accessed via Bank1 -> 64 MB usable). */
#define MT25TL01G_FLASH_SIZE        0x4000000U   /* 64 MB per die             */
#define MT25TL01G_SECTOR_SIZE       0x10000U     /* 64 KB                     */
#define MT25TL01G_SUBSECTOR_SIZE    0x1000U      /* 4 KB                      */
#define MT25TL01G_PAGE_SIZE         0x100U       /* 256 B                     */
#define MT25TL01G_DUMMY_CYCLES_READ_QUAD 8U      /* @ default volatile cfg    */

/* Initialize the MT25TL01G die wired to Bank1 and switch QUADSPI into
 * memory-mapped mode. hqspi must already be initialized via CubeMX with
 * DualFlash = DISABLE, FlashID = QSPI_FLASH_ID_1, FlashSize = 25
 * (2^(25+1) = 64 MB per die). */
HAL_StatusTypeDef QspiFlash_InitMemoryMapped(QSPI_HandleTypeDef *hqspi);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_FLASH_H */
