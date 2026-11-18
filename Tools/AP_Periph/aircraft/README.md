# Aircraft CAN nodes

| Guide | Scope |
|---|---|
| [Node 70](node-70.md) | G474 avionics: build, defaults, wiring, update |
| [Node 70/71 alternative](node-70-71.md) | One G474 combining avionics and spray actuators; numeric CAN ID 70 |
| [Node 71](node-71.md) | G474 spray actuators: three PWM ESCs and two valve GPIOs |
| [Node 80](node-80.md) | L431 spray sensors: ADC/pulse hardware target; application telemetry pending |
| [CAN wiring](can-wiring.md) | Harness layout, termination, resistor installation, and bench tests |
| [PX4](px4.md) | Cube Orange+ v1.17.0-mod build, receiver settings, checks |

Reserve **70–79 for G474** and **80–89 for L431** nodes. These are valid DroneCAN IDs, not protocol-reserved ranges: inventory the actual bus before assigning them. Reserve PX4 ID 1; Here4 needs its own verified unique ID outside these ranges. Dynamic allocation does not enforce these reservations.

Share driver code and feature sets across roles where flash permits. Each MCU needs its own binary; G474 and L431 images are not interchangeable. Embedded node-specific defaults also require distinct images or a provisioning step. Board ID and DroneCAN node ID are separate identities.

**Transport constraint:** the current PX4 DroneCAN implementation uses classic CAN at 1 Mbit/s. The node-70 target enables existing bidirectional CAN mirroring at that rate. This is the agreed first implementation. The requested 5 Mbit/s uplink is not implemented: it requires validated PX4 CAN-FD support and a gateway design compatible with the Here4 segment. `CAN_FDBAUDRATE=5` alone is insufficient.

## Items to complete

- [ ] Choose the separate 70 + 71 arrangement or the combined 70/71 alternative and confirm the final physical layout.
- [ ] Complete the integration, calibration, and bench-test checklists in the selected node guides and PX4 guide.
- [ ] Complete node 80’s sensor firmware and PX4 telemetry integration before relying on spray pressure, flow, or current readings.
