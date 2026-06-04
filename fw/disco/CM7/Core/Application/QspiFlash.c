/**
 * QspiFlash.c - MT25TL01G dual-flash bring-up + memory-mapped mode
 *
 * Hardware: STM32H747I-DISCO has two MT25TL01G dies wired in dual-flash
 * mode (BK1 + BK2 in parallel, shared CLK and NCS). CubeMX is configured
 * for "Dual Bank with Quad Lines" so the QSPI controller drives both
 * dies' IO lanes on every transfer, byte-interleaving the two dies into
 * a single 128 MB address space at 0x90000000.
 *
 * In dual-flash mode every command/data byte sent on a "1-line"
 * transfer is sent to BOTH dies simultaneously (BK1_IO0 and BK2_IO0
 * are both driven). Status / read-data is pulled from both dies and
 * interleaved (BK1 byte then BK2 byte). Consequently:
 *   - Status polling needs StatusBytesSize=2, mask=0x0202, match=0x0202
 *   - Volatile-config-register write needs NbData=2 (one byte per die)
 *   - Read-ID returns 16 bytes if you ask for 16 (8 from each die,
 *     interleaved)
 */

#include "QspiFlash.h"

/* Micron/MT25TL01G command set (subset used here) */
#define CMD_RESET_ENABLE            0x66U
#define CMD_RESET_MEMORY            0x99U
#define CMD_WRITE_ENABLE            0x06U
#define CMD_READ_STATUS_REG         0x05U
#define CMD_WRITE_VOLATILE_CFG      0x81U
#define CMD_READ_ID                 0x9FU
#define CMD_QUAD_OUT_FAST_READ_4B   0x6CU   /* 1-1-4, 4-byte addressing */

/* Volatile cfg register: [7:4] dummy cycles, [3]=1 reserved, [2:0]=wrap.
 * 0x8B -> 8 dummy cycles, no wrap, XIP disabled. */
#define MT25TL01G_VCR_VALUE         0x8BU

static HAL_StatusTypeDef qspi_cmd_only(QSPI_HandleTypeDef *hqspi, uint8_t opcode)
{
    QSPI_CommandTypeDef cmd = {0};
    cmd.InstructionMode  = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction      = opcode;
    cmd.AddressMode      = QSPI_ADDRESS_NONE;
    cmd.DataMode         = QSPI_DATA_NONE;
    cmd.DummyCycles      = 0;
    cmd.DdrMode          = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode         = QSPI_SIOO_INST_EVERY_CMD;
    return HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef qspi_write_enable(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef     cmd = {0};
    QSPI_AutoPollingTypeDef cfg = {0};

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = CMD_WRITE_ENABLE;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_NONE;
    cmd.DdrMode         = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle= QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode        = QSPI_SIOO_INST_EVERY_CMD;
    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Poll status register until WEL (bit 1) is set on BOTH dies.
     * In dual-flash mode the controller concatenates the two status
     * bytes into a 16-bit value (low = die0, high = die1). */
    cmd.Instruction = CMD_READ_STATUS_REG;
    cmd.DataMode    = QSPI_DATA_1_LINE;

    cfg.Match           = 0x0202U;
    cfg.Mask            = 0x0202U;
    cfg.MatchMode       = QSPI_MATCH_MODE_AND;
    cfg.StatusBytesSize = 2;
    cfg.Interval        = 0x10;
    cfg.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    return HAL_QSPI_AutoPolling(hqspi, &cmd, &cfg, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
}

static HAL_StatusTypeDef qspi_set_dummy_cycles(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef cmd = {0};
    /* Dual-flash: send the same VCR value to both dies (one byte per die). */
    uint8_t value[2] = { MT25TL01G_VCR_VALUE, MT25TL01G_VCR_VALUE };

    if (qspi_write_enable(hqspi) != HAL_OK) {
        return HAL_ERROR;
    }

    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction     = CMD_WRITE_VOLATILE_CFG;
    cmd.AddressMode     = QSPI_ADDRESS_NONE;
    cmd.DataMode        = QSPI_DATA_1_LINE;
    cmd.DummyCycles     = 0;
    cmd.NbData          = 2;
    cmd.DdrMode         = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle= QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode        = QSPI_SIOO_INST_EVERY_CMD;

    if (HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }
    if (HAL_QSPI_Transmit(hqspi, value, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

static HAL_StatusTypeDef qspi_enable_memory_mapped(QSPI_HandleTypeDef *hqspi)
{
    QSPI_CommandTypeDef      cmd = {0};
    QSPI_MemoryMappedTypeDef mm  = {0};

    cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    cmd.Instruction       = CMD_QUAD_OUT_FAST_READ_4B;
    cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
    cmd.AddressSize       = QSPI_ADDRESS_32_BITS;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode          = QSPI_DATA_4_LINES;
    cmd.DummyCycles       = MT25TL01G_DUMMY_CYCLES_READ_QUAD;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

    mm.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    mm.TimeOutPeriod     = 0;

    return HAL_QSPI_MemoryMapped(hqspi, &cmd, &mm);
}

HAL_StatusTypeDef QspiFlash_InitMemoryMapped(QSPI_HandleTypeDef *hqspi)
{
    if (hqspi == NULL) {
        return HAL_ERROR;
    }

    /* The chip can be left in any of these states by a previous boot or
     * by STM32CubeProgrammer's external loader:
     *   - QPI mode (4-line opcodes)        -> only 4-line cmds work
     *   - Continuous-read / XIP (no opcode -> next bus cycle is read addr)
     *   - Deep power-down
     *   - 4-byte addressing latched
     * Send wake + Reset-Enable + Reset-Memory in 1-line and 4-line
     * widths so any prior mode is cleared. */
    {
        QSPI_CommandTypeDef cmd = {0};
        cmd.AddressMode      = QSPI_ADDRESS_NONE;
        cmd.DataMode         = QSPI_DATA_NONE;
        cmd.DummyCycles      = 0;
        cmd.DdrMode          = QSPI_DDR_MODE_DISABLE;
        cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
        cmd.SIOOMode         = QSPI_SIOO_INST_EVERY_CMD;

        const uint32_t widths[2] = {
            QSPI_INSTRUCTION_4_LINES,
            QSPI_INSTRUCTION_1_LINE,
        };
        for (int i = 0; i < 2; ++i) {
            cmd.InstructionMode = widths[i];
            cmd.Instruction = 0xABU;  /* Release from deep power-down */
            (void)HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
            HAL_Delay(1);
            cmd.Instruction = CMD_RESET_ENABLE;
            (void)HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
            cmd.Instruction = CMD_RESET_MEMORY;
            (void)HAL_QSPI_Command(hqspi, &cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE);
            HAL_Delay(1);
        }
        (void)HAL_QSPI_Abort(hqspi);
    }

    /* Set 8 dummy cycles via volatile config register on both dies. */
    if (qspi_set_dummy_cycles(hqspi) != HAL_OK) return HAL_ERROR;

    /* Switch to memory-mapped mode -> 0x90000000..0x97FFFFFF live. */
    if (qspi_enable_memory_mapped(hqspi) != HAL_OK) return HAL_ERROR;

    return HAL_OK;
}
