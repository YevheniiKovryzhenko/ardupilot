# Node 70 — G474 avionics

## Build

Run from this modified ArduPilot checkout (currently `/home/jack/ardupilot/ap_periph/ardupilot`), with the ArduPilot ARM toolchain and recursive submodules installed:

```sh
./waf configure --board Aircraft-G474-70
./waf AP_Periph
```

Upload `build/Aircraft-G474-70/bin/AP_Periph.bin` through the existing MatekG474 CAN bootloader. This target inherits the modified sensor drivers from `MatekG474-Periph`, adds CAN mirroring, and embeds node-70 defaults. Its hardware/bootloader board ID remains **1170**. A stock image does not provide the paired SF45 raw-sample extension.

CAN1 connects to the Cube; CAN2 connects to Here4 Blue. Both run **classic CAN at 1 Mbit/s**, with bidirectional frame mirroring. Here4 retains its own source ID; its messages are not republished as node 70. Do not also connect these segments together externally: that creates a forwarding loop.

Local build validation: `./waf configure --board Aircraft-G474-70` and `./waf AP_Periph -j4` passed with ARM GCC 13.2.1: 217,481 bytes flash used, 269,880 bytes free. Hardware operation has not yet been validated.

## Parameters

These values are embedded in the target's `defaults.parm`; the table also serves as the manual configuration checklist for a matching feature build. Saved parameters override embedded defaults. This target inherits parameter format version 4: upgrading from another version erases saved parameters, but an existing version-4 installation must be reset deliberately or have these values written and saved. Save remote changes and reboot before use.

| Parameter | Value | Purpose |
|---|---:|---|
| `CAN_NODE` | 70 | Aircraft avionics identity |
| `CAN_BAUDRATE` | 1000000 | CAN1 nominal bitrate, bit/s |
| `CAN_PROTOCOL` | 1 | CAN1 DroneCAN |
| `CAN2_BAUDRATE` | 1000000 | Here4 segment bitrate, bit/s |
| `CAN2_PROTOCOL` | 1 | CAN2 DroneCAN |
| `CAN_MIRROR_PORTS` | 3 | Mirror both ports (bits 0 and 1) |
| `CAN_SLCAN_CPORT` | 0 | Disable serial CAN passthrough |
| `GPS1_TYPE` | 0 | Disable local UART GPS; Here4 travels through mirroring |
| `GPS_PORT` | 2 | Unused UART GPS assignment |
| `ARSPD_TYPE` | 1 | MS4525 airspeed backend available on I2C |
| `PRX_PORT` | 1 | SF45/B UART1 |
| `PRX_BAUDRATE` | 921600 | SF45/B baud |
| `PRX1_TYPE` | 8 | LightWare SF45/B |
| `PRX1_ADDR` | 0 | SF45 sensor ID |
| `RNGFND_PORT` | 4 | US-D1 UART4 |
| `RNGFND_BAUDRATE` | 115200 | US-D1 baud |
| `RNGFND1_TYPE` | 11 | US-D1 serial backend |
| `RNGFND1_MIN` | 0.5 | US-D1 minimum valid range, m |
| `RNGFND1_MAX` | 50.0 | US-D1 maximum valid range, m |
| `RNGFND_MAX_RATE` | 100 | Range publication rate limit, Hz |

Set Here4's bitrate to 1 Mbit/s and verify its unique node ID separately; node 70's defaults do not configure another device. PX4 receiver settings are in [px4.md](px4.md).

PWM output functions `OUT1_FUNCTION` through `OUT11_FUNCTION` are all **0** (disabled). No actuator is assigned by this avionics preset.

## Connections

Use the board's **silkscreen labels**, not inferred connector numbering. MCU names below come from `MatekG474/hwdef.inc`; CAN MCU pins are behind transceivers and must not be used as external CAN wiring points.

| Connected hardware pin | G474 physical label | MCU / software mapping |
|---|---|---|
| Pixhawk CAN1 H | CAN1 H | PA12 TX / CAN1 |
| Pixhawk CAN1 L | CAN1 L | PA11 RX / CAN1 |
| Pixhawk CAN1 GND / BEC GND | CAN1 G / GND | Common ground |
| Separate BEC + via CAN1 harness | CAN1 5V | Node supply, regulated 4.5–5.5 V |
|
| Here4 CAN H | CAN2 H | PB6 TX / CAN2 |
| Here4 CAN L | CAN2 L | PB5 RX / CAN2 |
| Here4 GND | G / GND | Ground |
|
| SF45 TXD (yellow) | R1 / RX1 | PA10 / USART1_RX, `PRX_PORT=1` |
| SF45 RXD (orange) | T1 / TX1 | PA9 / USART1_TX |
| SF45 GND | G / GND | Ground |
|
| US-D1 TX (green) | R4 / RX4 | PC11 / UART4_RX, `RNGFND_PORT=4` |
| US-D1 RX (white) | T4 / TX4 | PC10 / UART4_TX |
| US-D1 GND (black) | G / GND | Ground |

This wiring assumes **SF45/B** and the **UART-output US-D1**. Both serial interfaces use 3.3 V logic. Check the actual sensor harness before wiring. Each sensor is powered separately; retain the common ground connections shown above. Power the node from a separate regulated 5 V BEC through the CAN1 harness power conductor. Leave the Pixhawk CAN1 +5 V conductor disconnected from that BEC-fed rail to avoid tying the two supplies together; keep CAN H, CAN L, and ground connected. For Here4, do not infer connector pin numbers or supply capacity from the CAN signal labels. The Cube carrier board determines its connector numbering.

Follow the [CAN wiring and termination guide](can-wiring.md) for endpoint resistor placement, loose-resistor installation, and power-off resistance tests. CAN1 and CAN2 each need their own two endpoint terminators; Here4 termination does not terminate CAN1. No second physical path may join the mirrored buses. Disconnect SF45 from UART1 during UART-based board recovery.

## Update and verify

For routine updates, use DroneCAN GUI/Mission Planner and a CAN adapter at 1 Mbit/s to load the `.bin`. For initial installation or bootloader recovery, use Matek's DFU procedure and the combined HEX, not a guessed `.bin` address.

Alternatively, PX4 can serve the `.bin` from the Cube microSD card: copy it to the card root, set `UAVCAN_ENABLE=2`, reboot, then run `uavcan update` if necessary. PX4 stores this hardware image as `/fs/microsd/ufw/1170.bin`. **Isolate the intended node:** the updater matches hardware board ID 1170, not node ID 70, and can overwrite other G474 roles. Return to `UAVCAN_ENABLE=1` after updating if automatic allocation/update is unwanted.

Check `uavcan status` for node 70 and the independent Here4 ID, then follow the reception checks in [px4.md](px4.md). Bench validation must include GPS traffic across CAN2, SF45 scan directions/expiry, and US-D1 range reception.

## References

- [Matek CAN-G474 pinout and recovery](https://www.mateksys.com/?portfolio=can-g474)
- [LightWare SF45/B guide](https://lightwarelidar.com/wp-content/uploads/2025/07/SF45B-Product-Guide-v3.pdf)
- [US-D1 UART manual](https://ainstein.ai/wp-content/uploads/US-D1-Technical-User-Manual-D00.02.05.docx.pdf)

## Items to complete

- [ ] Validate US-D1 readings against its 0.5–50 m specification and the manufacturer's SNR requirement (>13 dB); the existing UART driver does not implement SNR rejection. Saved range limits override the new preset.

- [ ] Apply and read back the installation parameters on both AP_Periph and PX4; verify unique node IDs and both CAN segments.
- [ ] Bench-test SF45/B, US-D1, and Here4 simultaneously, including data freshness, CAN errors, and disconnect/reconnect behavior.
- [ ] Set and verify SF45 mounting orientation/yaw and scan limits; check radar distances and installation offsets.
- [ ] Calibrate the Here4 compass if used, configure GPS installation offsets, and verify GPS reception across the forwarding node.
- [ ] If airspeed is installed, configure PX4 reception, perform zero-pressure calibration, and verify scale.
