/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */

#include <touchgfx/hal/OSWrappers.hpp>
#include "stm32h7xx_hal.h"

extern "C" {
    extern LTDC_HandleTypeDef hltdc;
    extern MDMA_HandleTypeDef hmdma_display;

    /* Hook provided by the generated TouchGFXGeneratedHAL.cpp; we have
     * to call it from the MDMA transfer-complete IRQ to advance the
     * partial-framebuffer state machine (free transmitted block, start
     * the next pending one if any). */
    void DisplayDriver_TransferCompleteCallback(void);
}

using namespace touchgfx;

/* LTDC framebuffer in SDRAM. Single buffer in partial mode (no FB swap).
 * Keep FB0_BASE / FB_STRIDE_BYTES in sync with the LTDC layer config in
 * MX_LTDC_Init: 720 px wide, RGB888 = 3 B/px. */
static constexpr uint32_t kFbBase        = 0xD0000000U;
static constexpr uint32_t kFbStrideBytes = 720U * 3U;

/* Set true when MDMA is mid-transfer; reset by MDMA TC IRQ. The framework
 * polls this via touchgfxDisplayDriverTransmitActive() before queueing a
 * new transfer in flushFrameBuffer(), and busy-waits on it inside
 * endFrame() until the last block of the frame has been transmitted. */
static volatile int s_transmitActive = 0;

extern "C" int touchgfxDisplayDriverTransmitActive(void)
{
    return s_transmitActive;
}

/* Called by the framework when a block has been rendered into AXI-SRAM
 * and is ready to be DMAed into the LTDC framebuffer in SDRAM. The block
 * is a rectangle (x, y, w, h) in screen coordinates; the source `pixels`
 * buffer is contiguous (one row immediately follows the previous, with
 * stride = w*3). The destination is the LTDC framebuffer at
 * kFbBase + y*kFbStrideBytes + x*3, with destination stride = full screen
 * width.
 *
 * Programmed as: BlockDataLength = w*3 (one row), BlockCount = h (number
 * of rows). After each row MDMA jumps the destination pointer by
 * `kFbStrideBytes - w*3` so we land at the start of the next row in the
 * full-screen FB; source has no offset between blocks since rows are
 * contiguous. */
extern "C" void touchgfxDisplayDriverTransmitBlock(const uint8_t* pixels,
                                                   uint16_t x, uint16_t y,
                                                   uint16_t w, uint16_t h)
{
    const uint32_t rowBytes  = static_cast<uint32_t>(w) * 3U;
    const uint32_t dstAddr   = kFbBase + (static_cast<uint32_t>(y) * kFbStrideBytes) + (static_cast<uint32_t>(x) * 3U);
    const int32_t  dstOffset = static_cast<int32_t>(kFbStrideBytes) - static_cast<int32_t>(rowBytes);

    /* Cache coherency: blockAllocator memory lives in AXI-SRAM which is
     * cacheable WB. CPU rendering wrote the block via D-cache; MDMA
     * reads via AXI master (cache-bypass). Clean (write-back) the dirty
     * lines so MDMA sees current pixel data. The block is contiguous
     * (rowBytes * h bytes from `pixels`). */
    SCB_CleanDCache_by_Addr(reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(pixels)),
                            static_cast<int32_t>(rowBytes * h));

    s_transmitActive = 1;

    /* Patch only the destination block-update value (DUV, upper 16 bits
     * of CBRUR). Source update value (SUV, lower 16 bits) stays 0 since
     * the block buffer is contiguous. CBRUR offsets are signed 16-bit,
     * applied to the current pointer at the END of each block, so a
     * value of (kFbStrideBytes - rowBytes) lines up the next row at
     * dst_init + (block_n+1) * kFbStrideBytes. */
    WRITE_REG(hmdma_display.Instance->CBRUR,
              (static_cast<uint32_t>(dstOffset) & 0xFFFFU) << 16);

    if (HAL_MDMA_Start_IT(&hmdma_display,
                          reinterpret_cast<uint32_t>(pixels),
                          dstAddr,
                          rowBytes,
                          h) != HAL_OK)
    {
        /* Should not happen unless the channel is busy; but if it does,
         * we must still let the framework progress. Mark inactive and
         * fake the completion callback. */
        s_transmitActive = 0;
        DisplayDriver_TransferCompleteCallback();
    }
}

void TouchGFXHAL::initialize()
{
    /* Register MDMA TC/Error callbacks before HAL initialize so they
     * are valid the first time the framework calls flushFrameBuffer.
     * The TC callback clears s_transmitActive and tells the framework
     * to start the next pending block (if any). */
    hmdma_display.XferCpltCallback = [](MDMA_HandleTypeDef*) {
        s_transmitActive = 0;
        DisplayDriver_TransferCompleteCallback();
    };
    hmdma_display.XferErrorCallback = [](MDMA_HandleTypeDef*) {
        /* On error, abort the current transfer and let the framework
         * recover. The frame will be partially torn but the engine
         * keeps running. */
        s_transmitActive = 0;
        DisplayDriver_TransferCompleteCallback();
    };

    TouchGFXGeneratedHAL::initialize();
}

/**
 * Gets the frame buffer address used by the TFT controller.
 *
 * @return The address of the frame buffer currently being displayed on the TFT.
 */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    /* Not used in partial-framebuffer mode; the generated parent
     * implementation already returns 0 to signal this. */
    return TouchGFXGeneratedHAL::getTFTFrameBuffer();
}

/**
 * Sets the frame buffer address used by the TFT controller.
 *
 * @param [in] address New frame buffer address.
 */
void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    /* Not used in partial-framebuffer mode (single LTDC framebuffer, no
     * swap). Forward to generated parent for completeness. */
    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
}

/**
 * This function is called whenever the framework has performed a partial draw.
 *
 * @param rect The area of the screen that has been drawn, expressed in absolute coordinates.
 *
 * @see flushFrameBuffer().
 */
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    // Calling parent implementation of flushFrameBuffer(const touchgfx::Rect& rect).
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.
    // Please note, HAL::flushFrameBuffer(const touchgfx::Rect& rect) must
    // be called to notify the touchgfx framework that flush has been performed.
    // To calculate the start address of rect,
    // use advanceFrameBufferToRect(uint8_t* fbPtr, const touchgfx::Rect& rect)
    // defined in TouchGFXGeneratedHAL.cpp

    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

bool TouchGFXHAL::blockCopy(void* RESTRICT dest, const void* RESTRICT src, uint32_t numBytes)
{
    return TouchGFXGeneratedHAL::blockCopy(dest, src, numBytes);
}

/**
 * Configures the interrupts relevant for TouchGFX. This primarily entails setting
 * the interrupt priorities for the DMA and LCD interrupts.
 */
void TouchGFXHAL::configureInterrupts()
{
    // Calling parent implementation of configureInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::configureInterrupts();
}

/**
 * Used for enabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::enableInterrupts()
{
    // Calling parent implementation of enableInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::enableInterrupts();
}

/**
 * Used for disabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::disableInterrupts()
{
    // Calling parent implementation of disableInterrupts().
    //
    // To overwrite the generated implementation, omit the call to the parent function
    // and implement the needed functionality here.

    TouchGFXGeneratedHAL::disableInterrupts();
}

/**
 * Configure the LCD controller to fire interrupts at VSYNC. Called automatically
 * once TouchGFX initialization has completed.
 */
void TouchGFXHAL::enableLCDControllerInterrupt()
{
    /* Override the (empty) generated implementation: arm the LTDC line
     * event so HAL_LTDC_LineEventCallback fires once per frame and we can
     * deliver a VSYNC tick to the TouchGFX render task. Line 0 is the start
     * of the active area and gives a stable, jitter-free frame boundary. */
    HAL_LTDC_ProgramLineEvent(&hltdc, 0U);
}

/**
 * C-callable shim used by HAL_LTDC_LineEventCallback in stm32h7xx_it /
 * Display.c to wake the TouchGFX render task on every VSYNC.
 *
 * In partial-framebuffer mode the framework needs HAL::vSync() ticked
 * (to advance the frame counter / block-allocator state) AND
 * OSWrappers::signalVSync() (to wake the render task). The generated
 * touchgfxSignalVSync() does both — forward to it.
 */
extern "C" void touchgfxSignalVSync(void);

extern "C" void TouchGFX_SignalVsync(void)
{
    touchgfxSignalVSync();
    /* Re-arm the line event so it fires again on the next frame. */
    HAL_LTDC_ProgramLineEvent(&hltdc, 0U);
}

bool TouchGFXHAL::beginFrame()
{
    return TouchGFXGeneratedHAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    TouchGFXGeneratedHAL::endFrame();
}

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
