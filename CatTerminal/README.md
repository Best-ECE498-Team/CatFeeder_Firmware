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
