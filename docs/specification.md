# T3 Digidash - Spezifikation

**Projektarbeit CAS Embedded Systems 2026**

**Team:** Rinaldo Leone, Timon Burkard, Matthias Schär

**Dozenten:** Michael Pichler, Dominique Kunz, Manuel Di Cerbo

**Datum:** 23. Mai 2026

## Inhaltsverzeichnis

1. Projektbeschreibung
2. Verwendete Hardware
3. Anforderungen
4. Systemübersicht
5. Planung
6. Quellen

## 1. Projektbeschreibung

Zum Abschluss des CAS Embedded Systems soll ein verteiltes System entwickelt werden. Dieses wird in einem VW Transporter Typ 2.5 eingesetzt, um verschiedene analoge Instrumente im Armaturenbrett zu ersetzen. Einfache analoge Anzeigeelemente beschränken sich typischerweise auf den Tankfüllstand und die Kühlwassertemperatur. Der Kabelbaumabgriff, welcher zum Tacho führt,, beinhaltet jedoch noch diverse andere nützliche Signale, wie zum Beispiel die Drehzahlanzeige. Mit Hilfe eines passenden TFT-LCD kann somit eine konfigurierbare und dynamische digitale Anzeige gestaltet werden. Diese bietet weiter die Möglichkeit, zusätzliche, nicht originale Anzeigeelemente anzuschliessen, wie zum Beispiel einen Sensor zur Messug der Öltemperatur.

Nachfolgend eine Auflistung aller Grössen, die im Rahmen dieses Projektes sind:

| Signal                  | Sensor-Typ                   | Messgrösse     | Wertebereich     | Bemerkungen |
|-------------------------|------------------------------|----------------|------------------|-------------|
| Wassertemperatur        | NTC gegen Chassis-GND        | 1100 - 100 Ohm | -10 ... 150 °C   | |
| Drehzahl                | Pulse von Zündspule          | 12V            | 0 ... 7000 U/min | Achtung, Spannungsspitzen. Via Optokoppler messen. Skaliert mit Faktor 4 |
| Batteriespannung        | Spannungsteiler              | -              | 0 ... 20 V       | |
| Tankfüllstand           | Hebelgeber gegen Chassis-GND | 40 ... 300 Ohm | 0 ... 80 l       | |
| Blinker                 | Schalter                     | 12V / 0V       | on / off         | |
| Fernlicht               | Schalter                     | 12V / 0V       | on / off         | |
| Öldruckschalter 0.3 bar | Schalter                     | 12V / 0V       | on / off         | |
| Öldruckschalter 1.8 bar | Schalter                     | 12V / 0V       | on / off         | |

## 2. Verwendete Hardware

Das System besteht aus drei eigenständigen Hardware-Plattformen, die über digitale Schnittstellen
miteinander kommunizieren, sowie aus mehreren projektspezifischen Adapterplatinen.

### 2.1 Mikrocontroller-Boards

- **ST Nucleo NUCLEO-H755ZI-Q** (STM32H755ZI, Dual-Core Cortex-M7 + Cortex-M4)
  Sensor-Frontend: Erfassung, Aufbereitung und Bereitstellung aller Fahrzeugsignale.
- **ST Discovery Kit STM32H747I-DISCO** (STM32H747XI, Dual-Core Cortex-M7 + Cortex-M4, 4.3" TFT)
  Anzeige-Einheit: Rendert das digitale Armaturenbrett auf dem integrierten TFT-LCD.
- **Seeed Studio XIAO ESP32-C3** (RISC-V, WiFi/BLE)
  Wireless-Bridge: Stellt die Messdaten via BLE (und optional WiFi) für externe Geräte
  (z.B. Smartphone) zur Verfügung.

### 2.2 Eigenentwickelte Hardware

- **`measurement-circuits`** -- Sammlung der Messschaltungen (Spannungsteiler, Optokoppler-
  Eingangsstufe für die Drehzahl, Pull-Up-Beschaltungen für NTC / Hebelgeber, Schutzbeschaltung
  gegen Transienten aus dem Bordnetz).
- **`t3-harness-adapter`** -- Adapter zwischen dem originalen VW T3 Kabelbaum-Stecker am
  Armaturenbrett und der Nucleo-Plattform inkl. der Messschaltungen.
- **`disco-tft-adapter`** -- Adapter zwischen dem Discovery Kit und den im Fahrzeug verbauten
  Bedienelementen / Versorgung.

### 2.3 Schnittstellen

- **Bordnetz:** 12 V (Klemme 15 / 30), Chassis-GND
- **Nucleo &harr; Disco:** UART (zukünftig optional CAN)
- **Nucleo &harr; ESP:** UART
- **ESP &harr; Host (Handy/PC):** BLE GATT
- **Disco &harr; TFT:** RGB / MIPI-DSI (boardseitig vorhanden)

## 3. Anforderungen

### 3.1 Muss-Anforderungen

- Ausmessen aller signale aus 1.
  - Wassertemperatur
  - Drehzahl
  - Batteriespannung
  - Tankfüllstand
  - Blinker
  - Fernlicht
  - Öldruckschalter 0.3 bar
  - Öldruckschalter 1.8 bar
- die Signale werden mit fixer Samplingrate ausgemessen (z.B. 10 Hz)
- die Signale werden geeignet gefiltert
- die Signale werden über UART vom Nucleo Board auf ein Discovery-Kit und ein ESP32 übertragen
- das ESP32-C3 publiziert die Messsignale über Bluetooth Low Energy (BLE)
- das Discovery-Kit stellt die Messsignale auf einem TFT LCD dar

### 3.2 Soll-Anforderungen

- die komplette Software soll portabel verfasst sein, für eine einfache Integration auf custom board
- die Auslastung der CPUs soll stetig überprüft werden
- die Anzeige auf dem TFT LCD soll an einen originalen T3 Tacho angeglichen sein
- das Projekt soll modular aufgebaut sein, erlaubt zukünftige Erweiterungen (weitere Sensoren / Instrumente)

### 3.3 Kann-Anforderungen

- die TFT LCD Anzeige ist konfigurierbar
- die TFT LCD Anzeige lässt sich kalibrieren
- Konfiguration und Kalibration können persistent gespeichert werden
- eine Android App stellt die über BLE empfangenen Werte auf einem Dashboard dar

## 4. Systemübersicht

Das System ist als verteiltes Embedded-System aufgebaut. Die Sensorerfassung, die
Visualisierung und die drahtlose Anbindung sind bewusst auf drei separate Plattformen verteilt,
um Verantwortlichkeiten klar zu trennen und unabhängige Entwicklung im Team zu ermöglichen.

![System Overview](img/System%20Overview.drawio.svg)

Die Nucleo-Plattform ist die zentrale Datenquelle. Sie liest alle Fahrzeugsignale ein,
normalisiert sie zu physikalischen Grössen und stellt sie zyklisch zur Verfügung. Discovery
Kit und ESP32-C3 sind reine Konsumenten dieser Daten und tauschen mit der Nucleo nur
strukturierte Telemetrie-Frames aus.

Der gesamte Code soll portabel gestaltet werden. In einem finalen Produkt wird das verteilte System
zusammengeführt auf einer einzelnen Hauptplatine. Dies ist jedoch nicht im Rahmen des Projektes.

### 4.1 Nucleo -- Sensor-Frontend

![Nucleo](img/Nucleo.drawio.svg)

Auf der Nucleo läuft die komplette Signalverarbeitungspipeline. Diese ist als FreeRTOS-Anwendung
auf dem Cortex-M7 (CM7) und Cortex-M4 (CM4) realisiert und in vier klar getrennte Stufen
aufgeteilt:

- **Acquisition** -- Erfassung der Rohwerte aus den Peripherien:
  ADC für Wassertemperatur, Tankfüllstand und Batteriespannung; Timer im Input-Capture-Modus
  für die Drehzahl-Pulse; GPIO mit Interrupt für die Schalter (Blinker, Fernlicht,
  Öldruck 0.3 / 1.8 bar).
- **Conversion** -- Umrechnung der Rohwerte in physikalische Grössen anhand der in
  [sensor-measurements.md](sensor-measurements.md) beschriebenen Kennlinien (Spannungsteiler,
  Beta-Gleichung für NTC, lineare Kennlinie für den Hebelgeber, Frequenz/4 für die Drehzahl).
- **Filtering** -- Glättung der zeitdiskreten Messwerte (z.B. gleitender Mittelwert,
  Hysterese für Schaltsignale, Plausibilitätsprüfung).
- **Publication** -- Serialisierung der gefilterten Werte zu einem Telemetrie-Frame und
  zyklische Ausgabe über die UART-Schnittstellen an Disco und ESP.

Die beiden Cores teilen sich die Arbeit: Der CM4 übernimmt die zeitkritische Acquisition, die Conversion und das Filtering.
Der CM7 übernimmt lediglich die Publication via UART. Die Inter-Core-Kommunikation erfolgt über den Shared-SRAM.

### 4.2 Discovery Kit -- Anzeige

![Discovery Kit](img/Discovery-Kit.drawio.svg)

Das Discovery Kit STM32H747I-DISCO ist über den `disco-tft-adapter` mit dem TFT-LCD
verbunden und stellt das digitale Armaturenbrett dar. Es empfängt zyklisch Telemetrie-Frames
von der Nucleo via UART, dekodiert diese und aktualisiert die Anzeige.

Die Renderpipeline basiert auf TouchGFX / LTDC und nutzt den integrierten Framebuffer im SDRAM.
Sofern es die Rechenleistung zulässt ist der CM7 für die gesamte Funktionalität verantwortlich: Vom UART-Empfang, frame parsing hin zum Rendering des UIs

Folgende Anzeigen sind vorgesehen:

- Drehzahlmesser (analog dargestellt)
- Wassertemperatur (analog dargestellt)
- Tankfüllstand (analog dargestellt)
- Batteriespannung (digital)
- Statusicons für Blinker, Fernlicht, Öldruckwarnung

Folgende Flows müssen realisiert werden:

- UART receiver
- Frame parser
- UI Manager
- DSI TFT Driver

### 4.3 ESP32-C3 -- Wireless-Bridge

Der ESP32-C3 empfängt die gleichen Telemetrie-Frames wie das Discovery Kit über UART
und stellt sie via BLE GATT als Notify-Characteristic zur Verfügung. Ein Smartphone oder
ein Laptop kann sich verbinden und die Daten z.B. in einer Telemetrie-App live mitlesen
oder zu Diagnosezwecken aufzeichnen.

Auf dem ESP läuft FreeRTOS (ESP-IDF) mit zwei Tasks:

- **UART-RX-Task** -- empfängt Frames von der Nucleo, validiert und puffert sie.
- **BLE-Task** -- sendet die zuletzt empfangenen Daten als BLE-Notification an den
  verbundenen Client.

Optional kann der ESP zusätzlich einen WiFi-Stack starten, um die Daten via MQTT oder
HTTP an einen Telemetrie-Server zu pushen.

## 5. Planung

Siehe Anhang.

## 6. Quellen

- Volkswagen AG, *VW T3 Stromlaufplan*, Werkstatthandbuch
- Volkswagen AG, *Reparaturleitfaden VW Transporter -- Elektrische Anlage*, Werkstatthandbuch
- Dieter Korp, *Jetzt helfe ich mir selbst -- VW Transporter T3*, Motorbuch Verlag
- T3-Pedia, *VW1301 Widerstandskennwerte und -anwendung*,
  https://www.t3-pedia.de/index.php?title=VW1301-Widerstandskennwerte_und_-anwendung,
  aufgerufen am 23.05.2026
- STMicroelectronics, *RM0399 Reference manual STM32H745/755 and STM32H747/757 advanced
  Arm-based 32-bit MCUs*, https://www.st.com/resource/en/reference_manual/rm0399-stm32h745755-and-stm32h747757-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- STMicroelectronics, *UM2411 Discovery kit with STM32H747XI MCU*,
  https://www.st.com/resource/en/user_manual/um2411-discovery-kit-with-stm32h747xi-mcu-stmicroelectronics.pdf
- STMicroelectronics, *UM2407 STM32H7 Nucleo-144 boards (MB1363)*,
  https://www.st.com/resource/en/user_manual/um2407-stm32h7-nucleo144-boards-mb1363-stmicroelectronics.pdf
- Espressif Systems, *ESP32-C3 Technical Reference Manual*,
  https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf
- FreeRTOS, *FreeRTOS Kernel V11.3.0 Documentation*, https://www.freertos.org/


