# ANJ-3600W-LVP-WIFI — Modbus RTU register research

**OEM hardware:** SPI4836M80-300P (ANENJI 3600 W, 48 V, 120 V inverter).  
**Status:** Work in progress; values below are from passive captures, **not** an official manufacturer register specification.

## Connection and protocol

- Observed RTU traffic: **9600 baud, 8 data bits, no parity, 1 stop bit (8N1)**.
- Observed slave address byte: **0xFF** (255). This is what the factory Wi-Fi module transmits; whether it is a conventional configurable device ID is unverified.
- Read function: **0x03** (holding registers). **Write function 0x06** (Write Single Register) is now captured and acknowledged for **Eco Enable at 41005**. Other registers' writeability is unverified.
- The factory SmartValue/Eybond Wi-Fi module is the active polling device. Captures were made using an ESP32-S3 in receive-only mode.
- On the tested RJ45 RS485-1 connector, the user verified **pin 1=A, pin 2=B, pin 7=+5 V, pin 8=GND**. Other hardware revisions may differ; verify before wiring.
- Do **not** drive the bus or enable an additional termination resistor without checking the physical network. An automatic-direction RS485 adapter is not necessarily passive just because TXD is disconnected.

## Identified registers

Addresses below are **raw decimal addresses transmitted on the wire**, not 4xxxx documentation notation or zero/one-based library offsets.

| Address (decimal) | Hex | Name | Observed raw value | Interpretation | Confidence |
|---|---|---|---|---|---|
| 40000 | 0x9C40 | `on_off` | 1 | Current ON/OFF control readback is 1; OFF encoding and write behavior not tested | Confirmed read and field association |
| 41000 | 0xA028 | `OutPriority` | 3 | **Hybrid** | Confirmed by SmartValue setting |
| 41001 | 0xA029 | `Output Volt Set` | 1200 (`0x04B0`) | Likely **120.0 V** (÷10); scale inferred | Confirmed read and field association; scale unverified |
| 41002 | 0xA02A | `Output Frequency Set` | 6000 (`0x1770`) | Likely **60.00 Hz** (÷100); scale inferred | Confirmed read and field association; scale unverified |
| 41003 | 0xA02B | `Line Range Set` | 0 (`0x0000`) | **UPS**; other app options APL and GEN, encodings unknown | Confirmed by SmartValue setting |
| 41004 | 0xA02C | `Para Mode Set` | 2 (`0x0002`) | **2P0**; available modes: Single, Parallel, 2P0, 2P1, 2P2, 3P1, 3P2, 3P3; all other encodings unverified | Confirmed by SmartValue setting |
| 41005 | 0xA02D | `Eco Enable` | Read: 1 (`0x0001`); write: 0 (`0x0000`) | **1 = Enabled; 0 = Disabled** | Read confirmed; function-06 disable write acknowledged; persistence not yet checked |
| 41006 | 0xA02E | `Charge Priority` | 2 (`0x0002`) | **SNU**; app options OSO, OUO, SNU, NUC; other encodings unverified (likely 0,1,3 by display order) | Confirmed read and field association |
| 41007 | 0xA02F | `Max Charge Current Set (A)` | 800 (`0x0320`) | **80 A** (raw ÷10) | Read and scale corroborated by SmartValue setting; write unverified |
| 30024 | 0x7548 | AC input voltage candidate | 1213 | Possibly 121.3 V (÷10), consistent with app | Hypothesis |
| 30026 | 0x754A | AC input frequency candidate | 5997 | Possibly 59.97 Hz (÷100) | Hypothesis |
| 30058 | 0x756A | AC output voltage candidate | 1214 | Possibly 121.4 V (÷10) | Hypothesis |
| 30060 | 0x756C | AC output frequency candidate | 5997 | Possibly 59.97 Hz (÷100) | Hypothesis |

**Output priority options in SmartValue:** PV, Grid, Battery, Hybrid. Only **3 = Hybrid** is verified. The proposed 0=PV, 1=Grid, 2=Battery sequence is **unverified** and must not be treated as a write-ready enum.

### Raw read evidence

```text
# on_off, one register starting at 40000
TX  FF 03 9C 40 00 01 BE 50
RX  FF 03 02 00 01 50 50

# OutPriority, one register starting at 41000
TX  FF 03 A0 28 00 01 33 DC
RX  FF 03 02 00 03 D1 91

# Output Volt Set, one register starting at 41001
TX  FF 03 A0 29 00 01 62 1C
RX  FF 03 02 04 B0 92 E4

# Output Frequency Set, one register starting at 41002
TX  FF 03 A0 2A 00 01 92 1C
RX  FF 03 02 17 70 9F 84

# Line Range Set, one register starting at 41003; SmartValue currently UPS
TX  FF 03 A0 2B 00 01 C3 DC
RX  FF 03 02 00 00 91 90

# Para Mode Set, one register starting at 41004; SmartValue currently 2P0
TX  FF 03 A0 2C 00 01 72 1D
RX  FF 03 02 00 02 10 51

# Eco Enable, one register starting at 41005; SmartValue currently enabled
TX  FF 03 A0 2D 00 01 23 DD
RX  FF 03 02 00 01 50 50

# Eco Enable, disable using Write Single Register (function 0x06)
TX  FF 06 A0 2D 00 00 2E 1D
RX  FF 06 A0 2D 00 00 2E 1D

# Charge Priority, one register starting at 41006; SmartValue currently SNU
TX  FF 03 A0 2E 00 01 D3 DD
RX  FF 03 02 00 02 10 51

# Max Charge Current Set (A), one register starting at 41007; SmartValue currently 80 A
TX  FF 03 A0 2F 00 01 82 1D
RX  FF 03 02 03 20 90 B8

# candidate grid-input measurements: 30024..30030
TX  FF 03 75 48 00 07 8B CC
RX  FF 03 0E 04 BD 00 00 17 6D 00 0D 00 00 00 00 00 30 FB F4

# candidate AC-output measurements: 30058..30065
TX  FF 03 75 6A 00 08 6B C2
RX  FF 03 10 04 BE FF F3 17 6D 00 00 00 00 00 00 00 00 FF 63 8B 9B
```

## Other observed register blocks

The factory Wi-Fi module issued function-03 reads from the following start addresses. These are **observed block starts**, not independently decoded register names:

| Decimal start | Hex start | Quantity |
|---:|---|---:|
| 30000 | 0x7530 | 6 |
| 30011 | 0x753B | 6 |
| 30024 | 0x7548 | 7 |
| 30032 | 0x7550 | 2 |
| 30035 | 0x7553 | 3 |
| 30039 | 0x7557 | 2 |
| 30042 | 0x755A | 3 |
| 30046 | 0x755E | 4 |
| 30050 | 0x7562 | 2 |
| 30058 | 0x756A | 8 |
| 30061 | 0x756D | 4 |
| 30068 | 0x7574 | 3 |
| 30071 | 0x7577 | 5 |
| 30084 | 0x7584 | 3 |
| 30101 | 0x7595 | 3 |
| 30109 | 0x759D | 6 |
| 30117 | 0x75A5 | 4 |
| 30121 | 0x75A9 | 2 |
| 40000 | 0x9C40 | 1 |
| 41000 | 0xA028 | 15 (also independently read as 1) |
| 41018 | 0xA03A | 9 |
| 41027 | 0xA043 | 1 |
| 41031 | 0xA047 | 1 |
| 41065 | 0xA069 | 1 |
| 41100 | 0xA08C | 13 |
| 41113 | 0xA099 | 1 |
| 41115 | 0xA09B | 10 |
| 41127 | 0xA0A7 | 9 |
| 51000 | 0xC738 | 2 |
| 51014 | 0xC746 | 25 |
| 51044 | 0xC764 | 2 |
| 52044 | 0xCB4C | 16 |
| 52071 | 0xCB67 | 2 |

### Battery/configuration-like block 41100–41112

Captured read: `FF 03 A0 8C 00 0D 72 3A`

Response data (13 successive 16-bit words):

| Address | Raw hex | Decimal | ÷10 |
|---:|---|---:|---:|
| 41100 | 0006 | 6 | 0.6 |
| 41101 | 0240 | 576 | 57.6 |
| 41102 | 0240 | 576 | 57.6 |
| 41103 | 0258 | 600 | 60.0 |
| 41104 | 0078 | 120 | 12.0 |
| 41105 | 0218 | 536 | 53.6 |
| 41106 | 01E8 | 488 | 48.8 |
| 41107 | 0240 | 576 | 57.6 |
| 41108 | 01EC | 492 | 49.2 |
| 41109 | 01C4 | 452 | 45.2 |
| 41110 | 01C0 | 448 | 44.8 |
| 41111 | 001E | 30 | 3.0 |
| 41112 | 0200 | 512 | 51.2 |

Many values resemble battery-voltage settings, but **their names, scales, and writeability remain unverified**. In particular, **41100 = 6**, not 57.6.

## Safe next steps

1. Capture readbacks while changing **one harmless setting at a time** in SmartValue; record old and new values and corresponding LCD setting.
2. Function `0x06` has been captured for Eco Enable. Capture a **follow-up read** to verify the disabled setting persisted, and identify write functions/encodings for other settings individually.
3. Do not test power-off or output-priority writes on a loaded/parallel inverter unless an interruption is acceptable.
4. Confirm whether parallel inverter units have independent IDs or the factory interface addresses a shared controller.

## Sources and notes

- Hardware/manual lead: [ANENJI SPI4836M80-300P manual](https://device.report/m/f7e4669f1e6bb81efc547895b8013c2f3d8de2ea167f6d0cc6650457605c2a41).
- Register evidence: user-supplied ESPHome UART captures from November 22, 2026.
- No complete, verified public register map has yet been identified for this exact OEM protocol.
