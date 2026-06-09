# STM32 Debug Console Starter

This is a small Python/Tkinter UI based on your sketch:

- Top bar: COM port selector, baud rate, Start/Stop, Clear, Print/autoscroll, timestamp toggle
- Middle: large debug print/log window
- Bottom: command input for sending text to the STM32 UART

## Setup

Install Python 3.11+ from <https://www.python.org/downloads/> if `python --version` does not work in PowerShell.

Then run:

```powershell
cd <your directory containing CatTerminal.py>
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python .\CatTerminal.py
```

If `pyserial` is not installed yet, the app still opens in `DEMO` mode so you can see the layout.

## Build a Windows EXE

To make a one-file Windows app you can send to other people, double-click:

```text
build_exe.bat
```

Or run:

```powershell
.\build_exe.ps1
```

The finished app will be:

```text
dist\CatTerminal.exe
```

Other users do not need to install Python to run that `.exe`. They may still need the correct USB serial/ST-Link driver for the STM32 board.