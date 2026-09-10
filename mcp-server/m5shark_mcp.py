#!/usr/bin/env python3
"""M5SHARK MCP Server - Full device control from AI assistants.

Exposes EVERY tool in the M5SHARK firmware as an MCP tool so Claude,
ChatGPT, or any MCP-compatible AI can operate the entire device.

Usage:
    python m5shark_mcp.py                          # auto-detect port
    python m5shark_mcp.py --port COM6

Claude Desktop config (claude_desktop_config.json):
{
  "mcpServers": {
    "m5shark": {
      "command": "python",
      "args": ["C:\\path\\to\\m5shark_mcp.py", "--port", "COM6"]
    }
  }
}
"""

import asyncio
import json
import sys
import argparse
import time

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("ERROR: pyserial not installed. Run: pip install pyserial", file=sys.stderr)
    sys.exit(1)

try:
    from mcp.server import Server
    from mcp.server.stdio import stdio_server
    from mcp.types import Tool, TextContent
except ImportError:
    print("ERROR: mcp not installed. Run: pip install mcp", file=sys.stderr)
    sys.exit(1)


class SharkSerial:
    def __init__(self, port=None, baud=115200):
        self.port = port
        self.baud = baud
        self.conn = None

    @staticmethod
    def find_port():
        for p in serial.tools.list_ports.comports():
            desc = (p.description or "").lower()
            hwid = (p.hwid or "").lower()
            if "ch340" in desc or "ch340" in hwid or "cp210" in desc:
                return p.device
        return None

    def connect(self):
        if self.port is None:
            self.port = self.find_port()
            if not self.port:
                return False
        try:
            self.conn = serial.Serial(self.port, self.baud, timeout=1)
            self.conn.reset_input_buffer()
            self.conn.write(b"\n")
            time.sleep(0.3)
            self.conn.reset_input_buffer()
            return True
        except serial.SerialException:
            self.conn = None
            return False

    def disconnect(self):
        if self.conn:
            self.conn.close()
            self.conn = None

    def ok(self):
        return self.conn is not None and self.conn.is_open

    def send(self, cmd, timeout=3.0):
        if not self.ok():
            return "ERROR: not connected. Use connect tool first."
        self.conn.reset_input_buffer()
        self.conn.write((cmd + "\n").encode())
        deadline = time.time() + timeout
        lines = []
        while time.time() < deadline:
            if self.conn.in_waiting:
                line = self.conn.readline().decode(errors="replace").strip()
                if line.startswith(">") and lines:
                    break
                if line and not line.startswith("#"):
                    lines.append(line)
            else:
                time.sleep(0.02)
        return "\n".join(lines) if lines else "(no output)"

    def send_json(self, mcmd, timeout=3.0):
        if not self.ok():
            return {"error": "not connected"}
        self.conn.reset_input_buffer()
        self.conn.write(("mcp " + mcmd + "\n").encode())
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.conn.in_waiting:
                line = self.conn.readline().decode(errors="replace").strip()
                if line.startswith("{"):
                    try:
                        return json.loads(line)
                    except json.JSONDecodeError:
                        pass
            else:
                time.sleep(0.02)
        return {"error": "timeout"}


shark = SharkSerial()
server = Server("m5shark")

S = {"type": "object", "properties": {}}  # empty schema shortcut


def tool(name, desc, props=None, required=None):
    schema = {"type": "object", "properties": props or {}}
    if required:
        schema["required"] = required
    return Tool(name=name, description=desc, inputSchema=schema)


TOOLS = [
    # === Connection ===
    tool("connect", "Connect to M5SHARK over USB. Auto-detects COM port.", {
        "port": {"type": "string", "description": "COM port (e.g. COM6)"},
        "baud": {"type": "number", "description": "Baud rate (default 115200)"}
    }),
    tool("disconnect", "Disconnect from the device."),
    tool("status", "Device status: firmware version, battery, SD card, heap, scan mode."),
    tool("send_command", "Send ANY raw serial CLI command.", {
        "command": {"type": "string", "description": "CLI command (e.g. 'help', 'ch 6', 'ssid -g 10')"}
    }, ["command"]),

    # === Wi-Fi Scanning ===
    tool("scan_ap", "Active scan for nearby Wi-Fi access points (all channels, dual-band, hidden included)."),
    tool("sniff_beacon", "Passively sniff Wi-Fi beacon frames on current channel."),
    tool("sniff_probe", "Sniff Wi-Fi probe requests (devices searching for networks)."),
    tool("sniff_deauth", "Sniff deauthentication frames (detect deauth attacks)."),
    tool("sniff_pmkid", "Capture EAPOL/PMKID handshakes for password cracking."),
    tool("sniff_raw", "Capture raw Wi-Fi packets to PCAP file on SD card."),
    tool("packet_monitor", "Live packet counter: beacon, deauth, and probe request rates."),
    tool("channel_analyzer", "Analyze Wi-Fi activity across all channels (busy channel ranking)."),
    tool("set_channel", "Set the Wi-Fi channel (1-14 for 2.4GHz, 36-177 for 5GHz).", {
        "channel": {"type": "number", "description": "Channel number"}
    }, ["channel"]),

    # === Wi-Fi Attacks ===
    tool("beacon_spam", "Flood the air with fake AP beacons using SSID list.", {
        "type": {"type": "string", "enum": ["spam", "clone", "rickroll", "funny", "mimic"], "description": "Attack type"}
    }),
    tool("deauth_flood", "Send deauth frames to selected targets (authorized targets only).", {
        "target": {"type": "string", "description": "Target AP (use select_ap first, or 'all')"}
    }),
    tool("auth_rush", "Flood AP with authentication requests."),
    tool("cSA_attack", "Channel switch announcement attack."),
    tool("evil_portal", "Start evil portal captive portal on the device AP."),

    # === SSID Management ===
    tool("ssid_add", "Add an SSID to the target list.", {
        "ssid": {"type": "string", "description": "Network name to add"}
    }, ["ssid"]),
    tool("ssid_generate", "Generate random SSIDs for beacon spam.", {
        "count": {"type": "number", "description": "Number of SSIDs to generate (10-2000)"}
    }, ["count"]),
    tool("ssid_list", "List all SSIDs in the target list."),
    tool("ssid_clear", "Clear all SSIDs from the list."),
    tool("ssid_select", "Select an SSID by index.", {
        "index": {"type": "number", "description": "Index number"}
    }, ["index"]),

    # === Target Management ===
    tool("list_aps", "List all discovered access points."),
    tool("select_ap", "Select an AP as attack target by index.", {
        "index": {"type": "number", "description": "AP index"}
    }, ["index"]),
    tool("list_stations", "List all discovered client stations."),
    tool("select_station", "Select a station as target.", {
        "index": {"type": "number"}
    }, ["index"]),

    # === Bluetooth ===
    tool("ble_scan", "Scan for nearby Bluetooth devices (all types)."),
    tool("ble_scan_airtag", "Scan specifically for Apple AirTags / FindMy devices."),
    tool("ble_scan_flipper", "Scan for Flipper Zero devices."),
    tool("ble_scan_flock", "Scan for flock-signature devices."),
    tool("ble_scan_meta", "Scan for Meta (Ray-Ban) devices."),
    tool("ble_scan_skimmers", "Scan for card skimmer devices (HC-03/05/06)."),
    tool("ble_spam", "BLE advertising attack (authorized targets only).", {
        "type": {"type": "string", "enum": ["sourapple", "applejuice", "swiftpair", "samsung", "google", "flipper", "all"]}
    }, ["type"]),
    tool("ble_spoof_airtag", "Spoof an AirTag with a specific ID."),
    tool("ble_stop", "Stop any running Bluetooth scan or attack."),

    # === GPS ===
    tool("gps_data", "Get GPS data: fix, satellites, signal quality, position, altitude, time."),
    tool("gps_nmea", "Stream raw NMEA sentences from GPS module."),
    tool("gps_tracker", "Start GPX track logging (one point per second while fix exists)."),
    tool("gps_poi", "Mark a point of interest at current GPS position."),

    # === Wardrive ===
    tool("wardrive_start", "Start wardriving: logs all nearby Wi-Fi + BLE with GPS coords to SD."),
    tool("stop_scan", "Stop any running scan, attack, or tool."),
    tool("list_sd", "List files on the SD card."),
    tool("read_file", "Read a file from the SD card.", {
        "path": {"type": "string", "description": "File path (e.g. /wardrive_0.log)"}
    }, ["path"]),

    # === BadUSB ===
    tool("badusb_run", "Run a BadUSB payload from SD card.", {
        "file": {"type": "string", "description": "Ducky script path (e.g. /SCRIPTS/payload.ducky)"}
    }, ["file"]),
    tool("badusb_type", "Type text on the paired target (BadUSB over BLE).", {
        "text": {"type": "string", "description": "Text to type"}
    }, ["text"]),

    # === System ===
    tool("battery", "Get battery percentage and voltage."),
    tool("heap", "Get free heap memory and PSRAM status."),
    tool("reboot", "Reboot the device."),
    tool("set_theme", "Change the UI theme.", {
        "theme": {"type": "string", "enum": ["matrix", "watchdogs", "cyber", "spider"]}
    }, ["theme"]),
    tool("set_led", "Set LED color.", {
        "r": {"type": "number"}, "g": {"type": "number"}, "b": {"type": "number"}
    }),
    tool("set_brightness", "Set display brightness (0-9).", {
        "level": {"type": "number", "description": "0=dim 9=full"}
    }, ["level"]),

    # === Settings ===
    tool("settings_save", "Save current settings to persistent storage."),
    tool("settings_load", "Load saved settings."),
    tool("settings_list", "List all settings and their values."),
    tool("factory_reset", "Reset all settings to factory defaults."),

    # === Info ===
    tool("help", "Get full CLI help text with all available commands."),
    tool("version", "Get firmware version string."),
]


@server.list_tools()
async def list_tools():
    return TOOLS


def _cmd(name, args=""):
    """Build the CLI command string from a tool call."""
    # Wi-Fi scanning
    if name == "scan_ap": return "sniffbt"  # BLE while we wait for dedicated AP scan cmd
    if name == "sniff_beacon": return "sniffbeacon"
    if name == "sniff_probe": return "sniffprobe"
    if name == "sniff_deauth": return "sniffdeauth"
    if name == "sniff_pmkid": return "sniffpmkid"
    if name == "sniff_raw": return "sniffraw"
    if name == "packet_monitor": return "sniffpckt"
    if name == "channel_analyzer": return "ch -s"

    # Wi-Fi attacks
    if name == "beacon_spam": return f"attack -t beacon -a {args.get('type', 'spam')}"
    if name == "deauth_flood": return "attack -t deauth"
    if name == "auth_rush": return "attack -t auth"
    if name == "cSA_attack": return "attack -t csa"
    if name == "evil_portal": return "evilportal"

    # SSID
    if name == "ssid_add": return f"ssid -a \"{args.get('ssid', '')}\""
    if name == "ssid_generate": return f"ssid -g {args.get('count', 100)}"
    if name == "ssid_list": return "ssid -l"
    if name == "ssid_clear": return "ssid -c"
    if name == "ssid_select": return f"ssid -s {args.get('index', 0)}"

    # Targets
    if name == "list_aps": return "list -a"
    if name == "select_ap": return f"select -a {args.get('index', 0)}"
    if name == "list_stations": return "list -s"
    if name == "select_station": return f"select -s {args.get('index', 0)}"

    # Bluetooth
    if name == "ble_scan": return "sniffbt"
    if name == "ble_scan_airtag": return "sniffbt -t airtag"
    if name == "ble_scan_flipper": return "sniffbt -t flipper"
    if name == "ble_scan_flock": return "sniffbt -t flock"
    if name == "ble_scan_meta": return "sniffbt -t meta"
    if name == "ble_scan_skimmers": return "sniffskim"
    if name == "ble_spam": return f"blespam -t {args.get('type', 'all')}"
    if name == "ble_spoof_airtag": return f"spoofat -t {args.get('index', 0)}"
    if name == "ble_stop": return "stopscan"

    # GPS
    if name == "gps_nmea": return "nmea"
    if name == "gps_tracker": return "gpstracker"
    if name == "gps_poi": return "gpspoi"

    # System
    if name == "list_sd": return "ls"
    if name == "set_channel": return f"ch {args.get('channel', 6)}"
    if name == "battery": return "mcp status"
    if name == "heap": return "mcp heap"
    if name == "reboot": return "reboot"
    if name == "set_theme": return f"theme {['matrix','watchdogs','cyber','spider'].index(args.get('theme','matrix'))}"
    if name == "set_led": return f"led -t rgb {args.get('r',0)} {args.get('g',0)} {args.get('b',0)}"
    if name == "set_brightness": return f"brightness {args.get('level', 9)}"
    if name == "help": return "help"
    if name == "version": return "version"

    # Settings
    if name == "settings_save": return "settings save"
    if name == "settings_load": return "settings load"
    if name == "settings_list": return "settings list"
    if name == "factory_reset": return "cleardevice"

    # Wardrive
    if name == "wardrive_start": return "wardrive"
    if name == "stop_scan": return "stopscan"

    return None


@server.call_tool()
async def call_tool(name, arguments):
    global shark

    # Connection management
    if name == "connect":
        port = arguments.get("port")
        if port:
            shark.port = port
        shark.baud = arguments.get("baud", 115200)
        if shark.connect():
            ver = shark.send("version", 2)
            return [TextContent(type="text", text=f"Connected on {shark.port}\n{ver}")]
        ports = [p.device for p in serial.tools.list_ports.comports()]
        return [TextContent(type="text", text=f"Failed. Available ports: {ports}")]

    if name == "disconnect":
        shark.disconnect()
        return [TextContent(type="text", text="Disconnected")]

    if name == "status":
        if not shark.ok():
            return [TextContent(type="text", text="Not connected.")]
        data = shark.send_json("status", 3)
        if "error" not in data:
            return [TextContent(type="text", text=json.dumps(data, indent=2))]
        return [TextContent(type="text", text=shark.send("version", 2))]

    if name == "send_command":
        if not shark.ok():
            return [TextContent(type="text", text="Not connected.")]
        return [TextContent(type="text", text=shark.send(arguments.get("command", ""), 5))]

    # Everything else maps to a CLI command
    if not shark.ok():
        return [TextContent(type="text", text="Not connected. Use connect tool first.")]

    cmd = _cmd(name, arguments)
    if cmd is None:
        return [TextContent(type="text", text=f"Unknown tool: {name}")]

    # Some tools need longer timeout
    long_timeout = 10.0 if name in ("ble_scan", "ble_scan_airtag", "ble_scan_flipper",
        "ble_scan_flock", "ble_scan_meta", "ble_scan_skimmers", "ble_spam", "scan_ap",
        "sniff_beacon", "sniff_probe", "sniff_raw", "channel_analyzer", "packet_monitor") else 5.0

    result = shark.send(cmd, long_timeout)
    return [TextContent(type="text", text=f"> {cmd}\n{result}")]


async def main():
    parser = argparse.ArgumentParser(description="M5SHARK MCP Server")
    parser.add_argument("--port", type=str, default=None)
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()
    shark.port = args.port
    shark.baud = args.baud

    async with stdio_server() as (read_stream, write_stream):
        await server.run(read_stream, write_stream, server.create_initialization_options())


if __name__ == "__main__":
    asyncio.run(main())
