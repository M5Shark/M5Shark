# M5SHARK MCP Server

Control your M5SHARK device from any AI assistant (Claude, ChatGPT, etc.) over USB using the Model Context Protocol.

## Setup

```bash
pip install pyserial mcp
```

## Connect to Claude Desktop

Add this to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "m5shark": {
      "command": "python",
      "args": [
        "C:\\path\\to\\m5shark_mcp.py",
        "--port", "COM6"
      ]
    }
  }
}
```

## Full Tool List (50+ tools)

### Connection
| Tool | Description |
|---|---|
| `connect` | Connect over USB (auto-detects port) |
| `disconnect` | Disconnect |
| `status` | Firmware, battery, SD, heap, scan mode |
| `send_command` | Send any raw CLI command |
| `help` | Full CLI help text |
| `version` | Firmware version string |

### Wi-Fi Scanning
| Tool | Description |
|---|---|
| `scan_ap` | Active survey (dual-band, hidden included) |
| `sniff_beacon` | Sniff beacon frames |
| `sniff_probe` | Sniff probe requests |
| `sniff_deauth` | Sniff deauth frames |
| `sniff_pmkid` | Capture EAPOL/PMKID handshakes |
| `sniff_raw` | Capture raw packets to PCAP |
| `packet_monitor` | Live packet counter with rates |
| `channel_analyzer` | Activity across all channels |
| `set_channel` | Set Wi-Fi channel (1-14, 36-177) |

### Wi-Fi Attacks (authorized targets only)
| Tool | Description |
|---|---|
| `beacon_spam` | Fake AP flood (spam/clone/rickroll/funny/mimic) |
| `deauth_flood` | Deauth frames to selected targets |
| `auth_rush` | Auth request flood |
| `cSA_attack` | Channel switch announcement |
| `evil_portal` | Captive portal on device AP |

### SSID Management
| Tool | Description |
|---|---|
| `ssid_add` | Add SSID to target list |
| `ssid_generate` | Generate random SSIDs (10-2000) |
| `ssid_list` | List all SSIDs |
| `ssid_clear` | Clear the list |
| `ssid_select` | Select SSID by index |

### Target Management
| Tool | Description |
|---|---|
| `list_aps` | List discovered access points |
| `select_ap` | Select AP as attack target |
| `list_stations` | List discovered stations |
| `select_station` | Select station as target |

### Bluetooth
| Tool | Description |
|---|---|
| `ble_scan` | Scan all nearby Bluetooth devices |
| `ble_scan_airtag` | Scan for AirTags / FindMy |
| `ble_scan_flipper` | Scan for Flipper Zero |
| `ble_scan_flock` | Scan for flock signatures |
| `ble_scan_meta` | Scan for Meta / Ray-Ban |
| `ble_scan_skimmers` | Scan for card skimmers |
| `ble_spam` | BLE advertising attack (7 types) |
| `ble_spoof_airtag` | Spoof an AirTag |
| `ble_stop` | Stop Bluetooth scan/attack |

### GPS
| Tool | Description |
|---|---|
| `gps_data` | Fix, satellites, signal, position, altitude, time |
| `gps_nmea` | Raw NMEA stream |
| `gps_tracker` | Start GPX logging |
| `gps_poi` | Mark point of interest |

### Wardrive
| Tool | Description |
|---|---|
| `wardrive_start` | Start wardriving (Wi-Fi + BLE + GPS to SD) |
| `stop_scan` | Stop any running tool |
| `list_sd` | List SD card files |
| `read_file` | Read a file from SD |

### BadUSB
| Tool | Description |
|---|---|
| `badusb_run` | Run Ducky script from SD |
| `badusb_type` | Type text on paired target |

### System
| Tool | Description |
|---|---|
| `battery` | Battery percentage and voltage |
| `heap` | Free heap and PSRAM |
| `reboot` | Reboot the device |
| `set_theme` | Change UI theme (4 options) |
| `set_led` | Set LED RGB color |
| `set_brightness` | Display brightness (0-9) |

### Settings
| Tool | Description |
|---|---|
| `settings_save` | Save settings |
| `settings_load` | Load settings |
| `settings_list` | List all settings |
| `factory_reset` | Reset to defaults |

## Example AI Conversations

- "Connect to my M5SHARK and check the battery"
- "Scan for nearby Wi-Fi networks and show me the results"
- "Find any Flipper Zeros nearby"
- "Start wardriving and tell me when you find networks"
- "What's my GPS location and how many satellites can I see?"
- "Set the theme to Cyber 2077"
- "Generate 100 random SSIDs for beacon spam"
- "Run a deauth attack on the selected AP"
- "What's my free heap memory?"
- "Read the wardrive log from the SD card"
