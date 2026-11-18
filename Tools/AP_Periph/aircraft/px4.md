# PX4 — Cube Orange+ receiver

## Build

Use the paired **v1.17.0-mod** checkout and its custom `surgeon` target:

```sh
cd /home/jack/PX4/v1.17.0-mod
make cubepilot_cubeorangeplus_surgeon -j4
make cubepilot_cubeorangeplus_surgeon upload
```

The generated `.px4` is in `build/cubepilot_cubeorangeplus_surgeon/`. Upload by USB with the PX4 bootloader or QGroundControl. Stock PX4 does not contain this paired SF45 CAN path. Node firmware and wiring are documented in [node-70.md](node-70.md).

## Parameters

Set these in QGroundControl or the MAVLink Console, save, and reboot. The table describes required installation settings, not a claim that all are already firmware defaults. Mounting values assume upright, forward-facing SF45.

| Parameter | Value | Purpose |
|---|---:|---|
| `UAVCAN_ENABLE` | 1 | DroneCAN sensors; use 2 temporarily for firmware server |
| `UAVCAN_BITRATE` | 1000000 | Classic CAN, matching node 70 |
| `UAVCAN_NODE_ID` | 1 | Flight-controller identity, unique on bus |
| `UAVCAN_SUB_GPS` | 1 | Subscribe to Here4 GPS |
| `UAVCAN_SUB_GPS_R` | 1 | Subscribe to relative GPS messages if supplied |
| `UAVCAN_SUB_MAG` | 1 | Subscribe to Here4 magnetic field if supplied |
| `UAVCAN_SUB_RNG` | 1 | Receive US-D1 through the range-sensor bridge |
| `SENS_EN_SF45_CFG` | 0 | Disable duplicate direct-UART SF45 input |
| `SF45C_ENABLE` | 1 | Start custom CAN receiver |
| `SF45C_NODE` | 70 | Match node-70 source ID (existing receiver default is 42) |
| `SF45C_SENSOR` | 0 | Match `PRX1_ADDR` |
| `SF45C_ORIENT` | 24 | Upright; use 25 for downward installation |
| `SF45C_YAW` | 0 | Forward; 2 right, 4 backward, 6 left |
| `SF45C_TIMEOUT` | 100 | Sample expiry, milliseconds |

Apply SF45 vehicle mounting corrections on PX4 only. Calibrate the Here4 compass and configure estimator sensor selection/offsets for the actual installation. This guide does not prescribe unverified estimator tuning. The present DroneCAN path does not implement the requested 5 Mbit/s CAN-FD uplink; both G474 interfaces use 1 Mbit/s for this version.

## Verify reception

With the vehicle disarmed, run:

```text
uavcan status
lightware_sf45_can status
listener sf45_sample
listener obstacle_distance
listener distance_sensor
listener sensor_gps
```

Confirm node 70 and Here4 appear under distinct IDs; SF45 sample counts increase with source 70 / sensor 0; the obstacle map follows the installed scan directions; US-D1 range and Here4 GPS update. Disconnect each sensor in turn and verify stale data handling, including SF45 expiry to unknown. Electrical presence alone does not verify the raw-sample firmware contract.

## Wire contract and limits

The modified AP_Periph sends raw SF45 distance/yaw samples using DroneCAN `Proximity` with `flags=1`. PX4 passes these to `sf45_sample` and `lightware_sf45_can`, which publishes a 72-bin, 5-degree `obstacle_distance` map with attitude projection and expiry. This paired extension is not a standard SF45 DroneCAN profile; keep the two modified firmware images paired. US-D1 uses the normal range bridge and Here4 uses normal GPS/compass bridges.

The AP backend requests SF45 update-rate code **5 (500 readings/s)**; scan limits remain stored in the sensor. Float16 transport quantizes distance/yaw, and PX4 timestamps reception rather than synchronized acquisition. Validate bus load and sample expiry with all attached devices operating.

PX4's [DroneCAN guide](https://docs.px4.io/v1.17/en/dronecan/index) describes the general bus and subscription setup; the local source defines this custom SF45 extension.

## Items to complete

- [ ] Apply and verify the documented receiver parameters and unique CAN identities on the actual flight controller.
- [ ] Complete SF45 installation settings, Here4 compass calibration if used, and sensor position/offset setup.
- [ ] Implement/configure the spray-command source and independent control of both valve hardpoint IDs; verify actuator ID mapping against AP_Periph.
- [ ] Implement node-80 pressure/flow/current reception and logging once its telemetry contract is defined.
- [ ] Define and test spray shutdown on disarm, command loss, and restart; the current node relay outputs do not time out.
- [ ] Run the reception and simultaneous-operation bench checks with the exact paired firmware images.
