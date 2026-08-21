# GameTCP3

A Windows 10/11 network optimization toolkit for online gaming and streaming. Applies targeted TCP/IP registry tweaks and network stack configurations to reduce latency and improve performance.

## Overview

**GameTCP3** is the unified successor to GameTCP and GameTCP2, combining all optimizations into a single tool with 80+ registry tweaks, recursive interface configuration, and network stack reset.

## Requirements

- Windows 10 or Windows 11
- Administrator privileges (required for registry modifications)
- .NET Framework (included with Windows)
- Visual Studio .NET 2019+ or VB.NET compiler (`vbc`) for building from source

## Building

Run `Create.bat` to compile and embed the admin manifest:

```
Create.bat
```

## Usage

- **GameSet.bat** - Launches GameTCP3 with administrator privileges (UAC elevation)
- **teredo.bat** - Interactive Teredo tunneling configuration menu

## What It Modifies

- Game task scheduling (affinity, clock rate, GPU/CPU priority)
- Multimedia system profile (network throttling, system responsiveness)
- TCP/IP parameters (Nagle's algorithm, TTL, window size, port range)
- Per-interface TCP/IP settings (ACK frequency, window size, MTU, recursive)
- DNS cache optimization
- LanmanServer and LanmanWorkstation parameters
- NetBT interface configuration
- Internet connection settings (HKCU and HKLM)
- Policy and miscellaneous system settings
- Memory management
- MSMQ settings
- Network stack reset via netsh commands

## Important Notes

- A system reboot is required after applying changes
- Some settings may not be suitable for all network configurations
- This is experimental software - use at your own discretion
- Always test with your specific network setup before relying on these changes

## License

BSD 3-Clause License. See [LICENSE.md](LICENSE.md) for details.

## Links

- [GitHub Repository](https://github.com/Zanphar/GameTCP/)
