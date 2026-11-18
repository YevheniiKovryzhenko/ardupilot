# CAN wiring and termination

## Wiring

Use a linear CAN trunk with CAN H connected to H and CAN L to L, a twisted H/L pair, and a common ground between devices. Keep branches (stubs) short; avoid star wiring. This guide covers conventional high-speed CAN wiring.

Power is a separate conductor in a CAN harness, not part of the H/L signaling pair. A CAN node or CAN GPS unit may accept power through its CAN connector or a separate input; check its specified voltage range and connector current rating. If an external BEC powers the harness, disconnect the Pixhawk power conductor from that BEC-fed rail unless the hardware explicitly supports combining those supplies; retain H, L, and common ground. Separately powered peripherals still need the specified signal reference/ground connection. Verify connector labels and pin numbering against the actual board/carrier before wiring.

## Daisy-chain, branches, and star layouts

DroneCAN devices are electrically connected **in parallel**: every transceiver connects to the same H wire and the same L wire on its segment. “Series” commonly describes the physical daisy-chain cable route; devices are not wired electrically in series. Never connect one device's H to another device's L.

### Daisy-chain (physical line)

Each line below represents an H/L twisted pair. `[T]` means one 120 Ω resistor across H/L at that endpoint.

```text
[T] Pixhawk ===== CAN peripheral ===== CAN peripheral ===== CAN GPS [T]
```

Run H straight through to H and L straight through to L at each device. Parallel pass-through connectors can make this convenient; they do not forward messages in software. Intermediate terminators must be disabled. The trunk length is the sum of all cable sections between the two terminated ends, not the length of one section.

### Trunk with parallel branches (stubs)

```text
[T] Pixhawk ==========+================+========== CAN GPS [T]
                     |                |
                 short stub       short stub
                     |                |
                 CAN sensor       CAN node
```

A tee or passive hub can attach devices to the trunk. A stub is the entire one-way path from the trunk junction to the device transceiver, including harness, connector/hub routing, and internal traces. Stub devices remain unterminated. A passive hub does not reset cable-length limits or create independent segments.

### Passive star

```text
                        CAN sensor
                            |
Pixhawk ================= passive hub ================= CAN GPS
                            |
                         CAN node
```

A star with several long arms creates several reflecting cable ends; adding a terminator to every arm overloads the bus. It has no universal safe arm-length limit. Treat two arms as the main trunk and every other arm as a stub; if those branches cannot meet the stub guidance below, reroute as a line or use an appropriate active solution. A passive CAN splitter is not an active repeater or gateway.

## Cable-length limits

There is **no single exact wire-length limit for every DroneCAN installation**. Propagation delay, transceiver timing, controller bit timing, cable characteristics, node loading, and edge reflections all matter. Bitrate is included here because it affects the available timing margin.

| Layout / measurement | Conventional classic CAN guidance at 1 Mbit/s | How to apply it |
|---|---|---|
| Point-to-point or daisy-chain trunk | Up to 40 m under the cited reference conditions | Sum all cable sections along the trunk between terminators; this is not a per-device allowance |
| Each branch / stub | At most 0.30 m (30 cm); shorter is preferable | Measure junction to transceiver, including internal routing; 30 cm of external cable leaves no allowance for internal traces |
| Passive star | No separate guaranteed arm length | Choose one trunk between two ends; all other arms must qualify as short stubs |
| Mixed daisy-chain and short branches | Same trunk and individual-stub guidance | Branching does not increase the permitted trunk length |
| Independent segments joined by a software gateway | Apply limits independently to each segment | Each segment has its own transceivers and two endpoint terminators |

The **40 m trunk / 0.30 m stub** figures are conventional reference limits documented by [Analog Devices](https://www.analog.com/en/resources/app-notes/an-1123.html) and [TI](https://www.ti.com/lit/an/slla486c/slla486c.pdf), not a guarantee that every harness meeting those dimensions will work. Many simultaneous stubs add loading and reflections; meeting the individual-stub limit alone is insufficient. Keep aircraft harnesses substantially shorter where practical and validate the assembled network. Lower bitrate may permit a longer trunk, but does not automatically justify long stubs because signal edge speed still matters. CAN-FD needs a separate timing/signal-integrity assessment; do not reuse these figures as CAN-FD limits.

## Combining layouts

**Daisy-chain sections and short tee branches can be mixed on one segment**, provided there is one identifiable trunk, two terminators at its ends, and short unterminated stubs. A compact passive hub can be part of that layout when its wiring obeys the same rules. Do not mix in long star arms or close the trunk into a ring.

A dual-port node with independent transceivers and software forwarding can connect two separately terminated segments, each with its own line-and-stub layout. This differs from two parallel connectors on one transceiver, which remain one segment. An active repeater's topology and length limits depend on its manufacturer; do not assume it behaves like a software gateway or grants another full cable-length allowance.

## Termination

Fit **one 120 Ω resistor across CAN H and CAN L at each physical end of each segment**. Count built-in or jumper-enabled terminators: the goal is two per segment, not one per device. Intermediate nodes do not need termination. Do not assume a Pixhawk carrier, CAN GPS unit, or CAN node already has a resistor fitted or enabled; check its documentation or measure it with power disconnected.

A dual-port CAN node can forward frames in software between two separate transceivers. These ports remain separate electrical segments: termination on the GPS side does not terminate the Pixhawk side. For example:

| Example segment | First end: 120 Ω across H/L | Second end: 120 Ω across H/L |
|---|---|---|
| Pixhawk ↔ dual-port CAN node | Pixhawk end | Node's Pixhawk-facing port |
| Dual-port CAN node ↔ CAN GPS unit | Node's GPS-facing port | CAN GPS end |

By contrast, a Pixhawk, several CAN peripherals, and a CAN GPS unit wired along one continuous H/L trunk form a single segment. Terminate only the two physical ends of that trunk; leave intermediate devices unterminated. Two connectors on a device may simply be parallel connections to the same transceiver rather than independent ports—check the hardware documentation.

Use an existing terminator at an endpoint if present; otherwise add one there. Do not create a second physical path between segments already linked by a software forwarding bridge, as that can create a forwarding loop.

## Using loose resistors

A **120 Ω, 1%, ¼ W resistor** is suitable for normal CAN termination. Resistors have no polarity. At each endpoint missing termination:

1. Disconnect every power source, including USB and sensor supplies.
2. Connect one resistor lead to **CAN H** and the other to **CAN L**, in parallel across the pair. Keep the normal H-to-H and L-to-L signal wiring intact; do not put the resistor in series with either wire.
3. Place the resistor near the endpoint connector, using an inline harness splice, a properly made terminator plug, or designated termination pads if provided. Keep leads short.
4. Solder and insulate the joints individually; provide strain relief so vibration cannot flex bare resistor leads or pull on a connector. Do not push loose leads into connector contacts.
5. Check the completed segment with the resistance test below before applying power.

```text
CAN H ──────┬──── endpoint H
            │
          [120 Ω]
            │
CAN L ──────┴──── endpoint L
```

The resistor connects **only H to L**, never to 5 V or ground. If 120 Ω parts are unavailable, two 240 Ω resistors in parallel form one 120 Ω terminator; make and test that assembly before installation.

## Power-off multimeter test

Disconnect **all** power sources and let the rails discharge. Never use resistance mode on a powered bus. Keep the segment's normal wiring and both endpoint terminators connected, then measure resistance between H and L. Test each electrically separate segment independently.

| H-to-L resistance | Typical interpretation |
|---|---|
| Approximately 60 Ω | Two 120 Ω terminators in parallel: expected |
| Approximately 120 Ω | One terminator present: find the missing endpoint termination |
| Approximately 40 Ω | Three 120 Ω terminators: remove the extra one |
| Very high / open | Missing termination or an open cable; check continuity |
| Near zero | H/L short or incorrect resistor/wiring; fix before powering |

These readings assume standard passive termination; connected electronics can affect a measurement. If unexpected, disconnect and check each cable/device individually. An isolated endpoint with its terminator should measure approximately 120 Ω. Also check H-to-H, L-to-L, and ground continuity through each cable, and check for unintended shorts to power or ground. A 60 Ω reading alone does not prove correct polarity, cable continuity, or signal quality.

## Powered bench test

After resistance and wiring checks, power the system with the vehicle disarmed and actuators safe. Confirm each device's supply remains within its manufacturer-specified range under load. Use the flight controller's CAN status tools or a CAN analyzer to verify that all expected devices appear and, for DroneCAN, have unique node IDs. On a Pixhawk running PX4 DroneCAN, `uavcan status` is one example of a status command.

Verify all attached peripherals publish fresh data simultaneously. For a forwarding node, confirm that devices on the far segment are visible from the flight-controller side and that required return traffic also passes. Watch for increasing CAN error counters, device disappearances, stale samples, or missed deadlines during sustained operation. A multimeter cannot verify waveform quality; investigate persistent errors with a CAN analyzer or oscilloscope.

## References

- [PX4 CAN wiring guidance](https://docs.px4.io/main/en/can/index)
- [Analog Devices AN-1123 CAN implementation guide](https://www.analog.com/en/resources/app-notes/an-1123.html)
- [TI CAN bus design and termination guidance](https://www.ti.com/lit/an/slla486c/slla486c.pdf)

## Items to complete

- [ ] Inventory built-in/enabled terminators and verify the final harness topology, connector pinouts, grounds, and supply routing.
- [ ] Perform the documented power-off resistance/continuity checks on each segment.
- [ ] Run the powered simultaneous-device test and investigate any increasing CAN errors, stale samples, or device disappearances.
