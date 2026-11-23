# ANJ-3600W-LVP-WIFI — Modbus RTU register research

**OEM hardware:** SPI4836M80-300P (ANENJI 3600 W, 48 V, 120 V inverter).  
**Status:** Work in progress; values below are from passive captures, **not** an official manufacturer register specification.

## Connection and protocol

- Observed RTU traffic: **9600 baud, 8 data bits, no parity, 1 stop bit (8N1)**.
- Observed slave address byte: **0xFF** (255). This is what the factory Wi-Fi module transmits; whether it is a conventional configurable device ID is unverified.
- Read function: **0x03** (holding registers). **Write function 0x06** (Write Single Register) is captured and acknowledged for **Eco Enable at 41005**. **Write function 0x10** (Write Multiple Registers) is captured and acknowledged for **Local Time Set at 41018–41020**. Other registers' writeability is unverified.
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
| 41008 | 0xA030 | `Charge Current By PV Set` | 800 (`0x0320`) | **80 A** (raw ÷10) | Read and scale corroborated by SmartValue setting; write unverified |
| 41009 | 0xA031 | `Charge Current By Line Set (A)` | 0 (`0x0000`) | **0 A**; scaling unverified because zero cannot distinguish scales (likely ÷10) | Read and field association confirmed; write unverified |
| 41011 | 0xA033 | `Mute Enable` | 1 (`0x0001`) | **1 = Enabled** (confirmed by SmartValue); 0 = Disabled unverified | Read and enabled-state mapping confirmed; write unverified |
| 41012 | 0xA034 | `On Grid Set` | 2 (`0x0002`) | **Anti-backflow (with CT added)**; options: Disable, On-Grid, Anti-backflow (CT), Anti-backflow (smart meter); other enum encodings unverified | Read and CT mode confirmed; write unverified |
| 41013 | 0xA035 | `Charge Full Current Set` | 30 (`0x001E`) | **3 A** (raw ÷10); SmartValue options 2–15 A in 1 A steps; other values calculated, not observed | Read and scale corroborated by SmartValue; write unverified |
| 41014 | 0xA036 | `NG Function Enabled` | 0 (`0x0000`) | **Disabled**; 1 = Enabled is plausible but unverified; function semantics unknown | Read and disabled-state mapping confirmed; write unverified |
| 41018–41020 | 0xA03A–0xA03C | `Local Time Set` (year/month/day/hour/minute/second) | Read: `0x180B`, `0x1E0B`, `0x0726`; write: `0x1A0B`, `0x1614`, `0x1F26` | Six packed bytes in Y/M/D/h/m/s order, year 2000+byte. Read **2024-11-30 11:07:38**; write **2026-11-22 20:31:38** | Read decoding confirmed against SmartValue; function-`0x10` write acknowledged; post-write readback pending |
| 41022 | 0xA03E | `Battery Active Enable` | 1 (`0x0001`) | Likely **Enabled**; UI state and 0=Disabled mapping not yet confirmed; precise function unknown | Read confirmed; write unverified |
| 41023 | 0xA03F | `On Grid Active Power Set (W)` | 3600 (`0x0E10`) | **3600 W**, raw value in watts (1:1), confirmed against SmartValue; meaning as grid power limit/export target not yet verified | Read confirmed; write unverified; grid-sensitive control |
| 41025 | 0xA041 | `Hybrid Priority Enable` | 1 (`0x0001`) | **Enabled**, confirmed against SmartValue display; 0=Disabled not yet verified; separate from `OutPriority` at 41000 | Read and enabled-state mapping confirmed; write and exact behavior unverified |
| 41027 | 0xA043 | `On Grid Standard Set` | 0 (`0x0000`) | **N/A**, confirmed against SmartValue; enum 0=N/A confirmed, 1=VDE4105, 2=IEEE1547, 3=CEI0-21 inferred from menu order | Read confirmed; write unverified; safety-sensitive grid-interconnection profile |
| 41031 | 0xA047 | `External CT Host Enable` | 1 (`0x0001`) | **Enabled**, confirmed against SmartValue; 1=Enabled verified, 0=Disabled inferred; CT host role and parallel-inverter behavior unverified | Read confirmed; write unverified; anti-backflow/CT configuration |
| 41032 | 0xA048 | `Output phase setting` (SmartValue Default tab, 2/9; phase-angle selection) | 0 (`0x0000`) | **Phase difference 0°**, confirmed; enum 0=0° verified, 1=positive 120°, 2=positive 180°, 3=reverse 120° inferred from menu order; distinct from enable flag 41091 | Read confirmed; write unverified; safety-sensitive phase configuration |
| 41065 | 0xA069 | `Island Enable` | 1 (`0x0001`) | **Enabled**, confirmed against SmartValue; 1=Enabled verified, 0=Disabled inferred; exact islanding/off-grid function unverified | Read confirmed; write unverified; safety-sensitive grid/islanding setting |
| 41078 | 0xA076 | `Anti-reserve Compensate (W)` (SmartValue Default tab, 3/9) | 150 (`0x0096`) | **150 W**, raw units watts (1:1), confirmed against SmartValue; anti-backflow compensation/offset semantics unverified | Read confirmed; write unverified; grid/CT control setting |
| 41079 | 0xA077 | `Second Output On Time (min)` (SmartValue Default tab, 4/9) | 0 (`0x0000`) | **0 min** confirmed against SmartValue; 1 raw unit = 1 minute likely, not independently established from zero; zero semantics and secondary-output behavior unknown | Read confirmed; write unverified |
| 41091 | 0xA083 | `Output Phase Setting` (SmartValue Default tab, 1/9) | 0 (`0x0000`) | **Disabled**, confirmed against SmartValue; 0=Disabled verified, 1=Enabled inferred; precise phase behavior unknown | Read confirmed; write unverified; phase/parallel configuration safety-sensitive |
| 41100 | 0xA08C | `Battery Type` | 6 (`0x0006`) | **L16** confirmed; proposed zero-based menu enum: 0 User Defined, 1 Sealed Lead Acid, 2 Flooded Lead Acid, 3 Gel Cell, 4 L14, 5 L15, **6 L16**, 7 L7, 8 L8, 9 L9, 10 N7, 11 N8, 12 N13, 13 N14. **All except 6 inferred, not tested.** | Read confirmed; write unverified; battery charging safety-sensitive |
| 41101 | 0xA08D | `Cv Volt` | 576 (`0x0240`) | **57.6 V**, raw ÷10, confirmed against SmartValue; constant-voltage charging setpoint | Read confirmed; write unverified; charging safety-sensitive |
| 41102 | 0xA08E | `Float Volt` | 576 (`0x0240`) | **57.6 V**, raw ÷10, confirmed against SmartValue; float charging voltage setpoint | Read confirmed; write unverified; charging safety-sensitive |
| 41104 | 0xA090 | `Cv Charge Time Set (min)` | 120 (`0x0078`) | **120 minutes (2 hours)**, raw units are minutes, confirmed against SmartValue; exact timer behavior unverified | Read confirmed; write unverified; charging safety-sensitive |
| 41105 | 0xA091 | `Cv Charge Back Volt (volts)` | 536 (`0x0218`) | **53.6 V**, raw ÷10, confirmed against SmartValue; likely CV charge re-entry threshold, behavior unverified | Read confirmed; write unverified; charging safety-sensitive |
| 41106 | 0xA092 | `Inverter 2 Line Volt (v)` | 488 (`0x01E8`) | **48.8 V**, raw ÷10, confirmed against SmartValue; likely inverter-to-line battery voltage transfer threshold, exact behavior unverified | Read confirmed; write unverified; battery/transfer safety-sensitive |
| 41107 | 0xA093 | `Line Back 2 Inverter Volt (v)` | 576 (`0x0240`) | **57.6 V**, raw ÷10, confirmed against SmartValue; likely line-to-inverter battery recovery transfer threshold, exact behavior unverified | Read confirmed; write unverified; battery/transfer safety-sensitive |
| 41108 | 0xA094 | `Battery Low Voltage (v)` | 492 (`0x01EC`) | **49.2 V**, raw ÷10, confirmed against SmartValue; low-battery threshold, exact alarm/transfer/shutdown behavior unverified | Read confirmed; write unverified; battery protection safety-sensitive |
| 41109 | 0xA095 | `Battery Delay Off Voltage (v)` | 452 (`0x01C4`) | **45.2 V**, raw ÷10, confirmed against SmartValue; likely delayed low-voltage shutdown threshold, delay behavior unverified | Read confirmed; write unverified; battery protection safety-sensitive |
| 41112 | 0xA098 | `Battery End Of Discharge Volts (V)` | 512 (`0x0200`) | **51.2 V**, raw ÷10, confirmed against SmartValue; discharge termination threshold semantics relative to other low-voltage settings unverified | Read confirmed; write unverified; battery protection safety-sensitive |
| 41113 | 0xA099 | `BMS-Setup` | 1 (`0x0001`) | **Enable 485-BMS**, confirmed against SmartValue; enum 0=Disable BMS (inferred), 1=Enable 485-BMS (confirmed), 2=Enable CAN-BMS (inferred) | Read confirmed; write unverified; BMS interface selection; protocol compatibility unverified |
| 41115 | 0xA09B | `Battery State Of Charge Alarm (%)` | 15 (`0x000F`) | **15%**, raw units are percent (1:1), confirmed against SmartValue; alarm behavior and applicability by battery type unverified | Read confirmed; write unverified; battery protection setting |
| 41116 | 0xA09C | `Battery Stop State Of Charge (%)` | 5 (`0x0005`) | **5%**, raw units are percent (1:1), confirmed against SmartValue; precise stop behavior and applicability by battery type unverified | Read confirmed; write unverified; battery protection safety-sensitive |
| 41117 | 0xA09D | `Battery Full State Of Charge (%)` | 100 (`0x0064`) | **100%**, raw units are percent (1:1), confirmed against SmartValue; full-SOC threshold behavior unverified | Read confirmed; write unverified; battery charging configuration |
| 41118 | 0xA09E | `Inverter 2 Line State of charge (%)` | 10 (`0x000A`) | **10%**, raw units are percent (1:1), confirmed against SmartValue; likely inverter-to-line SOC transfer threshold, exact behavior unverified | Read confirmed; write unverified; battery/transfer setting |
| 41119 | 0xA09F | `Line Back 2 inverter State of Charge (%)` | 20 (`0x0014`) | **20%**, raw units are percent (1:1), confirmed against SmartValue; likely line-to-inverter SOC transfer threshold, exact behavior unverified | Read confirmed; write unverified; battery/transfer setting |
| 41120 | 0xA0A0 | `On Time Line Charge Enable` | 0 (`0x0000`) | Likely **Disabled**; 1=Enabled inferred; SmartValue state and scheduled utility-charging semantics not yet verified | Read confirmed; write unverified |
| 41121 | 0xA0A1 | `Line Charge Start Time1(HH-mm)` | 4643 (`0x1223`) | **18:35**, confirmed against SmartValue; packed high byte=hour (18), low byte=minute (35), 24-hour clock | Read confirmed; write unverified; scheduled grid charging |
| 41122 | 0xA0A2 | `Line Charge End Time 1(HH-mm)` | 0 (`0x0000`) | **00:00**, confirmed against SmartValue; packed high byte=hour (0), low byte=minute (0), consistent with register 41121 encoding; midnight handling unverified | Read confirmed; write unverified; scheduled grid charging |
| 41123 | 0xA0A3 | `Line Charge Start Time2(HH-mm)` | 0 (`0x0000`) | **00:00**, confirmed against SmartValue; packed high byte=hour (0), low byte=minute (0), consistent with register 41121 encoding | Read confirmed; write unverified; scheduled grid charging |
| 41124 | 0xA0A4 | `Line Charge End Time 2(HH-mm)` | 0 (`0x0000`) | **00:00**, confirmed against SmartValue; packed high byte=hour (0), low byte=minute (0), consistent with register 41121 encoding; equal start/end behavior unverified | Read confirmed; write unverified; scheduled grid charging |
| 41127 | 0xA0A7 | `On time battery discharge enable` | 1 (`0x0001`) | **Enabled**, confirmed against SmartValue; 1=Enabled verified, 0=Disabled inferred; scheduled battery-discharge behavior unverified | Read confirmed; write unverified |
| 41128 | 0xA0A8 | `Battery Discharge Start Time 1(HH-mm)` | 5410 (`0x1522`) | **21:34**, confirmed against SmartValue; packed high byte=hour (21), low byte=minute (34), 24-hour clock | Read confirmed; write unverified; scheduled battery discharge |
| 41129 | 0xA0A9 | `Battery Discharge End Time 1(HH-mm)` | 5179 (`0x143B`) | **20:59** decoded; packed high byte=hour (20), low byte=minute (59); SmartValue display not separately confirmed; earlier than 21:34 start, possible overnight window unverified | Read confirmed; write unverified; scheduled battery discharge |
| 41130 | 0xA0AA | `Battery Discharge Start Time 2(HH-mm)` | 0 (`0x0000`) | **00:00** decoded; packed high byte=hour (0), low byte=minute (0), consistent with confirmed HH:mm encoding; SmartValue display not separately confirmed | Read confirmed; write unverified; scheduled battery discharge |
| 41131 | 0xA0AB | `Battery Discharge End Time 2(HH-mm)` | 0 (`0x0000`) | **00:00**, confirmed against SmartValue; packed high byte=hour (0), low byte=minute (0); equal start/end behavior unverified | Read confirmed; write unverified; scheduled battery discharge |
| 41135 | 0xA0AF | `Battery Connection Way` | 0 (`0x0000`) | **Battery Parallel Input**, confirmed against SmartValue; enum `0`=Battery Parallel Input confirmed, `1`=Battery Independent Input inferred from two available choices; physical behavior unverified | Read confirmed; write unverified; safety-sensitive battery topology configuration |
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

# Charge Current By PV Set, one register starting at 41008; SmartValue currently 80 A
TX  FF 03 A0 30 00 01 B3 DB
RX  FF 03 02 03 20 90 B8

# Charge Current By Line Set (A), one register starting at 41009; SmartValue currently 0 A
TX  FF 03 A0 31 00 01 E2 1B
RX  FF 03 02 00 00 91 90

# Mute Enable, one register starting at 41011; SmartValue shows enabled, returned value 1
TX  FF 03 A0 33 00 01 43 DB
RX  FF 03 02 00 01 50 50

# On Grid Set, one register starting at 41012; SmartValue currently Anti-backflow (with CT added)
TX  FF 03 A0 34 00 01 F2 1A
RX  FF 03 02 00 02 10 51

# Charge Full Current Set, one register starting at 41013; SmartValue currently 3 A
TX  FF 03 A0 35 00 01 A3 DA
RX  FF 03 02 00 1E 11 98

# NG Function Enabled, one register starting at 41014; SmartValue currently disabled
TX  FF 03 A0 36 00 01 53 DA
RX  FF 03 02 00 00 91 90

# Local Time Set, 3 registers starting at 41018; six bytes in Y/M/D/h/m/s order (inferred)
TX  FF 03 A0 3A 00 03 12 18
RX  FF 03 06 18 0B 1E 0B 07 26 3B 08

# Local Time Set, Write Multiple Registers (function 0x10), 3 registers at 41018
# Packed Y/M/D/h/m/s = 26/11/22/20/31/38 => 2026-11-22 20:31:38
TX  FF 10 A0 3A 00 03 06 1A 0B 16 14 1F 26 F8 7F
RX  FF 10 A0 3A 00 03 97 DB

# Battery Active Enable, one register at 41022; raw value 1, likely enabled
TX  FF 03 A0 3E 00 01 D2 18
RX  FF 03 02 00 01 50 50

# On Grid Active Power Set (W), one register at 41023; SmartValue currently 3600 W
TX  FF 03 A0 3F 00 01 83 D8
RX  FF 03 02 0E 10 94 3C

# Hybrid Priority Enable, one register at 41025; raw value 1 = Enabled confirmed against SmartValue
TX  FF 03 A0 41 00 01 E3 C0
RX  FF 03 02 00 01 50 50

# On Grid Standard Set, one register at 41027; SmartValue currently N/A
TX  FF 03 A0 43 00 01 42 00
RX  FF 03 02 00 00 91 90
# Enum 0=N/A (confirmed); 1=VDE4105, 2=IEEE1547, 3=CEI0-21 (inferred from UI order)
# Grid-compliance setting: no write or behavior verification

# External CT Host Enable, one register at 41031; SmartValue currently Enabled
TX  FF 03 A0 47 00 01 03 C1
RX  FF 03 02 00 01 50 50
# 1=Enabled confirmed; 0=Disabled inferred; CT host role and parallel-inverter behavior unverified

# SmartValue Default tab (setting 2 of 9): Output phase setting (phase angle), register 41032; currently 0 degrees
TX  FF 03 A0 48 00 01 33 C2
RX  FF 03 02 00 00 91 90
# Enum: 0=0 degrees (confirmed); 1=positive 120, 2=positive 180, 3=reverse 120 (inferred)
# Separate from Output Phase Setting enable flag at 41091; writeability unverified

# Island Enable, one register at 41065; SmartValue currently Enabled
TX  FF 03 A0 69 00 01 63 C8
RX  FF 03 02 00 01 50 50
# 1=Enabled confirmed; 0=Disabled inferred; islanding function and safety behavior unverified

# SmartValue Default tab (setting 3 of 9): Anti-reserve Compensate (W), register 41078; currently 150 W
TX  FF 03 A0 76 00 01 52 0E
RX  FF 03 02 00 96 11 FE
# 0x0096 = 150 W, no scaling; exact anti-backflow compensation behavior unverified

# SmartValue Default tab (setting 4 of 9): Second Output On Time (min), register 41079; currently 0 min
TX  FF 03 A0 77 00 01 03 CE
RX  FF 03 02 00 00 91 90
# Raw zero matches SmartValue 0 min; nonzero scaling, zero semantics and writeability unverified

# SmartValue Default tab (setting 1 of 9): Output Phase Setting, register 41091; currently Disabled
TX  FF 03 A0 83 00 01 42 3C
RX  FF 03 02 00 00 91 90
# 0=Disabled confirmed; 1=Enabled inferred; phase behavior and writeability unverified

# Battery Type, one register at 41100; SmartValue currently L16
TX  FF 03 A0 8C 00 01 72 3F
RX  FF 03 02 00 06 11 92

# Cv Volt, one register at 41101; SmartValue currently 57.6 V
TX  FF 03 A0 8D 00 01 23 FF
RX  FF 03 02 02 40 91 00
# 0x0240 = 576; divide by 10 for 57.6 V

# Float Volt, one register at 41102; SmartValue currently 57.6 V
TX  FF 03 A0 8E 00 01 D3 FF
RX  FF 03 02 02 40 91 00
# 0x0240 = 576; divide by 10 for 57.6 V

# Cv Charge Time Set (min), one register at 41104; SmartValue currently 120 minutes
TX  FF 03 A0 90 00 01 B3 F9
RX  FF 03 02 00 78 91 B2
# 0x0078 = 120 minutes, no scaling

# Cv Charge Back Volt (volts), one register at 41105; SmartValue currently 53.6 V
# Sniffer logged contiguous request and response in one UART entry
TX  FF 03 A0 91 00 01 E2 39
RX  FF 03 02 02 18 90 FA
# 0x0218 = 536; divide by 10 for 53.6 V

# Inverter 2 Line Volt (v), one register at 41106; SmartValue currently 48.8 V
TX  FF 03 A0 92 00 01 12 39
RX  FF 03 02 01 E8 90 4E
# 0x01E8 = 488; divide by 10 for 48.8 V

# Line Back 2 Inverter Volt (v), one register at 41107; SmartValue currently 57.6 V
TX  FF 03 A0 93 00 01 43 F9
RX  FF 03 02 02 40 91 00
# 0x0240 = 576; divide by 10 for 57.6 V

# Battery Low Voltage (v), one register at 41108; SmartValue currently 49.2 V
TX  FF 03 A0 94 00 01 F2 38
RX  FF 03 02 01 EC 91 8D
# 0x01EC = 492; divide by 10 for 49.2 V

# Battery Delay Off Voltage (v), one register at 41109; SmartValue currently 45.2 V
TX  FF 03 A0 95 00 01 A3 F8
RX  FF 03 02 01 C4 91 93
# 0x01C4 = 452; divide by 10 for 45.2 V

# Battery End Of Discharge Volts (V), one register at 41112; SmartValue currently 51.2 V
TX  FF 03 A0 98 00 01 32 3B
RX  FF 03 02 02 00 90 F0
# 0x0200 = 512; divide by 10 for 51.2 V

# BMS-Setup, one register at 41113; SmartValue currently Enable 485-BMS
TX  FF 03 A0 99 00 01 63 FB
RX  FF 03 02 00 01 50 50
# Enum 0=Disable BMS (inferred), 1=Enable 485-BMS (confirmed), 2=Enable CAN-BMS (inferred)
# Interface selection does not establish actual BMS protocol compatibility

# Battery State Of Charge Alarm (%), one register at 41115; SmartValue currently 15%
TX  FF 03 A0 9B 00 01 C2 3B
RX  FF 03 02 00 0F D1 94
# 0x000F = 15%, no scaling

# Battery Stop State Of Charge (%), one register at 41116; SmartValue currently 5%
TX  FF 03 A0 9C 00 01 73 FA
RX  FF 03 02 00 05 51 93
# 0x0005 = 5%, no scaling

# Battery Full State Of Charge (%), one register at 41117; SmartValue currently 100%
TX  FF 03 A0 9D 00 01 22 3A
RX  FF 03 02 00 64 90 7B
# 0x0064 = 100%, no scaling

# Inverter 2 Line State of charge (%), one register at 41118; SmartValue currently 10%
TX  FF 03 A0 9E 00 01 D2 3A
RX  FF 03 02 00 0A 11 97
# 0x000A = 10%, no scaling

# Line Back 2 inverter State of Charge (%), one register at 41119; SmartValue currently 20%
TX  FF 03 A0 9F 00 01 83 FA
RX  FF 03 02 00 14 91 9F
# 0x0014 = 20%, no scaling; 10-point SOC hysteresis relative to 41118

# On Time Line Charge Enable, one register at 41120; raw 0, likely Disabled
TX  FF 03 A0 A0 00 01 B3 F6
RX  FF 03 02 00 00 91 90
# 0=Disabled and 1=Enabled are inferred pending SmartValue confirmation

# Line Charge Start Time1(HH-mm), one register at 41121; SmartValue currently 18:35
TX  FF 03 A0 A1 00 01 E2 36
RX  FF 03 02 12 23 DC E9
# 0x1223: hour=0x12=18, minute=0x23=35; packed HH:mm, NOT decimal HHmm

# Line Charge End Time 1(HH-mm), one register at 41122; SmartValue currently 00:00
TX  FF 03 A0 A2 00 01 12 36
RX  FF 03 02 00 00 91 90
# 0x0000: hour=0x00=0, minute=0x00=0; packed HH:mm; midnight handling unverified

# Line Charge Start Time2(HH-mm), one register at 41123; SmartValue currently 00:00
TX  FF 03 A0 A3 00 01 43 F6
RX  FF 03 02 00 00 91 90
# 0x0000: hour=0x00=0, minute=0x00=0; packed HH:mm

# Line Charge End Time 2(HH-mm), one register at 41124; SmartValue currently 00:00
TX  FF 03 A0 A4 00 01 F2 37
RX  FF 03 02 00 00 91 90
# 0x0000: hour=0x00=0, minute=0x00=0; packed HH:mm; equal start/end behavior unverified

# On time battery discharge enable, one register at 41127; SmartValue currently Enabled
TX  FF 03 A0 A7 00 01 02 37
RX  FF 03 02 00 01 50 50
# 0x0001=Enabled confirmed; 0=Disabled inferred; write unverified

# Battery Discharge Start Time 1(HH-mm), one register at 41128; SmartValue currently 21:34
TX  FF 03 A0 A8 00 01 32 34
RX  FF 03 02 15 22 1F 19
# 0x1522: hour=0x15=21, minute=0x22=34; packed HH:mm, NOT decimal HHmm

# Battery Discharge End Time 1(HH-mm), one register at 41129; decoded 20:59
TX  FF 03 A0 A9 00 01 63 F4
RX  FF 03 02 14 3B DF 43
# 0x143B: hour=0x14=20, minute=0x3B=59; packed HH:mm; overnight handling unverified

# Battery Discharge Start Time 2(HH-mm), one register at 41130; decoded 00:00
TX  FF 03 A0 AA 00 01 93 F4
RX  FF 03 02 00 00 91 90
# 0x0000: hour=0x00=0, minute=0x00=0; packed HH:mm

# Battery Discharge End Time 2(HH-mm), one register at 41131; SmartValue currently 00:00
TX  FF 03 A0 AB 00 01 C2 34
RX  FF 03 02 00 00 91 90
# 0x0000: hour=0x00=0, minute=0x00=0; packed HH:mm; equal start/end behavior unverified

# Battery Connection Way, one register at 41135; SmartValue currently Battery Parallel Input
TX  FF 03 A0 AF 00 01 83 F5
RX  FF 03 02 00 00 91 90
# Enum: 0=Battery Parallel Input (confirmed); 1=Battery Independent Input (inferred, unverified)
# SmartValue choices: Battery Parallel Input, Battery Independent Input; write unverified
# Raw 6 = L16 verified. Proposed zero-based enumeration 0..13 follows menu order; other 13 encodings are inferred, not captured.
# 41018: 0x180B = year 24, month 11
# 41019: 0x1E0B = day 30, hour 11
# 41020: 0x0726 = minute 7, second 38
# Confirmed against SmartValue display: 2024-11-30 11:07:38; inverter clock is behind the actual date

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
