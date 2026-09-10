$ErrorActionPreference = 'Continue'
$PSNativeCommandUseErrorActionPreference = $false

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$arduinoCli = Join-Path $projectRoot '.tools\arduino-cli\arduino-cli.exe'
$configFile = Join-Path $projectRoot 'arduino-cli-esp32-3.3.4.yaml'
$libraryRoot = Join-Path $projectRoot '.build-libraries'
$outputRoot = Join-Path $projectRoot 'build\shark-v8-sd'
$maxOtaBytes = 0x370000  # OTA slot under the sketch's custom partitions.csv (m5shark\partitions.csv, auto-picked-up by the core's prebuild hook; was 0x330000 under default_8MB)
$tftSetup = Join-Path $libraryRoot 'TFT_eSPI\User_Setup.h'

if (-not (Test-Path -LiteralPath $arduinoCli)) {
  throw 'Arduino CLI is missing from .tools\arduino-cli.'
}

if (-not (Test-Path -LiteralPath $configFile)) {
  throw 'The isolated Arduino-ESP32 3.3.4 configuration is missing.'
}

$expectedDisplayDefines = [ordered]@{
  ILI9341_DRIVER = $null
  TFT_MISO = '2'
  TFT_MOSI = '7'
  TFT_SCLK = '6'
  TFT_CS = '23'
  TFT_DC = '24'
  TFT_RST = '-1'
  TFT_BL = '8'
  TOUCH_CS = '9'
  SPI_FREQUENCY = '40000000'
}

$tftSetupText = Get-Content -LiteralPath $tftSetup -Raw
foreach ($define in $expectedDisplayDefines.GetEnumerator()) {
  $pattern = if ($null -eq $define.Value) {
    '(?m)^\s*#define\s+' + [regex]::Escape($define.Key) + '\s*(?://.*)?$'
  }
  else {
    '(?m)^\s*#define\s+' + [regex]::Escape($define.Key) + '\s+' + [regex]::Escape($define.Value) + '(?:\s|$)'
  }
  if ($tftSetupText -notmatch $pattern) {
    throw "Marauder v8 display definition is missing or incorrect: $($define.Key)=$($define.Value)"
  }
}

New-Item -ItemType Directory -Path $outputRoot -Force | Out-Null

$compileArgs = @(
  'compile',
  '--config-file', $configFile,
  '--clean',
  '--jobs', '1',
  '--fqbn', 'esp32:esp32:esp32c5:FlashSize=8M,PartitionScheme=default_8MB,PSRAM=enabled',
  '--warnings', 'none',
  # Custom 8 MB layout: the sketch's partitions.csv (m5shark\partitions.csv,
  # auto-copied by the core's prebuild hook) grows the OTA app slots from
  # 0x330000 to 0x370000 (~260 KB headroom per slot; SPIFFS 1.5 MB -> 1.1 MB).
  # 'build.partitions' must stay a bare name (it feeds -DARDUINO_PARTITION_xxx),
  # and arduino-cli's size check reads 'upload.maximum_size' from the menu,
  # so both are overridden here.
  '--build-property', 'build.partitions=shark_8mb',
  '--build-property', 'upload.maximum_size=3604480',
  # Windows Application Control blocks this toolchain's unsigned lto-wrapper.
  # Keep the reproducible build policy-safe; the final slot-size guard below
  # still rejects any image that cannot fit the dual-OTA layout.
  #
  # The explicit -I pins the esp32c5 coexistence headers for NimBLE's
  # nimble_port.c (#include "esp_coexist_internal.h" under
  # CONFIG_SW_COEXIST_ENABLE). The platform include list resolves them
  # inconsistently between clean builds on this host; pinning the path
  # makes every build compile the same way.
  '--build-property', ('compiler.c.extra_flags=-Oz -fno-lto -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-strict-aliasing' +
    ' -IC:/Users/itsom/.arduino334/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-8410210c-v2/esp32c5/include/esp_coex/include' +
    ' -IC:/Users/itsom/.arduino334/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-8410210c-v2/esp32c5/include/esp_coex/include/private'),
  '--build-property', ('compiler.cpp.extra_flags=-Oz -fno-lto -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-strict-aliasing' +
    ' -IC:/Users/itsom/.arduino334/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-8410210c-v2/esp32c5/include/esp_coex/include' +
    ' -IC:/Users/itsom/.arduino334/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-8410210c-v2/esp32c5/include/esp_coex/include/private'),
  '--build-property', 'compiler.S.extra_flags=-Oz -fno-lto',
  # collect2.exe is also blocked by Windows Application Control on this host,
  # so the driver links through ld directly. collect2 normally filters driver
  # flags before ld sees them; without it a link-time -fno-lto leaks through
  # as bare "-f" and ld rejects it. LTO is already disabled per-translation-
  # unit above, so the link stage needs no -fno-lto of its own.
  '--build-property', 'compiler.c.elf.extra_flags=-Wl,-zmuldefs',
  # gcc-ar.exe is an LTO-aware wrapper and is blocked by Windows Application
  # Control on this host. This build disables LTO, so the signed/allowed GNU
  # archiver is the correct equivalent and avoids producing a stale image.
  '--build-property', 'compiler.ar.cmd={compiler.prefix}ar',
  '--libraries', $libraryRoot,
  '--output-dir', $outputRoot,
  (Join-Path $projectRoot 'sharkfw')
)

# ProcessStartInfo.ArgumentList does not exist on the .NET Framework runtime
# behind Windows PowerShell 5.1 (it is a .NET Core API), so building it that way
# silently launches arduino-cli with no arguments and then "validates" a stale
# binary. Invoke the CLI directly with the call operator and splatting, which
# passes every argument correctly on 5.1, and stamp the image first so a stale
# artifact can never masquerade as a fresh build.
$applicationImage = Join-Path $outputRoot 'm5shark.ino.bin'
if (Test-Path -LiteralPath $applicationImage) {
  Remove-Item -LiteralPath $applicationImage -Force
}

& $arduinoCli @compileArgs
$compileExitCode = $LASTEXITCODE

if ($compileExitCode -ne 0) {
  throw "SHARK v8 build failed with exit code $compileExitCode."
}

if (-not (Test-Path -LiteralPath $applicationImage)) {
  throw 'SHARK v8 build reported success but produced no application image.'
}
$applicationBytes = (Get-Item -LiteralPath $applicationImage).Length
if ($applicationBytes -gt $maxOtaBytes) {
  throw "SHARK v8 image is $applicationBytes bytes, exceeding the 8 MB dual-OTA slot ($maxOtaBytes bytes)."
}

Write-Host "SD OTA image verified: $applicationBytes / $maxOtaBytes bytes."
