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
LightWare SF45/B to PX4 DroneCAN integration: build and update the CAN-G474
AP_Periph image, configure the sensor UART and node ID, and confirm the paired
PX4 link. PX4 build and receiver parameters are documented in the PX4 checkout
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
CAN using PX4” in `src/drivers/distance_sensor/lightware_sf45_can/README.md`
in the PX4 checkout for the SD-card and `UAVCAN_ENABLE` steps.

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
1 Mbit/s and terminate the bus at its two physical ends. The board's default
UART assignments are port 1 for RC, port 2 for GPS, port 3 for MSP, and port 4
for a rangefinder. This example retains GPS and rangefinder on their default
ports and connects SF45 to USART3, serial port 3 (MCU PB10/TX and PB11/RX).
Cross TX/RX and share signal ground. USART3 is marked `NODMA` but is suitable
for this stream. Set `MSP_PORT=-1` to release port 3 from its default MSP use.

PX4 can set AP_Periph's remote parameters through its MAVLink Console. First
enable PX4's manual DroneCAN sensor mode and reboot it as described in the PX4
guide. In the console, run `uavcan status` and note the Matek node's current
ID. Substitute that current ID for `10` below; it remains the command target
while `CAN_NODE` is changed:

```text
uavcan param set 10 CAN_NODE 42
uavcan param set 10 CAN_BAUDRATE 1000000
uavcan param set 10 PRX1_TYPE 8
uavcan param set 10 PRX1_ADDR 0
uavcan param set 10 PRX_PORT 3
uavcan param set 10 PRX_BAUDRATE 921600
uavcan param set 10 MSP_PORT -1
uavcan param save 10
```

Choose an unused node ID instead of 42 if needed, and set PX4's `SF45C_NODE`
to the same value. Keep the PX4 node ID different. `CAN_BAUDRATE` must match
PX4's `UAVCAN_BITRATE` (normally 1000000). `PRX1_TYPE=8` selects AP's LightWare
SF45/B proximity backend and `PRX1_ADDR=0` selects sensor instance 0.
`PRX_BAUDRATE` must match the baud stored in SF45; 921600 matches the dedicated
PX4 UART driver's default host rate if the sensor has not been configured to
another value. `PRX_PORT=3` selects USART3. Save and power-cycle the CAN-G474
after changing its node ID or UART settings.

If the Matek node is not visible to PX4, configure it first with a USB-to-CAN
adapter and DroneCAN GUI Tool. PX4's remote parameter commands are
`uavcan param list <node-id>`, `uavcan param get <node-id> <name>`,
`uavcan param set <node-id> <name> <value>`, and
`uavcan param save <node-id>`.

#### Optional Ainstein US-D1 on UART port 4

If the US-D1 is connected to port 4, enable its existing AP serial rangefinder
backend before saving the node parameters:

```text
uavcan param set 10 RNGFND1_TYPE 11
uavcan param set 10 RNGFND_PORT 4
```

Replace `10` with the Matek node's current ID. The AP US-D1 backend uses
115200 baud. These parameters are separate from the SF45 proximity settings.
With PX4's UAVCAN sensor mode enabled, PX4 receives the US-D1 through its
normal DroneCAN range-sensor bridge; no SF45-specific parameter configures it.

### What differs from stock AP_Periph

The firmware remains AP_Periph for the MatekG474-Periph target. This
integration adds a board-enabled raw-sample callback to AP's existing SF45/B
UART parser. For SF45 proximity instance 0, AP_Periph sends each new
sensor-frame distance and yaw in the existing DroneCAN `Proximity` message
with `flags=1`. Other proximity types retain their normal `flags=0` coarse
boundary publication. This preserves individual scan angles instead of
reducing them to AP's eight 45-degree boundary sectors.

The SF45 backend requests update-rate code 6 (129 readings/s) and streams
first-return distance and yaw. This rate is fixed in the backend; there is no
AP_Periph rate parameter in this integration. Scan limits remain configured
in the SF45 itself. AP_Periph also preserves the sender's 20 ms CAN transfer
deadline when frames enter the HAL queue; other senders keep their default
timeout.

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
