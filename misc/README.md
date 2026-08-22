# Misc files

These files are related to drivers and will be needed at certain times.

`00-rdc.conf`: X.Org config file for this driver only.

`M2012-0.0.4.rom`: VBIOS for M2012 GPU (dump from `EBOX-3310MX`, BIOS Date: 08/16/2011, SoC is Vortex86MX+)

`M2012-0.0.8.rom`: VBIOS for M2012 GPU (dump from `EBOX-3350DX2`, BIOS Date: 02/10/2017, SoC is Vortex86DX2)

## Installation

`make install` copies both ROM files to `/usr/lib/firmware-rdc/` (the directory
is created if missing). The driver falls back to a ROM file from there when the
PCI option ROM is unreadable; the default file is `M2012-0.0.8.rom`. Select a
different firmware file (or an absolute path) with:

```conf
Section "Device"
	...
	Option "VBIOS" "M2012-0.0.8.rom"
	...
EndSection
```

## How to dump vbios rom

```bash
dd if=/dev/mem bs=1 skip=$((0xC0000)) count=32768 of=RDCVBIOS.ROM
```