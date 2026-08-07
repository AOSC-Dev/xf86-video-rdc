# xf86-video-rdc

X.Org driver for RDC Semiconductor's GPU IPs (M2012/M2015) in Vortex86 Series.

The following SoCs passed testing:

| SoC | GPU IP |
| ----- | --- |
| Vortex86 MX+ | M2012 |
| Vortex86 DX2 | M2012 |

Resolutions supported by the current driver:

| Resolution | Note |
| ----- | --- |
| 640x480 | |
| 800x600 | |
| 1024x768 | |
| 1280x720 | |
| 1280x1024 | |
| 1366x768 | |
| 1440x900 | |
| 1600x1200 | |
| 1680x1050 | |
| 1920x1080 | Requires at least 32 MB of VRAM |
| 1920x1200 | Requires at least 32 MB of VRAM |

## Build and Install

```bash
bash autogen.sh
./configure --prefix=/usr
make
sudo make install
```

## Disclaimer

This repository uses AI-assisted tools to port and fix driver source code.

```
- Model: DeepSeek V4 Flash
- Platform: DeepSeek Platform
- Agent platform: Kilo Code
- Prompt: (See the docs/llm-session.md)
```

## Thanks

[ardje/xf86-video-rdc](https://github.com/ardje/xf86-video-rdc)

## License

GNU GENERAL PUBLIC LICENSE Version 3

The initial driver code is from RDC Semiconductor Inc.(see the `COPYING` file)