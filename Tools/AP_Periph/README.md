# AP_Periph DroneCAN Peripheral Firmware

This is an ArduPilot based DroneCAN peripheral firmware. This firmware
takes advantage of the wide range of sensor drivers in ArduPilot to
make building a DroneCAN peripheral firmware easy.

The AP_Periph firmware is based on the same ChibiOS hwdef.dat system
that is used to define pinouts for STM32 based flight controllers
supported by ArduPilot. That means you can add support for a new
DroneCAN peripheral based on the STM32 by just writing a simple
hwdef.dat that defines the pinout of your device.

We have over 60 build targets building for AP_Periph firmwares. All
ArduPilot supported MCUs can be used, including:

- STM32F1xx
- STM32F3xx
- STM32F4xx
- STM32F7xx
- STM32H7xx
- STM32L4xx
- STM32G4xx

More can be added using the hwdef.dat system

## Features

The AP_Periph firmware can be configured to enable a wide range of
DroneCAN sensor types. Support is included for:

- GPS modules (including RTK GPS)
- Magnetometers (SPI or I2C)
- Barometers (SPI or I2C)
- Airspeed sensors (I2C)
- Rangefinders (UART or I2C)
- ADSB (uAvionix compatible Ping ADSB receiver on UART)
- Battery Monitor (Analog, I2C/SMBus, UART)
- LEDs (GPIO, I2C or WS2812 serial)
- Safety LED and Safety Switch
- Buzzer (tonealarm or simple GPIO)
- RC Output (All standard RCOutput protocols)
- RC input
- battery balance monitor
- EFI engines
- Proximity sensors

An AP_Periph DroneCAN firmware supports these DroneCAN features:

- dynamic or static CAN node allocation
- firmware upload
- automatically generated bootloader
- parameter storage in flash
- easy bootloader update
- high resiliance features using watchdog, CRC and board checks
- firmware and parameter update via MissionPlanner or DroneCAN GUI tool when attached to an autopilot
- firmware update via USB if USB port is provided
- parameter update using SLCAN and DroneCAN GUI on standalone peripheral via USB, if provided

## Building

Using f103-GPS as an example, build the main firmware like this:

- ./waf configure --board f103-GPS
- ./waf AP_Periph

that will build a file build/f103-GPS/bin/AP_Periph.bin. You can
now load that using the CAN bootloader and either dronecan_gui_tool or
MissionPlanner DroneCAN support.

## Flashing

To load directly with a stlink-v2, do this:

- st-flash write build/f103-GPS/bin/AP_Periph.bin 0x8006400

for the CUAV_GPS which loads at offset 0x10000 use this:

- st-flash write build/CUAV_GPS/bin/AP_Periph.bin 0x8010000

## Flashing bootloader

To flash the bootloader use this:

- st-flash write Tools/bootloaders/f103-GPS_bl.bin 0x8000000

## Building bootloader

To build the bootloader use this:

- Tools/scripts/build_bootloaders.py f103-GPS

the resulting bootloader will be in Tools/bootloaders

## Firmware Builds

Firmware targets are automatically built and distributed on the
ArduPilot firmware server on firmware.ardupilot.org. These firmwares
can be loaded using Mission Planner or the DroneCAN GUI Tool. Parameters
for peripherals can be changed using the Mission Planner DroneCAN support
or using DroneCAN GUI Tools.

## Matek CAN-G474 SF45/B to PX4

This section contains the complete ArduPilot-side instructions for the
Matek CAN-G474 sensor node: build and update its AP_Periph image, use the SF45/B
DroneCAN link to PX4, and configure the default GPS and ASPD-4525 airspeed
inputs. PX4 build and receiver parameters are documented in the PX4 checkout
at `src/drivers/distance_sensor/lightware_sf45_can/README.md`.

The CAN-G474 must run the modified `MatekG474-Periph` image. Stock AP_Periph on
this board does not publish the raw SF45 samples expected by the paired PX4
receiver.

### Build the correct AP_Periph image

From the modified ArduPilot checkout:

```sh
cd ~/ardupilot
source ~/venv-ardupilot/bin/activate
./waf configure --board MatekG474-Periph
./waf AP_Periph
```

The CAN bootloader image is:

```text
build/MatekG474-Periph/bin/AP_Periph.bin
```

Use this target and file; do not select a flight-controller `.apj`/`.px4` or
another AP_Periph board's `.bin`. The build also creates `AP_Periph.apj` and
`AP_Periph_with_bl.hex`; these are different formats for other update/recovery
paths, not substitutes for the normal CAN update.

### Update the Matek CAN-G474

For a normal update, attach a USB-to-CAN adapter to the same powered,
terminated 1 Mbit/s classic CAN bus as the PX4 and Matek. In DroneCAN GUI Tool,
open the bus, select the Matek node, choose its firmware update action, and
load `build/MatekG474-Periph/bin/AP_Periph.bin`. Mission Planner's DroneCAN
firmware updater can be used instead. Keep the node powered until the updater
reports success, then power-cycle it.

If you do not have a USB-to-CAN adapter, the Cube Orange+ can serve this image
over its connected CAN bus. This requires a working AP_Periph CAN bootloader;
it does not install or recover that bootloader. Follow “Update AP_Periph over
CAN using PX4” below. This path uses PX4's built-in firmware server and the
Cube's microSD card; it does not require DroneCAN GUI Tool on the computer.

#### Use PX4 as the CAN firmware server

1. Copy `build/MatekG474-Periph/bin/AP_Periph.bin` to the root of the Cube
   Orange+'s microSD card, then reinstall the card.
2. In PX4, set `UAVCAN_ENABLE=2`, keep `UAVCAN_BITRATE=1000000`, save, and
   reboot. Mode 2 enables PX4's DroneCAN firmware server. It does not route
   motor outputs over CAN. See the PX4 guide linked above for the other PX4
   receiver settings.
3. PX4 identifies this image as MatekG474 board ID `1170`, moves it to
   `/fs/microsd/ufw/1170.bin`, and serves it to connected nodes with that board
   ID when their image CRC differs. If the update does not start by itself,
   run `uavcan update` in the PX4 MAVLink Console.
4. Wait for the Matek node to reboot and reappear in `uavcan status`. PX4 may
   update every online MatekG474 node with a different image CRC, so disconnect
   other such nodes if you do not want them updated. Keep the intended image
   as `/fs/microsd/ufw/1170.bin`.
5. After updating, set `UAVCAN_ENABLE=1` and reboot if you want PX4 back in
   manual/static sensor mode. Mode 1 still receives sensors and supports
   remote AP_Periph parameter commands; mode 2 is needed for firmware updates.

The Matek must already have a working AP_Periph CAN bootloader. PX4 cannot
install or recover a missing bootloader. Use the DFU/combined HEX procedure
below for initial bootloader installation or recovery.

#### Use an ArduPilot flight controller as a USB-to-CAN adapter

An ArduPilot flight controller can bridge the computer's USB connection to
its CAN bus using SLCAN, so a separate USB-to-CAN adapter is not needed. These
settings are for the flight controller acting as the bridge, not for the
Matek CAN-G474 AP_Periph node. The flight controller must support SLCAN and
have a CAN connector wired to the same powered, terminated 1 Mbit/s bus.

1. Connect Mission Planner to the ArduPilot flight controller. In its full
   parameter list, set `CAN_P1_DRIVER=1`, `CAN_D1_PROTOCOL=1`,
   `CAN_P1_BITRATE=1000000`, and `CAN_SLCAN_CPORT=1`. Write the parameters and
   reboot. These enable the first CAN port for DroneCAN and select it for
   SLCAN passthrough.
2. Reconnect Mission Planner and set `CAN_SLCAN_SERNUM=0` and
   `CAN_SLCAN_TIMOUT=0`. `SERNUM=0` routes SLCAN to Serial0, normally the USB
   connection; it is cleared at boot, so do not reboot after setting it.
   In Mission Planner's **Initial Setup > Optional Hardware > UAVCAN
   (DroneCAN)** page, click **SLCAN Mode CAN1** to start the passthrough.
3. Close/disconnect Mission Planner so it releases the USB COM port, but leave
   the USB cable connected. Open DroneCAN GUI Tool and select the flight
   controller's USB COM port. Use a CAN bitrate of `1000000`; `115200` is a
   suitable adapter baud setting for the virtual USB serial connection.
   Enable **Local node ID** using the tool's Set/check control, and choose an
   ID that does not conflict with another node.
4. The connected nodes should appear in the GUI. Select the Matek node and
   use its firmware update action to load
   `build/MatekG474-Periph/bin/AP_Periph.bin`. Keep the node powered until the
   updater reports completion, then wait for it to restart.

ArduPilot documents the board-specific SLCAN setup in [SLCAN Access on F4
based Autopilots](https://ardupilot.org/copter/docs/common-slcan-f4.html) and
the GUI connection in [DroneCAN GUI](https://ardupilot.org/copter/docs/common-uavcan-gui.html).

This uses the node's installed AP_Periph CAN bootloader. For initial bootloader
installation or recovery, use Matek's CAN-G474 DFU procedure and the generated
`AP_Periph_with_bl.hex` with the documented STM32CubeProgrammer workflow. The
combined HEX includes the bootloader. Do not write `AP_Periph.bin` at an
assumed flash address, and do not use the recovery procedure for routine CAN
updates.

After the update, confirm that the node returns on the bus in the updater's
node information page or PX4's `uavcan status`. Record the reported software
version/commit and image CRC if available, and keep the exact `.bin` with the
source revision used to build it. A source commit alone may not identify local
uncommitted changes; the raw-sample check below confirms that the running node
has the SF45 publisher PX4 expects.

### Connect and configure the sensors

Connect CAN-H, CAN-L and common ground to a PX4 CAN port. Use classic CAN at
1 Mbit/s and terminate the bus at its two physical ends. The board maps USART1
to UART1, USART2 to UART2/GPS, USART3 to UART3/MSP, and UART4 to the rangefinder
port. This image uses UART1 for SF45/B, UART2 for GPS, and UART4 for US-D1; the
unrelated peripheral feature families remain disabled. Matek labels the UART
signals T1/R1, T2/R2, and T4/R4.
See the [Matek CAN-G474 product pinout](https://www.mateksys.com/?portfolio=can-g474).

Cross TX and RX at each sensor and share ground. For the SF45/B, connect its
yellow TXD wire to CAN-G474 RX1, orange RXD to TX1, and ground to ground. Power
the sensor from a suitable 5 V source. For the US-D1 UART version, connect its
white radar-RX wire to TX4, green radar-TX wire to RX4, black to ground, and
red to a suitable 5–13 V supply. Both sensors use 3.3 V UART logic; do not
connect either sensor's power wire to a UART signal pin. UART1 is also used by
the board's DFU update procedure, so disconnect the SF45/B from UART1 when
using a USB-TTL adapter for DFU recovery.

The long GPS connector carries UART2 and I2C2. GPS defaults to UART2
(`GPS_PORT=2`) with automatic protocol detection (`GPS1_TYPE=1`); no exact GPS
model is required for this default. The GPS-side I2C2 pins are available on
that connector, but compass support is not enabled in this image.

For the Matek ASPD-4525, connect its 5V, SCL, SDA, and ground wires to the
CAN-G474 I2C1 connector. The sensor is the TE 4525DO-DS5AI001DP; AP_Periph
defaults to `ARSPD_TYPE=1` and probes for it on the external I2C buses. I2C1
is the first external bus in this board's mapping. The sensor requires a 4–5.5V
supply. See Matek's [ASPD-4525 specifications and wiring](https://www.mateksys.com/?portfolio=aspd-4525).

The SF45/B supports I2C commands, but its continuous scan stream is available
only over UART, and this AP_Periph SF45 proximity backend is serial. I2C1 is
therefore not a substitute for UART1 here. See LightWare's [SF45/B Product
Guide](https://lightwarelidar.com/wp-content/uploads/2025/07/SF45B-Product-Guide-v3.pdf).
The US-D1 must be the UART-output version for this wiring; the vendor also
offers a CAN-output version. The [US-D1 technical manual](https://ainstein.ai/wp-content/uploads/US-D1-Technical-User-Manual-D00.02.05.docx.pdf)
lists the UART wire colors, signal level, supply range, and baud rate.

The image enables the SF45/B proximity backend (`PRX1_TYPE=8`, sensor address
0) on USART1 at 921600 baud and the US-D1 serial backend (`RNGFND1_TYPE=11`)
on UART4 at 115200 baud. It sets `CAN_NODE=42` and `CAN_BAUDRATE=1000000`, so
PX4's `SF45C_NODE` should be `42` and its `UAVCAN_BITRATE` should be
`1000000`. Keep PX4's own node ID distinct from 42. With a connected, detected
GPS or airspeed sensor, AP_Periph forwards its DroneCAN GPS or raw-air-data
messages automatically as well.

No AP_Periph parameter setup is needed after installing this image. The
MatekG474-Periph target enables GPS, MS4525 airspeed, SF45/B proximity, and the
US-D1 rangefinder. It compiles out compass, barometer, battery monitor, MSP,
ADS-B, RC input, EFI, and unused airspeed backends. Its parameter format version
is bumped for this configuration, so the first boot after updating erases saved
AP_Periph parameters and applies the new defaults. This also resets any previous
node ID or custom parameters to `CAN_NODE=42` and the sensor defaults above.

PX4 still needs its receiver settings enabled as described in the PX4 guide.
With its DroneCAN sensor bridge active, it receives the SF45 raw-sample
messages and US-D1 range measurements automatically; the US-D1 does not use
SF45-specific parameters.

### What differs from stock AP_Periph

The firmware remains AP_Periph for the MatekG474-Periph target. This
integration adds a board-enabled raw-sample callback to AP's existing SF45/B
UART parser. For SF45 proximity instance 0, AP_Periph sends each new
sensor-frame distance and yaw in the existing DroneCAN `Proximity` message
with `flags=1`. Other proximity types retain their normal `flags=0` coarse
boundary publication. This preserves individual scan angles instead of
reducing them to AP's eight 45-degree boundary sectors.

The SF45 backend now follows PX4's serial initialization sequence: it probes
the product name, sends the update-rate and output-format settings one at a
time after their acknowledgments, then enables streaming. It uses update-rate
code 5 (500 readings/s in SF45/B Product Guide Rev. 3, matching PX4's default).
The rate is fixed in the backend; there is no AP_Periph rate parameter in this
integration. Scan limits remain configured in the SF45 itself. AP_Periph also
preserves the sender's 20 ms CAN transfer deadline when frames enter the HAL
queue; other senders keep their default timeout.

`flags=1` is a paired AP/PX4 extension, not a standardized SF45 DroneCAN
profile. A generic receiver that ignores the flag can treat sensor-frame yaw
as body-frame data. Keep the modified AP_Periph sender and PX4 receiver
together.

### Confirm PX4 is receiving the SF45 samples

Complete the PX4 build and parameter steps in
`src/drivers/distance_sensor/lightware_sf45_can/README.md` in the PX4 checkout.
The bus bitrates must match, and the Matek node ID and SF45 sensor ID must
match PX4's `SF45C_NODE` and `SF45C_SENSOR`.

In PX4's MAVLink Console, check:

```text
uavcan status
lightware_sf45_can status
listener sf45_sample
listener obstacle_distance
```

The Matek should appear at its configured node ID. A growing receiver sample
count and `sf45_sample` messages with the expected node and sensor IDs confirm
PX4 is receiving the paired raw-SF45 format, not merely seeing a CAN node.
`obstacle_distance` should then show the map. If the node is visible but no
samples arrive, check that the Matek runs this modified `.bin`, the SF45 is
streaming at the configured UART baud, AP parameters select type 8 and the
correct port/address, and PX4 has the matching custom receiver build and IDs.

## User Bootloader Update

The bootloader is automatically stored in ROMFS in the main
firmware. End users can update the bootloader by setting the DroneCAN
parameter "FLASH_BOOTLOADER" to 1. After setting it to 1 the node will
respond with a debug text message which can be seen in the DroneCAN GUI
tool to show the result of the flash.

## SITL Testing

A wide range of DroneCAN peripherals are supported in the SITL
simulation system. The simplest way of starting a DroneCAN enabled
simulated vehicle is to use sim_vehicle.py.

For a quadplane use: sim_vehicle.py with the option -f quadplane-can

For a quadcopter use: sim_vehicle.py with the option -f quad-can

## Discussion and Feedback

Please join the discussions at these locations:

- [ArduPilot forum](https://discuss.ardupilot.org/)
- [ArduPilot Discord](https://ardupilot.org/discord)
