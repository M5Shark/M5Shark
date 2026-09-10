![M5SHARK R151](m5shark-r150-readme-banner.gif)

# M5SHARK R151

[![Buy M5SHARK](https://img.shields.io/badge/Buy-M5SHARK-blue?style=for-the-badge&logo=shopify)](https://m5shark.com/products/m5shark-marauder-v8)

![Overview](assets/01-overview.png)

A hand-held wireless security testing device built on the ESP32-C5, running the SHARK firmware with terminal UI, theme engine, 50+ tools, and full AI assistant control via MCP.

![Version](https://img.shields.io/badge/version-R152-blue) ![Chip](https://img.shields.io/badge/chip-ESP32--C5-orange) ![MCP](https://img.shields.io/badge/AI--Control-58%20MCP%20Tools-purple) ![License](https://img.shields.io/badge/license-GPL--3.0-lightgrey)

> **Get the device:** [m5shark.com/products/m5shark-marauder-v8](https://m5shark.com/products/m5shark-marauder-v8)

![divider](assets/10-ascii-divider.gif)

## Hardware

![Hardware](assets/02-hardware.png)

| Component | Specification |
|---|---|
| MCU | ESP32-C5 (RISC-V dual-core) |
| Flash | 8 MB (custom `shark_8mb` partition: 2× 3.6 MB OTA slots + 1.1 MB SPIFFS) |
| RAM | PSRAM enabled |
| Display | 240×320 ILI9341-class resistive touch, 40 MHz SPI |
| Radio | Wi-Fi 2.4+5 GHz / Bluetooth 5 (NimBLE) |
| GPS | UART module with external antenna (u.FL/SMA) |
| Storage | microSD (FAT32, dedicated SPI) |
| Battery | IP5306 power management (I²C) |
| USB | CH340 serial bridge for flashing + console |

### 3D Printable Case

STL files for the M5SHARK enclosure are in [`hardware/m5shark-v8-3d-case-stl.rar`](hardware/m5shark-v8-3d-case-stl.rar).

![divider](assets/10-ascii-divider.gif)

## Features

![Features](assets/features.png)

### Wi-Fi Tools

![Wireless](assets/03-wireless.png)

- **Scan AP** — active dual-band survey (2.4+5 GHz, hidden networks), per-network detail (SSID, channel+band, security type, RSSI bar, OPEN flagged), tap-to-select targets for attacks
- **Wardrive** — WiGLE-format CSV logging (Wi-Fi + BLE rows), works without GPS fix (0-coords until lock), TAG POI marker, SURVEY LOG dashboard, GPX POI files
- **MAC Monitor** — top-talker device tracking (WiFi + BLE), follow marker, amber highlight
- **Beacon/Probe/EAPOL/Deauth sniffers** — PCAP capture to SD
- **Packet Monitor** — live per-type bar graphs (MGMT/DATA/CTRL)
- **Channel Analyzer** — auto-sweep spectrum, busy-channel ranking
- **Pineapple Watch** / **Multi-SSID Watch** — rogue AP detection
- **Attacks** — beacon spam, auth rush, deauth, CSA, evil portal (authorized targets only)

### Bluetooth Tools

- **Sniffers** — FindMy Sniff/Monitor, Flipper, Flock, Meta Detect, Card Skimmers, Bluetooth Analyzer, Fox Hunt
- **Attacks** — Sour Apple, Apple Juice, Swiftpair, Samsung, Google, Flipper, Spam All, Spoof Airtag (deinit-free, MAC rotation at runtime)
- **BLE crash guard** — if BLE init ever crashes, next boot locks BLE out (auto-recovers after 5 clean boots)

### BadUSB Suite (over BLE)

Eight tools: SD payload picker (Ducky scripts), type text, self-test, Win recon, Win Wi-Fi keys, Linux recon, parameterized bash reverse shell, lock target. Free modifier combos (`CTRL-ALT t`, `GUI SHIFT s`).

### GPS Tools

![Signal Flow](assets/09-signal-flow.gif)

- **GPS Data** — fix pill (READY/NO FIX/NO ANT), SIGNALS + BEST dB live signal meter (GSV parsed), position panel, ANTENNA OPEN warning, animated SEARCHING counter
- **GPX Tracker** — one track point per second while fix exists
- **Wardrive integration** — coordinates auto-populate on lock

### Chameleon Ultra Remote

Full BLE central link with auto-connect/auto-reconnect. Nordic UART Service (verified from CU firmware source). Dashboard (version/mode/battery/slot), mode toggle, slot switching (1-8), READ HF (ISO14443-A UID/ATQA/SAK), READ LF (EM4100), EMULATE HF/LF (MIFARE Classic 1K / EM4100 on active slot).

### SSID Generator

Count steppers (10-2000), presets, GENERATE random names, ADD SSID via touch keyboard, CLEAR list. Feeds Beacon Spam / Probe Flood / Evil Portal.

### Web Control

Browser dashboard over the device's AP: live scan results, attack control, SD file manager, Evil Portal HTML management, wardrive log download, wallpaper upload.

### Theme Engine

![Themes](assets/07-themes.png)

Four themes: **Matrix** (green phosphor, default), **Watch Dogs** (bone white + cyan), **Cyber 2077** (yellow neon), **Spider** (red/blue). NVS-persisted, splash art matches active theme.

![divider](assets/10-ascii-divider.gif)

## Flashing

![Flashing](assets/05-flashing.png)

### USB flash from PC (recommended)

Download a flash kit from [Releases](https://github.com/M5Shark/M5SHARK/releases), extract, then:

```powershell
cd <extracted folder>
powershell -ExecutionPolicy Bypass -File ".\flash-full-fast.ps1" -Port COM6
```

Replace `COM6` with your device's port (Device Manager → Ports).

- If it won't connect: **hold BOOT, tap RESET, release after 2 seconds**, then retry
- If the write cuts out: add `-Baud 460800`
- Wait for **"WRITE COMPLETED"** in green, then **press RESET**

### SD card update (no PC, R138+ only)

Copy `update.bin` to the SD card root → Card Control → Update Firmware → select the file. A live progress bar and percentage counter runs during the write.

> **Important:** R138 changed the partition table (bigger 3.6 MB OTA slots). Devices on R137 or older must be updated over USB with a **full flash** (not `-Quick`, not SD update).

![divider](assets/10-ascii-divider.gif)

## Serial CLI

![Serial CLI](assets/06-serial-cli.png)

Connect via any terminal (PuTTY, Arduino Serial Monitor) at 115200 baud:

```
help                              full command list
wardrive                          start wardrive
sniffbt                           Bluetooth scan all
sniffbt -t airtag/flipper/flock/meta   targeted passive sniffs
blespam -t sourapple/all          BLE advertising attacks
gpsdata                           full GPS block (fix, sats, antenna, position)
nmea                              raw NMEA stream (antenna status visible)
stopscan                          stop the running tool
reboot                            restart
cleardevice                       factory-clear settings
```

![divider](assets/10-ascii-divider.gif)

## Building from source

![Building](assets/building-from-source.png)

```bash
powershell -ExecutionPolicy Bypass -File build-shark-v8.ps1
```

The build uses:
- Arduino ESP32 core 3.3.4 (isolated config: `arduino-cli-esp32-3.3.4.yaml`)
- NimBLE-Arduino 2.5.1 (vendored in `.build-libraries/`)
- TFT_eSPI (vendored, User_Setup.h pins validated by the build script)
- Custom `shark_8mb` partition layout (`m5shark/partitions.csv`)

![divider](assets/10-ascii-divider.gif)

## Architecture

![Architecture](assets/architecture.png)

```
m5shark/
├── m5shark.ino          # main setup/loop, boot trace, splash
├── configs.h            # hardware pin map, display geometry, feature flags
├── WiFiScan.cpp/.h      # all Wi-Fi scanning, attacks, wardrive, CLI helpers
├── MenuFunctions.cpp/.h # menu system, all dashboards, touch handling
├── SharkUI.cpp/.h       # boot splash (skull morph + DedSec), idle wallpaper
├── SharkTheme.cpp/.h    # 4-theme engine with NVS persistence
├── SharkWeb.cpp/.h      # Web Control AP + browser dashboard
├── SharkPrank.cpp/.h    # WiFi + BLE prank suite (monkey, portals, hunts)
├── SharkChameleon.cpp/.h # Chameleon Ultra BLE remote
├── BadUsb.cpp/.h        # BLE BadUSB suite (8 tools, Ducky engine)
├── SharkIceNav.cpp/.h   # GPS navigation radar
├── Display.cpp/.h       # display primitives, touch, scan cards
├── CommandLine.cpp/.h   # serial CLI
├── GpsInterface.cpp/.h  # GPS UART + GSV signal meter
├── SDInterface.cpp/.h   # SD card + firmware update
└── ...                  # EvilPortal, WdgResponse, buffer, settings, etc.
```

![divider](assets/10-ascii-divider.gif)

## AI Assistant Control (MCP)

Control the ENTIRE device from Claude, ChatGPT, or any AI assistant over USB cable. 58 tools covering every firmware capability.

### Setup

```bash
pip install pyserial mcp
```

Add to Claude Desktop (`claude_desktop_config.json`) — **port is auto-detected, just plug in the device**:

```json
{
  "mcpServers": {
    "m5shark": {
      "command": "python",
      "args": ["C:\\path\\to\\m5shark_mcp.py"]
    }
  }
}
```

### All 58 AI Tools

| Category | Tools |
|---|---|
| Connection | connect, disconnect, status, send_command, help, version |
| Wi-Fi Scan | scan_ap, sniff_beacon, sniff_probe, sniff_deauth, sniff_pmkid, sniff_raw, packet_monitor, channel_analyzer, set_channel |
| Wi-Fi Attacks | beacon_spam, deauth_flood, auth_rush, cSA_attack, evil_portal |
| SSID | ssid_add, ssid_generate, ssid_list, ssid_clear, ssid_select |
| Targets | list_aps, select_ap, list_stations, select_station |
| Bluetooth | ble_scan, ble_scan_airtag, ble_scan_flipper, ble_scan_flock, ble_scan_meta, ble_scan_skimmers, ble_spam, ble_spoof_airtag, ble_stop |
| GPS | gps_data, gps_nmea, gps_tracker, gps_poi |
| Wardrive | wardrive_start, stop_scan, list_sd, read_file |
| BadUSB | badusb_run, badusb_type |
| System | battery, heap, reboot, set_theme, set_led, set_brightness |
| Settings | settings_save, settings_load, settings_list, factory_reset |

### Prompt Examples — Getting Started

```
Connect to my M5SHARK and give me a full status report
```

```
What firmware version am I running and how's my battery?
```

```
Show me the help text so I can see all available commands
```

```
How much free memory do I have?
```

### Prompt Examples — Wi-Fi Reconnaissance

```
Scan for all nearby Wi-Fi networks and show me the results with signal strength
```

```
What channel is my device on right now? Switch it to channel 6
```

```
Run a channel analysis and tell me which channels are the busiest
```

```
Start a packet monitor and tell me what kind of traffic is in the air
```

```
Sniff for probe requests — what devices are looking for networks?
```

```
Are there any hidden networks nearby?
```

```
Find all open (unsecured) Wi-Fi networks nearby and rank them by signal strength
```

### Prompt Examples — Wardriving

```
Start wardriving and let me know when you've found more than 10 networks
```

```
Start wardriving, then after 30 seconds stop it and show me what was logged
```

```
Read the latest wardrive log from the SD card
```

```
Mark this location as a point of interest
```

```
I'm wardriving. Every time you find a new network, tell me the SSID, channel, and signal strength. Also track my GPS position.
```

### Prompt Examples — GPS

```
What's my GPS status? Do I have a fix and how many satellites?
```

```
Show me my exact latitude, longitude, and altitude
```

```
What's my signal quality — best dB and signal count?
```

```
Start GPS tracking and log my route
```

### Prompt Examples — Bluetooth

```
Scan for all nearby Bluetooth devices
```

```
Are there any Flipper Zeros near me?
```

```
Check for AirTags or FindMy devices nearby
```

```
Look for card skimmers in the area
```

```
Scan for Meta Ray-Ban devices
```

```
Stop the Bluetooth scan
```

### Prompt Examples — Wi-Fi Attacks (authorized targets only)

```
Generate 50 random SSIDs for beacon spam
```

```
List all the SSIDs I have in my target list
```

```
List all the access points you've found, then select the first one as target
```

```
Run a deauth flood on the selected access point
```

```
Start an evil portal
```

```
Stop all attacks now
```

```
Set up the device for a penetration test: generate 100 SSIDs, select the target AP, and prepare a beacon spam — but don't start the attack yet
```

### Prompt Examples — BadUSB

```
Run the BadUSB payload at /SCRIPTS/payload.ducky
```

```
Type "Hello from Claude" on the paired target
```

### Prompt Examples — System & Settings

```
Change the theme to Cyber 2077
```

```
Set the display brightness to level 5
```

```
Set the LED to red
```

```
Show me all my current settings
```

```
Save my current settings
```

```
Factory reset the device
```

```
Reboot my device
```

### Prompt Examples — Advanced (chained operations)

```
Connect to my M5SHARK, check the battery, scan for nearby Wi-Fi, and tell me the top 5 strongest networks
```

```
Do a full site survey: scan Wi-Fi, scan Bluetooth, check GPS, analyze channels, then give me a summary report
```

```
Monitor for deauthentication attacks for 30 seconds and tell me if anyone is trying to kick devices off the network
```

```
I'm doing a security assessment. Connect to the device, survey the area (Wi-Fi + Bluetooth), identify all open networks and potential rogue APs, then give me a threat report
```

![divider](assets/10-ascii-divider.gif)

## Troubleshooting

![Troubleshooting](assets/troubleshooting.png)

| Symptom | Cause | Fix |
|---|---|---|
| Boot-loops on splash (R117/R118 era) | Bad BLE library linked at build | Flash R151 (full flash) |
| BLUETOOTH LOCKED notice | BLE init crashed once; guard engaged | Auto-recovers after 5 clean boots |
| Flash won't connect | Auto-reset missed bootloader | Hold BOOT, tap RESET, release after 2 s |
| Write interrupted | USB dropout mid-write | Run same command again |
| Screen dark after flash | Chip waiting for reset | Press RESET once |
| GPS stays NO FIX / NO ANT | Antenna circuit open or weak sky view | Reseat antenna; watch SIGNALS/BEST dB outdoors |
| Wardrive logs 0,0 coords | No GPS fix yet (by design) | Coords fill in once receiver locks |
| Old-style screen flashes | Pre-R132 firmware | Update to R151 |

![divider](assets/10-ascii-divider.gif)

## Version history

![Version History](assets/version-history.png)

Key milestones:
- **R119** — splash boot-loop fixed (BLE crash guard)
- **R151** — MCP USB control (58 AI tools, full device control from Claude)
- **R120-121** — BadUSB suite + control fixes
- **R122** — Wardrive SHARK dashboard
- **R126** — Wardrive works without GPS fix
- **R128** — MAC Monitor dashboard
- **R129** — SSID Generator
- **R132** — no legacy UI flash on any tool
- **R134** — 40 MHz display (2× speed)
- **R138** — GPS Tracker + bigger OTA slots (3.6 MB)
- **R141** — GPS BEST dB signal meter
- **R142** — Scan AP active survey
- **R143** — Matrix green default theme
- **R144-151** — Chameleon Ultra remote (auto-connect, NUS, emulation)

![divider](assets/10-ascii-divider.gif)

## Legal

![Legal](assets/legal.png)

![Footer](assets/08-footer.png)

Licensed under GPL-3.0. See [LICENSE](LICENSE).

**Authorized testing only.** Use this firmware only on networks, devices, and systems you own or are explicitly permitted to test. You are responsible for complying with the laws that apply where you operate it.

[Get M5SHARK](https://m5shark.com/products/m5shark-marauder-v8)
