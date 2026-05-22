# 🖥️ Discovery Board TFT Adapter

This project contains the KiCad hardware design for a TFT display adapter designed to connect a TFT panel (GX040R-30MB-A1ICNL9707) to an STMicroelectronics STM32H747I Discovery Kit.

The adapter board includes circuitry to interface the display with the Discovery board, including a TPS61169 LED driver for the display backlight.

## 🔌 Connectivity between DISCO board and GX040R display

- according to the manufacturer, the display has to be connected as follows: Board (DP/N0) to LCD (DP/N1) and (DP/N1) to (DP/N3). A previous revision A of this board has gotten this wrong. D0 of the board has been connected to D0 of the TFT and D1 of the board to D1 of the TFT.
- CTP (capacitive touch display) functionalities (I2C, INT, ...) are not used

## 📚 Datasheets and Documentation

All relevant datasheets, manuals, and reference documents can be found in this [Google Drive folder](https://drive.google.com/drive/folders/1Etk3SSePW-oP0WRn9YrgEl5WCk2nYt2u?usp=sharing).

### 📋 Document Overview:

* **`disco_tft_adapter.pdf`** - The schematic for this adapter board project.
* **`GX040R-30MB-A1(V1).pdf`** - Datasheet for the TFT display panel used with this adapter.
* **`ICNL_9707_Datasheet...pdf`** - Datasheet for the ICNL9707, the display driver IC integrated into the TFT panel (might be obsolete)
* **`tps61169.pdf`** - Datasheet for the Texas Instruments TPS61169, the WLED driver IC used on this adapter for the display backlight.
* **`stm32h747-datasheet.pdf`** - Electrical characteristics and pinout for the STM32H747 microcontroller.
* **`stm32h747-ref-manual.pdf`** - Detailed reference manual for the STM32H747 MCU peripherals and registers.
* **`stm32h747-prog-manual.pdf`** - Programming manual detailing the architecture and instruction set for the STM32H747.
* **`mb1166-default-a09-schematic.pdf`** - Schematic for the MB1166 base board of the STM32H7 Discovery kit.
* **`mb1248-h747i-d03-schematic.pdf`** - Schematic for the MB1248 CPU board of the STM32H747I-DISCO kit.
* **`um2411-discovery-kit-...pdf`** - Official user manual (UM2411) for the STM32H747I-DISCO Discovery kit.
* **`JD9365DA-H3_User_Guide_Preliminary_V0.00_20200827.pdf` - User guide for the display driver. This is the last version that was received from the contact person at the display manufacturer.