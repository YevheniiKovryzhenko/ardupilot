# Node 80 — L431 spray sensors

## Build

From the modified ArduPilot root:

```sh
./waf configure --board Aircraft-L431-80
./waf AP_Periph
```

Upload `build/Aircraft-L431-80/bin/AP_Periph.bin` through the installed MatekL431 bootloader. This is currently a **hardware allocation/bring-up target**, not working spray telemetry firmware. It enables the pressure and battery-voltage ADC channels and reserves the flow edge-input pin. Pressure/voltage/current acquisition and scaling, flow pulse counting/totalization, and their CAN publication/PX4 reception still require implementation. Pressure, flow, accumulated volume, and spray status will use standard `uavcan.protocol.debug.KeyValue` messages; battery voltage and FRS current will share standard `uavcan.equipment.power.BatteryInfo` for native PX4 battery monitoring. Those publications are not implemented yet.

Local target build with VB2 enabled passed with ARM GCC 13.2.1: 117,969 bytes flash used, 103,208 bytes free. This verifies compilation, not hardware behavior.

## Connections

| Connected hardware pin | L431 physical label | MCU / software mapping |
|---|---|---|
| Pressure transducer A / output | Curr | PA1 / ADC input identifier 6 |
| 150 Ω resistor, signal end | Curr junction | Same junction as sensor A and ADC |
| 150 Ω resistor, ground end | G / GND | Sensor supply return and node common ground |
|
| FM838 Deutsch B / white signal, after level conversion | 5 | PA15 / GPIO54 / digital edge input; not PWM output |
| FM838 Deutsch C / black ground | G / GND | Sensor supply return and node common ground |
|
| FRS 2000-S Uout, through voltage divider | Curr2 | PB1 / ADC input identifier 16; reserved, not enabled in current target |
| FRS divider lower resistor / sensor 0 V supply return | G / GND | Common signal ground |
|
| PM20S-2 divided battery-voltage sense output | VB2 | PB0 / ADC input identifier 15; channel enabled, sampling/telemetry pending |
| PM20S-2 signal ground | G / GND | Common signal ground |
|
| Flight-controller CAN H | CAN H | PB9 TX / CAN1 |
| Flight-controller CAN L | CAN L | PB8 RX / CAN1 |
| Flight-controller CAN ground | G / GND | Common ground |

Sensor power is supplied separately. Pressure sensor B receives its specified supply; C remains disconnected. The two-wire pressure loop returns through the 150 Ω shunt. Nominal 4–20 mA produces 0.6–3.0 V; do not connect 12 V directly to Curr. The basic shunt-only interface is the agreed provisional circuit; additional fault protection is not included.

The FM838 Deutsch pinout is **A red: supply, B white: signal, C black: ground**; it is different from the pressure sensor pinout. The local TeeJet bulletin specifies a 50% square wave and 4.5–16 V supply, but does not specify the output-high voltage or output stage. Confirm the actual signal before selecting divider values. A divider designed for a presumed 12 V signal could fail to reach a valid logic HIGH if the signal is actually lower. Present only a valid 0–3.3 V signal to the node.

The **LEM FRS 2000-S** uses a voltage output, unlike the TeeJet pressure sensor's 4–20 mA loop. Supply it separately with its specified **10–28 V DC** (nominal 12 or 24 V), not 5 V. Its bidirectional signal is centered near **2.5 V at zero current**, with a nominal excursion of ±2 V across its ±6000 A measuring range. Do not connect Uout directly to the 3.3 V ADC and do not use the pressure sensor's 150 Ω shunt on this voltage output.

Proposed divider: `Uout → 10 kΩ → Curr2 junction → 15 kΩ → GND`, using 1% or better resistors. This scales a 0–5 V signal to 0–3.0 V, with nominal zero current near 1.5 V at the ADC. Use signal names from the exact sensor/connector documentation; reserved pins must remain disconnected. Curr2 is allocated here only: the current board target enables pressure and battery-voltage ADCs, but not Curr2. Adding this ADC channel, calibration/current conversion, and telemetry requires a later firmware change. Uref measurement can be added later if needed for offset tracking.

This 2000 A nominal sensor is a large-range device: at a 3.3 V, 12-bit ADC and the proposed divider, nominal quantization is approximately **2 A per count** before averaging. Check the required current resolution for the pump/nozzles before treating it as a precise low-current monitor. Source: [LEM FRS-Sxx datasheet](https://www.lem.com/sites/default/files/products_datasheets/frs_1000___3000-sxx__v5.pdf).

The PM20S-2 already contains a 25 kΩ / 1 kΩ divider for measuring its main battery input. Connect its **divided voltage-sense output directly to VB2**, plus common ground; no additional divider is needed. Do not connect raw battery voltage to VB2 or route this already-divided signal through the L431 Vbat pad's additional divider. Nominal scaling is `Vbattery = 26 × VADC`: 72 V produces approximately 2.769 V; 84 V produces approximately 3.231 V. Verify the actual sense output and maximum charged-battery voltage before connecting the node, then calibrate the voltage multiplier against a multimeter. This measures battery input voltage, not either regulated BEC output. Source: [Matek PM20S-2 specifications](https://www.mateksys.com/?portfolio=pm20s-2).

Physical pad 5 is configured as an ordinary GPIO input, with no timer output or PWM generation. Pulse counting uses digital edges and does not inherently require hardware PWM capture. PA15 has its own interrupt line; firmware still needs to attach the edge handler. The ADC and pulse input do not compete for a PWM timer. The remaining output pads are unassigned in this sensing target.

## Embedded parameters

| Parameter | Value | Purpose |
|---|---:|---|
| `CAN_NODE` | 80 | Spray sensor identity |
| `CAN_BAUDRATE` | 1000000 | CAN nominal bitrate |
| `CAN_SLCAN_CPORT` | 0 | Disable serial passthrough |

Saved parameters override embedded defaults. No sensor conversion/calibration parameters exist yet in this target; they must be added with the application driver rather than presented as working settings now.

## Sensor diagrams and equations

- [TeeJet pressure: current-loop shunt, power rating, and pressure equations](sensors/teejet-pressure/README.md)
- [FM838 flow: conditional pulse divider and pulse-to-flow equations](sensors/fm838/README.md)
- [FRS 2000-S current: analog divider and current equations](sensors/frs-2000-s/README.md)

## Scaling for the sensor implementation

For a measured ADC voltage V across 150 Ω: `current_mA = 1000 * V / 150`, then `pressure_bar = (current_mA - 4) * 10 / 16`. Preserve out-of-range and disconnected readings as faults rather than silently reporting valid zero pressure.

The available local FM838 bulletin lists **362 pulses/litre** and 2–38 litres/minute. At the listed upper flow, this is approximately **229 pulses/second**, readily within an interrupt-counting design's requirements on this MCU. Use the actual unit's calibration label and field calibration as the final coefficient; other FM838 revisions have different ratings. Compute `flow_L_min = frequency_Hz * 60 / pulses_per_litre` and accumulate total volume from counted pulses.

Source: TeeJet bulletin `98-01628-ENUS-R0_A4LT-FM838-TurbineFlowmeter-TB.pdf` in the supplied `DataSheets/Pump_and_Spray` folder, plus the actual pressure-sensor label photos (16-05015, 8–28 V, 4–20 mA, 0–10 bar). Bench validation remains outstanding.

## Items to complete

- [ ] Implement pressure ADC acquisition, shunt-based scaling, calibration parameters, and out-of-range/disconnection handling.
- [ ] Implement FM838 edge counting, flow rate, volume totalization, zero-flow handling, and a configurable pulses-per-litre coefficient.
- [ ] Confirm FM838 output voltage/stage and select/test the signal-level interface before connecting it to the node.
- [ ] Enable the Curr2 ADC channel for FRS 2000-S and implement divider scaling, zero offset, sensitivity, polarity, and current telemetry.
- [ ] Implement VB2 battery-voltage acquisition with a configurable multiplier (nominal 26) and calibrate it against a multimeter.
- [ ] Publish pressure, flow, volume, and spray status through standard KeyValue messages; implement the matching PX4 spray-data receiver.
- [ ] Publish measured voltage and FRS current together through standard BatteryInfo; configure PX4’s native battery subscription/monitor selection and avoid duplicate battery instances.
- [ ] Calibrate pressure against a reference gauge, flow against a measured volume, and current against a reference measurement; check whether FRS resolution meets the application need.
- [ ] Bench-test all inputs together, including supply variation, sensor disconnects, stale data, and CAN errors.
