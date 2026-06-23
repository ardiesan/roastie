# Wiring & Bill of Materials

## Parts List

| Part | Qty | Notes |
|------|-----|-------|
| ESP32 dev board (ESP-WROOM-32 or ESP32-CAM) | 1 | Must have PSRAM; AI-Thinker ESP32-CAM recommended |
| OV2640 camera module | 1 | Usually pre-attached to ESP32-CAM boards |
| MAX98357A I2S DAC breakout | 1 | 3.3 V logic, class-D amp |
| Speaker | 1 | 3 W – 5 W, 8 Ω, with short wire leads |
| 3.3 V regulator (if needed) | 1 | Built into ESP32-CAM; external if using bare ESP32 |
| Breadboard + jumper wires | 1 set | Male-to-male and male-to-female as needed |
| USB cable (5 V) | 1 | For power during testing |

## Pin Mapping

### ESP32 ↔ OV2640 Camera

| OV2640 Signal | ESP32 GPIO | Notes |
|---------------|-----------|-------|
| XCLK | 4 | Clock out |
| SIOD (SDA) | 26 | SCCB data |
| SIOC (SCL) | 27 | SCCB clock |
| D0 | 34 | Data bit 0 |
| D1 | 5 | Data bit 1 |
| D2 | 18 | Data bit 2 |
| D3 | 19 | Data bit 3 |
| D4 | 36 | Data bit 4 |
| D5 | 39 | Data bit 5 |
| D6 | 35 | Data bit 6 |
| D7 | 34 | Data bit 7 — **see note** |
| VSYNC | 25 | Vertical sync |
| HREF | 23 | Horizontal reference |
| PCLK | 22 | Pixel clock |

> **Note on AI-Thinker ESP32-CAM:** D0 and D6 share GPIO 34 on the original board schematic. This is a known hardware limitation that does not affect JPEG output (the sensor encodes data sequentially). If using a custom board, wire D0→34 and D7→35 separately.

### ESP32 ↔ MAX98357A DAC

| MAX98357A Pin | ESP32 GPIO | Notes |
|---------------|-----------|-------|
| BCLK | 18 | Bit clock (16 kHz) |
| LRC (WS) | 23 | Left/right clock |
| DIN | 19 | Serial data (16-bit PCM) |
| GND | GND | Common ground |
| VIN | 3.3 V | 2.5 V – 3.6 V |
| GAIN | NC | Leave unconnected (default +6 dB) |

### ESP32 ↔ Status LED

| LED Pin | ESP32 GPIO | Notes |
|---------|-----------|-------|
| Anode (+) | 2 | Built-in LED on most boards (active-high) |
| Cathode (-) | GND | Or use a 220 Ω series resistor if external LED |

### ESP32 ↔ Speaker

| Speaker Pin | MAX98357A | Notes |
|-------------|-----------|-------|
| + | OUT1 | Output A |
| - | OUT2 | Output B (bridge-tied load) |

## Wiring Diagram

```
  ┌──────────────┐            ┌───────────────┐            ┌─────────┐
  │  ESP32 Board │            │ MAX98357A DAC │            │ Speaker│
  │              │            │               │            │         │
  │ GPIO 4   ────┼───────────►│ XCLK (OV2640) │            │         │
  │ GPIO 26  ────┼───────────►│ SDA (OV2640)  │            │         │
  │ GPIO 27  ────┼───────────►│ SCL (OV2640)  │            │         │
  │ GPIO 34-39 ──┼───────────►│ D0-D7 (OV2640)│            │         │
  │ GPIO 25  ────┼───────────►│ VSYNC         │            │         │
  │ GPIO 23  ────┼───────────►│ LRC           │            │         │
  │ GPIO 22  ────┼───────────►│ PCLK          │            │         │
  │ GPIO 18  ────┼───────────►│ BCLK          │            │         │
  │ GPIO 19  ────┼───────────►│ DIN           │            │         │
  │ 3.3 V    ────┼───────────►│ VIN           │            │         │
  │ GND      ────┼───────────►│ GND           │            │         │
  │              │            │               │            │         │
  │              │            │ OUT1 ─────────┼──+──► Speaker +
  │              │            │ OUT2 ─────────┼──┴──► Speaker -
  └──────────────┘            └───────────────┘            └─────────┘
```

## Power Notes

- The ESP32 draws up to ~500 mA during Wi-Fi transmission. Use a **minimum 2 A** USB power supply.
- The MAX98357A is powered from the same 3.3 V rail. A typical 3 W speaker draws ~0.4 W at demo volume — well within the 3.3 V rail capacity.
- For battery operation, a 2-cell LiPo (7.4 V) through a 3.3 V buck converter works well.
