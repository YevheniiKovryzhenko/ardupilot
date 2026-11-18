# Node 71 — G474 spray actuators

## Build

From the modified ArduPilot root:

```sh
./waf configure --board Aircraft-G474-71
./waf AP_Periph
```

Upload `build/Aircraft-G474-71/bin/AP_Periph.bin` using the existing MatekG474-Periph bootloader. Hardware board ID remains **1170**: isolate this node during automatic board-ID-based updates so other G474 roles are not overwritten. Saved parameters override defaults; write the preset or deliberately reset an existing installation. No parameter-format bump is used to erase another role's settings automatically.

Local target build with forwarding enabled passed with ARM GCC 13.2.1: 137,725 bytes flash used, 349,632 bytes free. This verifies compilation, not hardware behavior.

## Allocation

Use a dedicated actuator node rather than assigning spray outputs to avionics node 70. This preserves physical placement flexibility and keeps spray wiring and commissioning separate. The three ESC signals share **TIM2** and therefore one conventional PWM rate. The two valve outputs are static GPIO, not PWM. Pads 4 and 7–11 are not configured by this target.

| Connected hardware pin | G474 physical label | MCU / software mapping |
|---|---|---|
| Pump ESC signal | 1 | PA0 / TIM2_CH1 / OUT1 |
| Pump ESC ground | G / GND | Common signal ground |
|
| Nozzle 1 ESC signal | 2 | PA1 / TIM2_CH2 / OUT2 |
| Nozzle 1 ESC ground | G / GND | Common signal ground |
|
| Nozzle 2 ESC signal | 3 | PA2 / TIM2_CH3 / OUT3 |
| Nozzle 2 ESC ground | G / GND | Common signal ground |
|
| Valve 1 BEC EN | 5 | PA6 / GPIO54 / RELAY1 / hardpoint ID 0 |
| Valve 1 BEC ground | G / GND | Common signal ground |
|
| Valve 2 BEC EN | 6 | PA7 / GPIO55 / RELAY2 / hardpoint ID 1 |
| Valve 2 BEC ground | G / GND | Common signal ground |
|
| Flight-controller CAN H | CAN1 H | PA12 TX / CAN1 |
| Flight-controller CAN L | CAN1 L | PA11 RX / CAN1 |
| Flight-controller CAN ground | G / GND | Common ground |
|
| Forwarded CAN segment H | CAN2 H | PB6 TX / CAN2 |
| Forwarded CAN segment L | CAN2 L | PB5 RX / CAN2 |
| Forwarded CAN segment ground | G / GND | Common ground |

BEC EN connections remain provisional pending electrical compatibility and power-sequence checks discussed for BEC20S-PRO. GPIO LOW commands BEC shutdown; actual EN behavior while the MCU is resetting or unpowered must be tested. All motor and valve power is supplied separately. Both CAN ports run DroneCAN at 1 Mbit/s with bidirectional frame forwarding. Terminate each physical segment separately; do not add another physical or forwarding path joining the same segments, which would create a loop. See the [CAN wiring guide](can-wiring.md).

## Embedded parameters

| Parameter | Value | Purpose |
|---|---:|---|
| `CAN_NODE` | 71 | Spray actuator identity |
| `CAN_BAUDRATE` | 1000000 | CAN1 nominal bitrate |
| `CAN_PROTOCOL` | 1 | DroneCAN |
| `CAN2_BAUDRATE` | 1000000 | CAN2 nominal bitrate |
| `CAN2_PROTOCOL` | 1 | CAN2 DroneCAN |
| `CAN_MIRROR_PORTS` | 3 | Bidirectional CAN1/CAN2 forwarding |
| `CAN_SLCAN_CPORT` | 0 | Disable serial passthrough |
| `OUT_RATE` | 50 | Shared PWM rate, Hz; provisional until ESC rate requirement is confirmed |
| `ESC_PWM_TYPE` | 1 | Conventional PWM |
| `OUT1_FUNCTION` | 55 | RCIN5: wire actuator ID 5, pump |
| `OUT2_FUNCTION` | 56 | RCIN6: wire actuator ID 6, nozzle 1 |
| `OUT3_FUNCTION` | 57 | RCIN7: wire actuator ID 7, nozzle 2 |
| `OUT1_MIN`, `OUT2_MIN`, `OUT3_MIN` | 1000 | Minimum command pulse, µs |
| `OUT1_MAX`, `OUT2_MAX`, `OUT3_MAX` | 2000 | Maximum command pulse, µs |
| `OUT1_TRIM`, `OUT2_TRIM`, `OUT3_TRIM` | 1500 | Midpoint for unitless command conversion, µs; not a shutdown command |
| `SRV_CMD_TIME_OUT` | 200 | Stop PWM pulses after actuator-command timeout, ms |
| `RELAY1_FUNCTION` | 10 | DroneCAN hardpoint ID 0 |
| `RELAY1_PIN` | 54 | Pad 5 GPIO |
| `RELAY1_DEFAULT` | 0 | Start LOW |
| `RELAY1_INVERTED` | 0 | ON = HIGH |
| `RELAY2_FUNCTION` | 11 | DroneCAN hardpoint ID 1 |
| `RELAY2_PIN` | 55 | Pad 6 GPIO |
| `RELAY2_DEFAULT` | 0 | Start LOW |
| `RELAY2_INVERTED` | 0 | ON = HIGH |

## Command integration and commissioning

The ESCs use DroneCAN `actuator.ArrayCommand`, not propulsion `esc.RawCommand`, to avoid mapping them to flight motor slots. AP_Periph subtracts one from each wire actuator ID; the current PX4 servo publisher sends zero-based IDs. Wire IDs **5, 6, 7** therefore correspond to PX4 servo output array slots **5, 6, 7** (the sixth, seventh, eighth positions). These are proposed allocations, not confirmed reservations on the vehicle; audit existing command IDs and configure the PX4 spray-control source before use. The PWM functions' RCIN names identify command mappings here, not a requirement for an RC receiver attached to this node.

Valve commands use DroneCAN `hardpoint.Command` IDs **0 and 1**, values **0/1**. The existing relay receiver latches its last state and has no communication-loss timeout. PX4's current hardpoint controller stores/repeats only its latest hardpoint command; a two-valve application must explicitly manage both outputs. PX4 spray-control integration and an application-level shutdown policy are not implemented by these board files.

Bench-check with pump/nozzle motors unable to cause motion or spray: verify pulse width, frequency, both valve GPIO states, arming/disarming, reset, and CAN disconnect. Motor PWM timeout suppresses pulses; it is not a verified ESC stop response. Test that behavior with the actual ESCs. No hardware validation has been performed.

## Items to complete

- [ ] Audit the vehicle-wide actuator/hardpoint IDs and configure PX4 commands for the pump, both nozzles, and both valves; explicitly manage both hardpoint outputs.
- [ ] Implement and test the valve communication-loss shutdown policy; current GPIO commands latch their last state.
- [ ] Confirm BEC20S-PRO EN electrical compatibility, output shutdown delay, and behavior during node startup, reset, and loss of node power.
- [ ] Bench-verify the ESC pulse range/rate, endpoints where calibration is required, arming/disarming, and motor stop response when PWM pulses cease.
- [ ] Verify embedded defaults against saved parameters and test both-direction CAN forwarding with separately terminated segments and no forwarding loops.
