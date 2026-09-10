# M5SHARK ASCII-terminal icon pack

This pack replaces all 72 firmware icon slots used by the M5SHARK menus and
status bar.  The visual direction comes from the supplied gray-on-black ASCII
skull reference: stark monochrome terminal graphics, character-like broken
strokes, and bold silhouettes that remain legible on the 240x320 R8 display.

## Change any icon

1. Replace the matching 128x128 PNG in `source/`.  Use white/light-gray artwork
   on black (transparent backgrounds also work after compositing to black).
2. From the project root, run:

   ```powershell
   python tools\generate_ascii_icon_assets.py
   ```

3. Rebuild the firmware with `build-shark-v8.ps1`.

The generator preserves edited source PNGs during a normal run.  It writes the
native 22x22 previews to `firmware-22x22/`, refreshes
`esp32_marauder/SharkAsciiIcons.h`, and rebuilds the labeled contact sheet.

`--refresh-source` intentionally discards source-folder edits and re-crops the
AI-generated master sheet:

```powershell
python tools\generate_ascii_icon_assets.py --refresh-source
```

## Generation provenance

- Mode: built-in image generation tool
- Reference: `reference-ascii-skull.png` (style reference only)
- Master: `ascii-terminal-master-v1.png`
- Firmware format: 72 XBM-compatible, LSB-first, 1-bit icons at 22x22 pixels
- Runtime behavior: UI assets only; radio, GPS, SD, touch, BLE, and scan logic
  are unchanged

The 18 scanner icons at indices 50–67, Beacon List Attack at index 68, Random
Beacon Spam at index 69, Funny SSID Beacon at index 70, and Rick Roll Beacon
at index 71 are purpose-built code-native art from
`scanner_source()` in `tools/generate_ascii_icon_assets.py`. They regenerate
deterministically and do not depend on cells from the older master contact
sheet.

Final generation prompt:

> Create a monochrome contact sheet of cyber-terminal pictograms for every
> firmware icon category, inspired by the supplied ASCII-character skull.
> Render crisp white and light-gray symbols from terminal characters on pure
> black, with no labels, logos, gradients, color, or watermark. Keep every
> silhouette distinct and recognizable when reduced to 22x22 pixels.
