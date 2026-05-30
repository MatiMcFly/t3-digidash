//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file Display.c

/// @brief Display initialization and control functions
#include "Display.h"
#include "main.h"

// * ************************************************************************ *
// *                                DEFINES                                   *
// * ************************************************************************ *
#define FRAMEBUFFER_BASE_ADDR ((uint8_t *)0xD0000000UL)
#define FRAMEBUFFER_WIDTH 720U
#define FRAMEBUFFER_HEIGHT 720U
#define CHECKER_TILE_SIZE 40U
#define DSI_LINK_TEST_MODE 0U

#define LCD_CMD_DELAY 0xFFFEU
#define LCD_CMD_END 0xFFFFU

// * ************************************************************************ *
// *                                EXTERNAL VARIABLES                        *
// * ************************************************************************ *
extern DSI_HandleTypeDef hdsi;
extern LTDC_HandleTypeDef hltdc;

// * ************************************************************************ *
// *                             STATIC VARIABLES                             *
// * ************************************************************************ *
volatile uint32_t g_display_stage = 0U;
volatile int32_t g_display_last_status = 0;
volatile uint32_t g_display_failed_step = 0U;

volatile uint8_t g_display_status[4] = {0};
volatile uint8_t g_display_power[1] = {0};
volatile uint8_t g_display_madctrl[1] = {0};
volatile HAL_StatusTypeDef g_read_result_09 = HAL_ERROR;
volatile HAL_StatusTypeDef g_read_result_0A = HAL_ERROR;
volatile HAL_StatusTypeDef g_read_result_0B = HAL_ERROR;

typedef struct {
  uint16_t cmd;
  uint8_t len;
  uint8_t data[1];
} LcdInitCmd;

static const LcdInitCmd kLcdInitSequence[] = {
    // Generated from updated datasheet (1-lane / 2-lane mode compatible)
    {LCD_CMD_DELAY, 10U, {0x00U}},
    {0x00E0U, 1U, {0x00U}},
    {0x00E1U, 1U, {0x93U}},
    {0x00E2U, 1U, {0x65U}},
    {0x00E3U, 1U, {0xF8U}},
    {0x0080U, 1U, {0x01U}}, // 0x00 = 1-lane (0x01 = 2-lane, 0x03 = 4-lane)
    {0x00E0U, 1U, {0x01U}},
    {0x0000U, 1U, {0x00U}},
    {0x0001U, 1U, {0x41U}},
    {0x0003U, 1U, {0x10U}},
    {0x0004U, 1U, {0x44U}},
    {0x0017U, 1U, {0x00U}},
    {0x0018U, 1U, {0xD0U}},
    {0x0019U, 1U, {0x00U}},
    {0x001AU, 1U, {0x00U}},
    {0x001BU, 1U, {0xD0U}},
    {0x001CU, 1U, {0x00U}},
    {0x0024U, 1U, {0xFEU}},
    {0x0035U, 1U, {0x26U}},
    {0x0037U, 1U, {0x09U}},
    {0x0038U, 1U, {0x04U}},
    {0x0039U, 1U, {0x08U}},
    {0x003AU, 1U, {0x0AU}},
    {0x003CU, 1U, {0x78U}},
    {0x003DU, 1U, {0xFFU}},
    {0x003EU, 1U, {0xFFU}},
    {0x003FU, 1U, {0xFFU}},
    {0x0040U, 1U, {0x04U}},
    {0x0041U, 1U, {0x64U}},
    {0x0042U, 1U, {0xC7U}},
    {0x0043U, 1U, {0x18U}},
    {0x0044U, 1U, {0x0BU}},
    {0x0045U, 1U, {0x14U}},
    {0x0055U, 1U, {0x02U}},
    {0x0057U, 1U, {0x49U}},
    {0x0059U, 1U, {0x0AU}},
    {0x005AU, 1U, {0x1BU}},
    {0x005BU, 1U, {0x19U}},
    {0x005DU, 1U, {0x7FU}},
    {0x005EU, 1U, {0x56U}},
    {0x005FU, 1U, {0x43U}},
    {0x0060U, 1U, {0x37U}},
    {0x0061U, 1U, {0x33U}},
    {0x0062U, 1U, {0x25U}},
    {0x0063U, 1U, {0x2AU}},
    {0x0064U, 1U, {0x16U}},
    {0x0065U, 1U, {0x30U}},
    {0x0066U, 1U, {0x2FU}},
    {0x0067U, 1U, {0x32U}},
    {0x0068U, 1U, {0x53U}},
    {0x0069U, 1U, {0x43U}},
    {0x006AU, 1U, {0x4CU}},
    {0x006BU, 1U, {0x40U}},
    {0x006CU, 1U, {0x3DU}},
    {0x006DU, 1U, {0x31U}},
    {0x006EU, 1U, {0x20U}},
    {0x006FU, 1U, {0x0FU}},
    {0x0070U, 1U, {0x7FU}},
    {0x0071U, 1U, {0x56U}},
    {0x0072U, 1U, {0x43U}},
    {0x0073U, 1U, {0x37U}},
    {0x0074U, 1U, {0x33U}},
    {0x0075U, 1U, {0x25U}},
    {0x0076U, 1U, {0x2AU}},
    {0x0077U, 1U, {0x16U}},
    {0x0078U, 1U, {0x30U}},
    {0x0079U, 1U, {0x2FU}},
    {0x007AU, 1U, {0x32U}},
    {0x007BU, 1U, {0x53U}},
    {0x007CU, 1U, {0x43U}},
    {0x007DU, 1U, {0x4CU}},
    {0x007EU, 1U, {0x40U}},
    {0x007FU, 1U, {0x3DU}},
    {0x0080U, 1U, {0x31U}},
    {0x0081U, 1U, {0x20U}},
    {0x0082U, 1U, {0x0FU}},
    {0x00E0U, 1U, {0x02U}},
    {0x0000U, 1U, {0x5FU}},
    {0x0001U, 1U, {0x5FU}},
    {0x0002U, 1U, {0x5EU}},
    {0x0003U, 1U, {0x5EU}},
    {0x0004U, 1U, {0x50U}},
    {0x0005U, 1U, {0x48U}},
    {0x0006U, 1U, {0x48U}},
    {0x0007U, 1U, {0x4AU}},
    {0x0008U, 1U, {0x4AU}},
    {0x0009U, 1U, {0x44U}},
    {0x000AU, 1U, {0x44U}},
    {0x000BU, 1U, {0x46U}},
    {0x000CU, 1U, {0x46U}},
    {0x000DU, 1U, {0x5FU}},
    {0x000EU, 1U, {0x5FU}},
    {0x000FU, 1U, {0x57U}},
    {0x0010U, 1U, {0x57U}},
    {0x0011U, 1U, {0x77U}},
    {0x0012U, 1U, {0x77U}},
    {0x0013U, 1U, {0x40U}},
    {0x0014U, 1U, {0x42U}},
    {0x0015U, 1U, {0x5FU}},
    {0x0016U, 1U, {0x5FU}},
    {0x0017U, 1U, {0x5FU}},
    {0x0018U, 1U, {0x5EU}},
    {0x0019U, 1U, {0x5EU}},
    {0x001AU, 1U, {0x50U}},
    {0x001BU, 1U, {0x49U}},
    {0x001CU, 1U, {0x49U}},
    {0x001DU, 1U, {0x4BU}},
    {0x001EU, 1U, {0x4BU}},
    {0x001FU, 1U, {0x45U}},
    {0x0020U, 1U, {0x45U}},
    {0x0021U, 1U, {0x47U}},
    {0x0022U, 1U, {0x47U}},
    {0x0023U, 1U, {0x5FU}},
    {0x0024U, 1U, {0x5FU}},
    {0x0025U, 1U, {0x57U}},
    {0x0026U, 1U, {0x57U}},
    {0x0027U, 1U, {0x77U}},
    {0x0028U, 1U, {0x77U}},
    {0x0029U, 1U, {0x41U}},
    {0x002AU, 1U, {0x43U}},
    {0x002BU, 1U, {0x5FU}},
    {0x002CU, 1U, {0x1EU}},
    {0x002DU, 1U, {0x1EU}},
    {0x002EU, 1U, {0x1FU}},
    {0x002FU, 1U, {0x1FU}},
    {0x0030U, 1U, {0x10U}},
    {0x0031U, 1U, {0x07U}},
    {0x0032U, 1U, {0x07U}},
    {0x0033U, 1U, {0x05U}},
    {0x0034U, 1U, {0x05U}},
    {0x0035U, 1U, {0x0BU}},
    {0x0036U, 1U, {0x0BU}},
    {0x0037U, 1U, {0x09U}},
    {0x0038U, 1U, {0x09U}},
    {0x0039U, 1U, {0x1FU}},
    {0x003AU, 1U, {0x1FU}},
    {0x003BU, 1U, {0x17U}},
    {0x003CU, 1U, {0x17U}},
    {0x003DU, 1U, {0x17U}},
    {0x003EU, 1U, {0x17U}},
    {0x003FU, 1U, {0x03U}},
    {0x0040U, 1U, {0x01U}},
    {0x0041U, 1U, {0x1FU}},
    {0x0042U, 1U, {0x1EU}},
    {0x0043U, 1U, {0x1EU}},
    {0x0044U, 1U, {0x1FU}},
    {0x0045U, 1U, {0x1FU}},
    {0x0046U, 1U, {0x10U}},
    {0x0047U, 1U, {0x06U}},
    {0x0048U, 1U, {0x06U}},
    {0x0049U, 1U, {0x04U}},
    {0x004AU, 1U, {0x04U}},
    {0x004BU, 1U, {0x0AU}},
    {0x004CU, 1U, {0x0AU}},
    {0x004DU, 1U, {0x08U}},
    {0x004EU, 1U, {0x08U}},
    {0x004FU, 1U, {0x1FU}},
    {0x0050U, 1U, {0x1FU}},
    {0x0051U, 1U, {0x17U}},
    {0x0052U, 1U, {0x17U}},
    {0x0053U, 1U, {0x17U}},
    {0x0054U, 1U, {0x17U}},
    {0x0055U, 1U, {0x02U}},
    {0x0056U, 1U, {0x00U}},
    {0x0057U, 1U, {0x1FU}},
    {0x00E0U, 1U, {0x02U}},
    {0x0058U, 1U, {0x40U}},
    {0x0059U, 1U, {0x00U}},
    {0x005AU, 1U, {0x00U}},
    {0x005BU, 1U, {0x30U}},
    {0x005CU, 1U, {0x01U}},
    {0x005DU, 1U, {0x30U}},
    {0x005EU, 1U, {0x01U}},
    {0x005FU, 1U, {0x02U}},
    {0x0060U, 1U, {0x30U}},
    {0x0061U, 1U, {0x03U}},
    {0x0062U, 1U, {0x04U}},
    {0x0063U, 1U, {0x04U}},
    {0x0064U, 1U, {0xA6U}},
    {0x0065U, 1U, {0x43U}},
    {0x0066U, 1U, {0x30U}},
    {0x0067U, 1U, {0x73U}},
    {0x0068U, 1U, {0x05U}},
    {0x0069U, 1U, {0x04U}},
    {0x006AU, 1U, {0x7FU}},
    {0x006BU, 1U, {0x08U}},
    {0x006CU, 1U, {0x00U}},
    {0x006DU, 1U, {0x04U}},
    {0x006EU, 1U, {0x04U}},
    {0x006FU, 1U, {0x88U}},
    {0x0075U, 1U, {0xD9U}},
    {0x0076U, 1U, {0x00U}},
    {0x0077U, 1U, {0x33U}},
    {0x0078U, 1U, {0x43U}},
    {0x00E0U, 1U, {0x00U}},
    {0x003AU, 1U, {0x77U}}, // Set Pixel Format: 0x77 = 24bpp (RGB888)
    {0x0011U, 0U, {0x00U}}, // Sleep Out
    {LCD_CMD_DELAY, 120U, {0x00U}},
    {0x0029U, 0U, {0x00U}}, // Display On
    {LCD_CMD_DELAY, 5U, {0x00U}},
    {0x0035U, 1U, {0x00U}}, // Tearing Effect ON, V-blanking only (TEM=0)
    // -------- Backlight control (used by panels with internal LED driver, e.g.
    // STM32H747I-DISCO MB1166 / OTM8009A). Ignored by panels whose backlight
    // is driven by an external GPIO. --------
    {0x0051U, 1U, {0xFFU}}, // Write Display Brightness = max
    {0x0053U, 1U, {0x2CU}}, // Write CTRL Display: BCTRL=1, DD=1, BL=1
    {0x0055U, 1U, {0x02U}}, // Write CABC: still image
    {LCD_CMD_END, 0U, {0x00U}},
};

// * ************************************************************************ *
// *                           FORWARD DECLARATIONS                           *
// * ************************************************************************ *
static void DisplayFail(uint32_t step, HAL_StatusTypeDef status);
static void DisplayResetSequence(void);
static HAL_StatusTypeDef EnableDsiLpCommands(void);
static HAL_StatusTypeDef DsiWriteCommand(uint8_t cmd, const uint8_t *data, uint16_t len);
static HAL_StatusTypeDef DisplaySendInitSequence(void);
static void FillCheckerboardRGB888(void);

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *
void DisplayInit(void) {
  // 1) Enable LP-command capability on the DSI host.
  g_display_stage = 1U;
  if (EnableDsiLpCommands() != HAL_OK) {
    DisplayFail(1U, HAL_ERROR);
  }

  // DIAG: backlight on early so visible glow proves panel power even
  // if DSI init still fails.
  HAL_GPIO_WritePin(BL_CTRL_GPIO_Port, BL_CTRL_Pin, GPIO_PIN_SET);

  // 2) Start the DSI wrapper. This is what actually drives the data
  //    lanes into LP-11 stop state via the host wrapper. WITHOUT this,
  //    PHY status bits PSS0/PSS1 stay 0 and every LP escape goes into
  //    a void. Video timing has been configured (HAL_DSI_ConfigVideoMode
  //    in MX_DSIHOST_DSI_Init), but no actual scan-out happens until
  //    LTDC starts feeding pixels (LTDC layer enable below).
  g_display_stage = 2U;
  if (HAL_DSI_Start(&hdsi) != HAL_OK) {
    DisplayFail(2U, HAL_ERROR);
  }

  // Allow LP commands during V-blank windows even though VidCfg set
  // LPCommandEnable = DISABLE.
  hdsi.Instance->VMCR |= DSI_VMCR_LPCE;

  // Give the PHY a moment to settle in LP-11 after the wrapper enable.
  HAL_Delay(2);

  // 3) Hardware reset the panel.
  g_display_stage = 3U;
  DisplayResetSequence();

  // 4) Push the full panel init sequence. LP commands flow during
  //    V-blank (LTDC isn't pushing video yet because the layer is
  //    still disabled), so timing is relaxed.
  g_display_stage = 4U;
  if (DisplaySendInitSequence() != HAL_OK) {
    DisplayFail(4U, HAL_ERROR);
  }

  // 5) Diagnostic readback of the panel ID (RDDIDIF, 0x04).
  g_display_stage = 5U;
  g_read_result_09 = HAL_DSI_Read(&hdsi, 0U, (uint8_t *)g_display_status,
                                  3U, DSI_DCS_SHORT_PKT_READ, 0x04U,
                                  (uint8_t *)g_display_status);

#if DSI_LINK_TEST_MODE
  g_display_stage = 12U;
  if (HAL_DSI_PatternGeneratorStart(&hdsi, 0U, 0U) != HAL_OK) {
    DisplayFail(12U, HAL_ERROR);
  }
#endif

  // 6) Fill the framebuffer before enabling the LTDC layer.
  g_display_stage = 9U;
  FillCheckerboardRGB888();

  g_display_stage = 8U;
  if (HAL_LTDC_SetAlpha(&hltdc, 255U, 0U) != HAL_OK) {
    DisplayFail(8U, HAL_ERROR);
  }
  __HAL_LTDC_LAYER_ENABLE(&hltdc, 0U);
  __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);

  /* Enable the LTDC line interrupt so TouchGFX gets a VSYNC tick every
   * frame. Priority must be numerically >= configMAX_SYSCALL_INTERRUPT_PRIORITY
   * (5) so the ISR can call FreeRTOS *FromISR APIs via OSWrappers::signalVSync.
   * The line event itself is armed by TouchGFXHAL::enableLCDControllerInterrupt
   * once HAL::initialize() finishes, and re-armed every frame by the
   * TouchGFX_SignalVsync() shim. */
  HAL_NVIC_SetPriority(LTDC_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);

  g_display_stage = 10U;
}

/* Bridge from the C-domain HAL callback to the TouchGFX OS abstraction. */
extern void TouchGFX_SignalVsync(void);

void HAL_LTDC_LineEventCallback(LTDC_HandleTypeDef *hltdc_param)
{
  (void)hltdc_param;
  TouchGFX_SignalVsync();
}

// Live counters for diagnosing flicker / image corruption.
// Increment from the LTDC ISR (or polled here) so the cause of
// flicker (FIFO underrun vs DSI errors) is visible in the debugger.
volatile uint32_t g_ltdc_underrun_count = 0U;
volatile uint32_t g_ltdc_transfer_error_count = 0U;

void DisplayTask(void) {
  g_display_stage = 11U;

  // Poll LTDC error flags. Underrun = SDRAM/FB bandwidth too low.
  // Transfer error = AHB master access fault.
  uint32_t isr = hltdc.Instance->ISR;
  if ((isr & LTDC_ISR_FUIF) != 0U) {
    g_ltdc_underrun_count++;
    hltdc.Instance->ICR = LTDC_ICR_CFUIF;
  }
  if ((isr & LTDC_ISR_TERRIF) != 0U) {
    g_ltdc_transfer_error_count++;
    hltdc.Instance->ICR = LTDC_ICR_CTERRIF;
  }

  // Reading from the display while video is streaming can cause
  // BTA/LP mode conflicts resulting in display flickering/pulsating.
  // Disabled the polling for now.
}

// * ************************************************************************ *
// *                              LOCAL FUNCTIONS                             *
// * ************************************************************************ *
static void DisplayFail(uint32_t step, HAL_StatusTypeDef status) {
  g_display_failed_step = step;
  g_display_last_status = (int32_t)status;
  Error_Handler();
}

static void DisplayResetSequence(void) {
  HAL_GPIO_WritePin(DSI_Reset_GPIO_Port, DSI_Reset_Pin, GPIO_PIN_SET);
  HAL_Delay(5);
  HAL_GPIO_WritePin(DSI_Reset_GPIO_Port, DSI_Reset_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(DSI_Reset_GPIO_Port, DSI_Reset_Pin, GPIO_PIN_SET);
  HAL_Delay(120);
}

static HAL_StatusTypeDef EnableDsiLpCommands(void) {
  DSI_LPCmdTypeDef lp = {0};

  lp.LPGenShortWriteNoP = DSI_LP_GSW0P_ENABLE;
  lp.LPGenShortWriteOneP = DSI_LP_GSW1P_ENABLE;
  lp.LPGenShortWriteTwoP = DSI_LP_GSW2P_ENABLE;
  lp.LPGenShortReadNoP = DSI_LP_GSR0P_ENABLE;
  lp.LPGenShortReadOneP = DSI_LP_GSR1P_ENABLE;
  lp.LPGenShortReadTwoP = DSI_LP_GSR2P_ENABLE;
  lp.LPGenLongWrite = DSI_LP_GLW_ENABLE;
  lp.LPDcsShortWriteNoP = DSI_LP_DSW0P_ENABLE;
  lp.LPDcsShortWriteOneP = DSI_LP_DSW1P_ENABLE;
  lp.LPDcsShortReadNoP = DSI_LP_DSR0P_ENABLE;
  lp.LPDcsLongWrite = DSI_LP_DLW_ENABLE;
  lp.LPMaxReadPacket = DSI_LP_MRDP_ENABLE;
  lp.AcknowledgeRequest = DSI_ACKNOWLEDGE_DISABLE; // Keep auto-acking disabled

  return HAL_DSI_ConfigCommand(&hdsi, &lp);
}

static HAL_StatusTypeDef DsiWriteCommand(uint8_t cmd, const uint8_t *data, uint16_t len) {
  if (len == 0U) {
    return HAL_DSI_ShortWrite(&hdsi, 0U, DSI_DCS_SHORT_PKT_WRITE_P0, cmd, 0U);
  }

  if (len == 1U) {
    return HAL_DSI_ShortWrite(&hdsi, 0U, DSI_DCS_SHORT_PKT_WRITE_P1, cmd, data[0]);
  }

  return HAL_DSI_LongWrite(&hdsi, 0U, DSI_DCS_LONG_PKT_WRITE, len, cmd, (uint8_t *)data);
}

static HAL_StatusTypeDef DisplaySendInitSequence(void) {
  uint32_t i;
  for (i = 0U; i < (uint32_t)(sizeof(kLcdInitSequence) / sizeof(kLcdInitSequence[0])); ++i) {
    const LcdInitCmd *entry = &kLcdInitSequence[i];

    if (entry->cmd == LCD_CMD_END) {
      return HAL_OK;
    }

    if (entry->cmd == LCD_CMD_DELAY) {
      HAL_Delay(entry->len);
      continue;
    }

    if (DsiWriteCommand((uint8_t)entry->cmd, entry->data, entry->len) != HAL_OK) {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

static void FillCheckerboardRGB888(void) {
  uint32_t x;
  uint32_t y;

  for (y = 0U; y < FRAMEBUFFER_HEIGHT; ++y) {
    for (x = 0U; x < FRAMEBUFFER_WIDTH; ++x) {
      const uint32_t tile_x = x / CHECKER_TILE_SIZE;
      const uint32_t tile_y = y / CHECKER_TILE_SIZE;
      const uint8_t white = (uint8_t)((tile_x + tile_y) & 0x1U ? 0xFFU : 0x00U);
      const uint32_t pixel = (y * FRAMEBUFFER_WIDTH + x) * 3U;

      FRAMEBUFFER_BASE_ADDR[pixel + 0U] = white;
      FRAMEBUFFER_BASE_ADDR[pixel + 1U] = white;
      FRAMEBUFFER_BASE_ADDR[pixel + 2U] = white;
    }
  }
}