/* axisram_bss_init.c
 *
 * Zero-fill the .axisram_bss linker section before C++ static
 * constructors run.
 *
 * Why this file exists:
 *   - Some statics (TouchGFXGeneratedHAL block allocator, framebuffer
 *     bookkeeping, etc.) are pinned to AXI-SRAM via a dedicated
 *     `.axisram_bss` section in the CM7 linker script.
 *   - That section lives outside the standard `_sbss .. _ebss` range,
 *     so the zero-fill loop in Reset_Handler does NOT cover it.
 *   - We can't add a second loop to Reset_Handler permanently, because
 *     CubeMX/CubeIDE regenerate `startup_stm32h747xx_CM7.s` wholesale
 *     on every code generation and wipe any local edits.
 *
 * How this works:
 *   - newlib's `__libc_init_array` (called from Reset_Handler before
 *     main()) walks `.preinit_array` first, then `.init_array`
 *     (C++ static constructors).
 *   - We register a function pointer in `.preinit_array` that zeros
 *     the AXISRAM bss range. This runs strictly before any C++ ctor
 *     and before main(), so all statics see a clean zeroed region.
 *
 * Do NOT remove this file. Without it, TouchGFX statics in AXISRAM
 * will start with random garbage and the display will crash or render
 * corruption shortly after boot.
 */

#include <stdint.h>

extern uint32_t _saxisram_bss;
extern uint32_t _eaxisram_bss;

static void zero_axisram_bss(void)
{
    uint32_t *p   = &_saxisram_bss;
    uint32_t *end = &_eaxisram_bss;
    while (p < end) {
        *p++ = 0U;
    }
}

/* Register zero_axisram_bss in .preinit_array so __libc_init_array
 * runs it before any C++ static constructor. `used` keeps the
 * function pointer from being GC'd by the linker. */
__attribute__((section(".preinit_array"), used))
static void (*const p_zero_axisram_bss)(void) = zero_axisram_bss;
