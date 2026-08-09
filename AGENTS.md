# xf86-video-rdc

## Project

`xf86-video-rdc` — X.Org DDX driver for RDC Semiconductor GPU IPs (**M2012**, **M2015**) found in the Vortex86 SoC family.

- **Driver name**: `rdcm15` → module `rdcm15_drv.so` (entry symbol `rdcm15ModuleData`).
- **Target platform**: 32-bit i586 (Vortex86), Xorg ≥ 1.13 (developed/tested on Xorg 21.1). Old Xorg (≤1.7) and XAA are **not** supported.
- **Chipsets**: M2010_A0, M2010, M2011, M2012, M2013, M2014, M2015, M2200. M2012/M2015 are single-display; the `rdcdual_*` dual-CRTC path is only built for ancient Xorg (`HAVE_DUAL`) and is effectively dead on modern servers.

## Build & install

```bash
bash autogen.sh                 # required after any Makefile.am change
./configure --prefix=/usr       # --prefix=/usr is REQUIRED (module goes to /usr/lib/xorg/modules)
make
sudo make install
```

`make install` installs:
- `$(moduledir)/drivers/rdcm15_drv.so` — the driver module
- `$(moduledir)/drivers/RDCVBIOS.ROM` — VBIOS (this board's PCI option ROM is unreadable; the driver falls back to this file at runtime)
- `/etc/X11/xorg.conf.d/00-rdc.conf` — **hardcoded** path (Xorg reads snippets from `/etc/X11/xorg.conf.d` regardless of `sysconfdir`; AOSC uses `/usr/etc` so `$(sysconfdir)/...` is wrong here). Installed only if absent.

Clean (removes *all* autotools artifacts, not just objects):

```bash
make clean
```

Build prerequisites: `pkg-config`, autoconf/automake/libtool, `xorg-server` (dev headers), `pixman-1`. `AM_CFLAGS` already carries `-std=gnu99 -D_DEFAULT_SOURCE` (needed for `usleep`/POSIX decls under GCC ≥14).

## Hardware constraints (these are why the port looks unusual — do not "fix" them away)

1. **No `iopl()` on the target kernel** (`xf86EnableIO` logs "Function not implemented"). Raw `inb`/`outb` (compiler.h inline asm) faults with SIGSEGV. Therefore:
   - All VGA port I/O goes through the GPU's **MMIO port alias**: `rdc_vgatool.c` `InPort`/`OutPort` use the global `RDC_IOBase` (set via `vSetRDCIOBase()`, base = `MMIOVirtualAddr + port`). Same mechanism as `CInt10.c` (`pRelated_IOAddress = pCBIOSExtension->pjIOAddress`).
   - Embedded Controller (EC) ports 0x62/0x66 go through `/dev/port` (`open/lseek/read/write`, `EC_*` functions in `rdc_tool.c`), not `inb`/`outb`.
2. **PCI option ROM unreadable** (`BIOS @ 0x????????`). `pci_device_read_rom` fails; `RDCMapVBIOS` falls back to `RDCVBIOS.ROM` (path constant `BIOS_ROM_PATH_FILE`). Handle `BIOSVirtualAddr == NULL` gracefully.
3. **VRAM is configurable** (16 MB default, up to 64 MB). Mode pool is filtered to fit `AvailableFBsize` (after capture/CMDQ/cursor reservations). Capture buffer (7 MB) is reserved only when enough FB remains for a 1920x1200@32bpp framebuffer.

## Critical invariants (regressing any of these re-breaks the driver)

- **Screen callback signatures** are the modern Xorg ABI (`ScreenInit(ScreenPtr,int,char**)`, `SwitchMode(ScrnInfoPtr,DisplayModePtr)`, etc.). Do not revert to `int scrnIndex` forms.
- **`ULONG`/`DWORD` are `unsigned int` (32-bit)** (see `BiosDef.h`). They must stay 32-bit: hardware registers and CMDQ packets are 32-bit; on LP64 they must not be `unsigned long`. `uint64` stays 64-bit.
- **`xalloc`/`xfree`/`xcalloc`/`xrealloc` are compatibility macros** over malloc/free (modern Xorg removed them).
- **EXA is the only accel path.** `HAVE_XAA` is off; PreInit auto-switches `useEXA=TRUE` and loads the `exa` module, else falls back to `noAccel`. `fb`/`vbe`/`xaa`/`ramdac` module loads are non-fatal where built-in/absent.
- **Mode pool rules** (`RDCBuildModePool`, `RDCValidMode`): max resolution 1920x1200, refresh ≤ 60 Hz only, and only modes that fit `AvailableFBsize` are offered. `xorg.conf` must NOT force `Virtual` larger than the largest fitting mode.
- **`RDCScreenInit` self-heals after logout/DM screen re-init** (PreInit is not re-run on that path). At the top of `RDCScreenInit`, if `MMIOVirtualAddr`/`FBVirtualAddr`/`BIOSVirtualAddr` are NULL (torn down by `RDCCloseScreen`), it must re-map them AND restore every PreInit-set CInt10 pointer:
  - `pjIOAddress` = new `MMIOVirtualAddr`
  - `pjROMLinearAddr` = new `BIOSVirtualAddr` (also reset `ulROMType = 0` so `RDCMapVBIOS` re-reads, then re-run `CBIOSInitialDataFromVBIOS`)
  - `pVideoVirtualAddress` = new `FBVirtualAddr`
  Missing any one causes a use-after-free/SEGV on logout (seen as SIGSEGV in `vgaHWGetIOBase`, SIGFPE, or `memset` writing to a stale FB address).
- **Message verbosity levels** (defined in `rdc.h`): `ErrorLevel=0`, `DefaultLevel=4`, `InfoLevel=6`, `InternalLevel=7`. Per-operation debug prints (e.g. EXA solid/copy) must use `InternalLevel`, **not** `ErrorLevel` — `ErrorLevel` floods the log on every 2D operation.

## Architecture (src/)

| File | Role |
|---|---|
| `rdc_driver.c` | Probe, PreInit/ScreenInit, callbacks, mode set orchestration, CInt10 glue |
| `rdc_mode.c` | Mode pool build/filter, `RDCSetMode` |
| `rdc_tool.c` | MMIO/FB/VBIOS map & unmap, EC access (/dev/port) |
| `rdc_vgatool.c` | Low-level MMIO port I/O helpers (GetReg/SetReg/…), palette |
| `CInt10.c` / `HDMI.c` / `TV.c` | VBIOS emulation (ROM tables, PLL, encoders) |
| `rdc_accel.c` | EXA driver (Solid/Copy/Upload/Download) |
| `rdc_cursor.c` | Hardware cursor |
| `rdc_video.c`, `vidinit.c` | Video overlay/capture (VPOST) |
| `rdc_extension.c` | Private RDC extension |
| `rdcdual_driver.c`, `rdcdual_display.c` | Dual-display path (HAVE_DUAL only, dead on modern Xorg) |
| `BiosDef.h` | Type typedefs (ULONG/DWORD 32-bit) |
| `misc/00-rdc.conf` | Shipped xorg.conf.d snippet (`Driver "rdcm15"`) |
| `misc/RDCVBIOS.ROM` | VBIOS dump for the file fallback |

## Verification

- Build: zero errors/warnings on `make`.
- On-target (i586): `Xorg -logfile /var/log/Xorg.0.log`; expected log markers: `LoadModule: "rdcm15"`, `Matched rdcm15`, `Video Memory Size=`, mode pool entries, `[EXA] Enabled EXA acceleration.`, `CBIOS: Setting ... resolution`.
- Test logout/relogin in the DE repeatedly — this exercises the re-init self-healing path and is the main regression surface.
- `xrandr` should list modes ≤ 1920x1200 at ≤ 60 Hz only.

## Known limitations

- 2D acceleration only; no 3D/OpenGL hardware support (GLX warnings like AIGLX `swrast_dri.so` are expected).
- VBE module may be absent (`pVbe` is NULL-guarded; DDC/EDID then unavailable).
- `FireCRCMDQ` ioctl path assumes the RDC kernel CR driver (inactive without it).
