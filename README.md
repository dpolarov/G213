# G213Color

Tiny native Windows utility for Logitech G213 Prodigy (USB VID:PID `046D:C336`).

It sets the keyboard to a static `#3E3100` color, registers itself for the current user's logon, and exits immediately. No background process, G HUB, OpenRGB, Python, or .NET runtime is required.

The HID interface selection follows OpenRGB's G213 detector: interface usage page `0xFF43`, usage `0x0602`. The lighting packet is based on the documented/reverse-engineered G213 protocol used by OpenRGB and G213Colors.

## Build

GitHub Actions builds a single x64 `G213Color.exe` with MSVC. The executable uses only Windows system APIs.

## Usage

Run `G213Color.exe` once. It sets the color, copies itself into `%LOCALAPPDATA%\G213Color\G213Color.exe`, and registers that installed copy under the current user's Windows startup. On later logons it waits briefly for USB hubs/KVMs to enumerate the keyboard, sets the color, and exits.

Close OpenRGB or Logitech software during the first test if either application is actively controlling the G213 lighting interface.

## License

MIT. Logitech is a trademark of Logitech; this project is not affiliated with Logitech.