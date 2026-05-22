# Mathematical Evaluation of VW T3 Dashboard Sensors
This documentation describes the mathematical principles and formulas for evaluating analog sensors in the VW T3 using a microcontroller with a $3.3\,\text{V}$ signal level (e.g., ESP32 or STM32 with a 12-bit ADC).

## 1. Common Base: Resistance Measurement via Pull-Up

Since the sensors for fuel level and coolant temperature represent temperature- or level-dependent resistances against vehicle ground, they are operated via a known pull-up resistor $R_{\text{pullup}}$ connected to the stable $3.3\,\text{V}$ rail of the microcontroller.

### Voltage Divider Formula

The analog signal at the ADC pin is calculated as follows:

$$V_{\text{ADC}} = V_{\text{REF}} \cdot \frac{R_{\text{sense}}}{R_{\text{pullup}} + R_{\text{sense}}}$$

Conversely, the current sensor resistance $R_{\text{sense}}$ is calculated in the microcontroller from the measured voltage $V_{\text{ADC}}$:

$$R_{\text{sense}} = \frac{V_{\text{ADC}} \cdot R_{\text{pullup}}}{V_{\text{REF}} - V_{\text{ADC}}}$$

- $V_{\text{REF}} = 3.3\,\text{V}$ (Reference voltage of the microcontroller)
- $R_{\text{pullup}}$: Selected fixed resistor on the adapter board

## 2. Fuel Level

The original T3 lever-type sensor works mechanically linearly but has an inverted characteristic curve (high resistance when the tank is empty).

### Sensor Parameters

- Full (approx. 70 liters): $R_{\text{sense}} \approx 40\,\Omega$
- Empty (0 liters): $R_{\text{sense}} \approx 300\,\Omega$

### Calibrated Linear Equation (Liters as a function of $R_{\text{sense}}$)

$$L_{\text{tank}} = -0.269 \cdot R_{\text{sense}} + 80.8$$

### Complete Characteristic Curve Equation (Directly from $V_{\text{ADC}}$)

Substituting the resistance formula of the voltage divider yields the hyperbolically curved voltage equation:

$$L_{\text{tank}} = -0.269 \cdot \left( \frac{V_{\text{ADC}} \cdot R_{\text{pullup}}}{3.3 - V_{\text{ADC}}} \right) + 80.8$$

(Recommended Pull-Up: $R_{\text{pullup}} = 330\,\Omega$)

## 3. Coolant Temperature (NTC Sensor)

Negative Temperature Coefficient (NTC) thermistors exhibit a non-linear, exponential temperature behavior. The simplified Steinhart-Hart equation (Beta equation) is used for software evaluation.

### Sensor Parameters (calculated from VW specifications)

- Nominal Temperature ($T_0$): $20^\circ\text{C}$ ($293.15\,\text{K}$) $\rightarrow R_0 = 1100\,\Omega$
- Operating Temperature ($T_1$): $90^\circ\text{C}$ ($363.15\,\text{K}$) $\rightarrow R_1 = 105\,\Omega$
- Material Constant ($\beta$): $3572\,\text{K}$

### Beta Equation for Temperature Calculation

From the sensor resistance $R_{\text{sense}}$ determined via the voltage divider, the absolute temperature in Kelvin is calculated:

$$T_{\text{Kelvin}} = \frac{1}{\frac{1}{T_0} + \frac{1}{\beta} \cdot \ln\left(\frac{R_{\text{sense}}}{R_0}\right)}$$Consequently, the output in degrees Celsius is:$$T_{\text{Celsius}} = T_{\text{Kelvin}} - 273.15$$

(Recommended Pull-Up: $R_{\text{pullup}} = 470\,\Omega$)

## 4. Battery Voltage

The measurement of the board voltage is carried out via a classic voltage divider to step down the fluctuating automotive voltage to the level of the microcontroller.

### Circuit Design

- $R_1$: Resistor from Terminal 15/30 to the ADC pin (e.g., $10\,\text{k}\Omega$)
- $R_2$: Resistor from the ADC pin to ground (e.g., $2.2\,\text{k}\Omega$)

$$V_{\text{ADC}} = V_{\text{Batt}} \cdot \frac{R_2}{R_1 + R_2}$$

## Calculating the Real Battery Voltage in Code

From the voltage $V_{\text{ADC}}$ measured at the ADC, the board voltage $V_{\text{Batt}}$ is obtained:

$$V_{\text{Batt}} = V_{\text{ADC}} \cdot \frac{R_1 + R_2}{R_2}$$

With the example values ($R_1 = 10\,\text{k}\Omega$, $R_2 = 2.2\,\text{k}\Omega$), the factor simplifies to:

$$V_{\text{Batt}} = V_{\text{ADC}} \cdot \frac{12.2}{2.2} \approx V_{\text{ADC}} \cdot 5.545$$

⚠️ Protection Note: A $3.3\,\text{V}$ Zener diode should absolutely be connected in parallel with $R_2$ in order to reliably clip overvoltage transients from the vehicle electrical system.