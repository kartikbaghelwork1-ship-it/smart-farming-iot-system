# Hardware Wiring Diagram — MITS-DU IoT Trainer Kit

## ESP32 GPIO Pin Mapping

```
                    ┌──────────────────────┐
                    │     ESP32 DevKit      │
                    │                      │
        DHT22 ──────┤ GPIO 25         3V3 ├────── Sensor VCC
                    │                      │
   Soil Moist ──────┤ GPIO 34 (ADC)   GND ├────── Common GND
                    │                      │
     pH Sensor ─────┤ GPIO 35 (ADC)       │
                    │                      │
    Relay IN ───────┤ GPIO 26             │
                    │                      │
   RS485 RO ────────┤ GPIO 16 (RX2)       │
                    │                      │
   RS485 DI ────────┤ GPIO 17 (TX2)       │
                    │                      │
   RS485 DE/RE ─────┤ GPIO 4              │
                    │                      │
                    └──────────────────────┘
```

## RS485 Sensor Connections

```
ESP32                RS485 Module              Sensors
┌──────┐            ┌───────────┐            ┌─────────────┐
│ TX2  ├────────────┤ DI        │            │ MST Sensor  │
│ (17) │            │           │    A ──────┤ (Addr: 0x01)│
│      │            │        A  ├────────────┤ Temp/Moist/ │
│ RX2  ├────────────┤ RO        │            │ EC          │
│ (16) │            │           │    B ──────┤             │
│      │            │        B  ├────────┐   └─────────────┘
│ D4   ├────────────┤ DE + RE   │        │
│      │            │           │        │   ┌─────────────┐
│ GND  ├────────────┤ GND       │        ├───┤ NPK Sensor  │
│      │            │           │        │   │ (Addr: 0x55)│
│ 3V3  ├────────────┤ VCC       │        └───┤ N/P/K       │
└──────┘            └───────────┘            └─────────────┘
```

## Relay + Pump Connection

```
ESP32 GPIO 26 ──────→ Relay IN
                      Relay COM ──────→ Pump Power (+)
                      Relay NO  ──────→ Power Supply (+)
                      Relay GND ──────→ Common GND

⚠️  Relay is Active LOW (LOW = ON, HIGH = OFF)
⚠️  Never connect pump directly to ESP32 GPIO!
⚠️  Use external power supply for the pump
```

## Sensor Power Requirements

| Sensor | Voltage | Current |
|--------|---------|---------|
| DHT22 | 3.3V - 5V | ~2.5mA |
| Soil Moisture | 3.3V - 5V | ~35mA |
| pH Sensor Module | 5V | ~10mA |
| RS485 Module | 3.3V - 5V | ~120mA |
| MST Sensor | 12V/24V | Check spec |
| NPK Sensor | 12V/24V | Check spec |
| Relay Module | 5V | ~70mA |

> **Note:** ESP32 GPIO is NOT 5V tolerant. Use level shifters or voltage dividers for 5V sensor outputs.
