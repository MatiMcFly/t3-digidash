# 🛻 T3 DigiDash -- Digital dashboard for the VW T3

(C) 2026 Matthias Schär, Rinaldo Leone, Timon Burkard

This is a project work part of the CAS Embedded Systems at FHNW Brugg-Windisch.

## Docs

Documentation, specification etc. can be found in [docs](docs/) directory.

## FW

This project consists of three firmware projects:

- [Disco](fw/disco/) -- ST Discovery Kit: STM32H747I-DISCO
- [Nucleo](fw/nucleo/) -- ST Nucleo Board: NUCLEO-H755ZI-Q
- [ESP](fw/esp/) -- Seeed ESP Board: ESP-C3

## HW

This project consists of three hardware projects:

- [disco-tft-adapter](hw/disco-tft-adapter/) -- Discovery Board TFT Adapter
- [t3-harness-adapter](hw/t3-harness-adapter/) -- T3 Wiring harness adapter
- [measurement-circuits](hw/measurement-circuits/) -- Collects all of the measurement circuits used
- [signal-emulator](hw/signal-emulator/) -- Sensor signal emulator (used for testing and demonstration)
