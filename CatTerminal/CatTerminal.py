import json
import os
import queue
import threading
import time
import tkinter as tk
from datetime import datetime
from pathlib import Path
from tkinter import messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


BAUD_RATES = ("9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600")
MAX_COMMAND_HISTORY = 200
MAX_LOG_LINES = 5000
MAX_LOG_BATCH_LINES = 250
MAX_PENDING_LINES = 10000


class SerialWorker:
    def __init__(self, port, baud_rate, line_queue):
        self.port = port
        self.baud_rate = baud_rate
        self.line_queue = line_queue
        self._stop_event = threading.Event()
        self._thread = None
        self._serial = None

    @property
    def is_running(self):
        return self._thread is not None and self._thread.is_alive()

    def start(self):
        if self.is_running:
            return
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._read_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop_event.set()
        if self._serial:
            try:
                self._serial.close()
            except Exception:
                pass

    def send(self, text):
        if not self._serial or not self._serial.is_open:
            raise RuntimeError("Serial port is not open.")
        self._serial.write(text.encode("utf-8"))

    def _put_line(self, tag, text):
        try:
            self.line_queue.put_nowait((tag, text))
        except queue.Full:
            try:
                self.line_queue.get_nowait()
            except queue.Empty:
                pass
            try:
                self.line_queue.put_nowait((tag, text))
            except queue.Full:
                pass

    def _read_loop(self):
        if serial is None:
            self._demo_loop()
            return

        try:
            self._serial = serial.Serial(self.port, self.baud_rate, timeout=0.1)
            self._put_line("status", f"Connected to {self.port} at {self.baud_rate} baud")
            while not self._stop_event.is_set():
                raw = self._serial.readline()
                if raw:
                    text = raw.decode("utf-8", errors="replace").rstrip()
                    self._put_line("rx", text)
        except Exception as exc:
            self._put_line("error", str(exc))
        finally:
            if self._serial:
                try:
                    self._serial.close()
                except Exception:
                    pass
            self._put_line("status", "Disconnected")

    def _demo_loop(self):
        self._put_line("status", "pyserial is not installed; running in demo mode")
        count = 0
        while not self._stop_event.is_set():
            count += 1
            self._put_line("rx", f"[demo] STM32 heartbeat {count}: adc=2048 temp=31.2C")
            time.sleep(1.0)
        self._put_line("status", "Demo stopped")


class DebugApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("CatTerminal")
        self.geometry("1100x720")
        self.minsize(820, 520)

        self.line_queue = queue.Queue(maxsize=MAX_PENDING_LINES)
        self.worker = None
        self.log_line_count = 0
        self.history_file = self._get_history_file()
        self.command_history = self._load_command_history()
        self.history_index = None
        self.history_draft = ""

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value="460800")
        self.command_var = tk.StringVar()
        self.line_ending_var = tk.StringVar(value="\\n")
        self.autoscroll_var = tk.BooleanVar(value=True)
        self.timestamp_var = tk.BooleanVar(value=True)

        self._build_ui()
        self.refresh_ports()
        self.after(50, self._drain_queue)

    def _build_ui(self):
        self.columnconfigure(0, weight=1)
        self.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(self, padding=(10, 10, 10, 6))
        toolbar.grid(row=0, column=0, sticky="ew")
        toolbar.columnconfigure(1, weight=1)

        ttk.Label(toolbar, text="COM").grid(row=0, column=0, padx=(0, 6))
        self.port_combo = ttk.Combobox(toolbar, textvariable=self.port_var, width=24, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="w")

        ttk.Button(toolbar, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=(8, 0))

        ttk.Label(toolbar, text="Baud").grid(row=0, column=3, padx=(18, 6))
        self.baud_combo = ttk.Combobox(toolbar, textvariable=self.baud_var, values=BAUD_RATES, width=10)
        self.baud_combo.grid(row=0, column=4)
        ttk.Label(toolbar, text="Bits/s").grid(row=0, column=5, padx=(6, 0))

        self.start_button = ttk.Button(toolbar, text="Start", command=self.toggle_serial)
        self.start_button.grid(row=0, column=6, padx=(18, 0))

        ttk.Button(toolbar, text="Clear", command=self.clear_log).grid(row=0, column=7, padx=(8, 0))
        ttk.Checkbutton(toolbar, text="Print", variable=self.autoscroll_var).grid(row=0, column=8, padx=(18, 0))
        ttk.Checkbutton(toolbar, text="Time", variable=self.timestamp_var).grid(row=0, column=9, padx=(8, 0))

        log_frame = ttk.Frame(self, padding=(10, 6))
        log_frame.grid(row=1, column=0, sticky="nsew")
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)

        self.log = tk.Listbox(
            log_frame,
            font=("Consolas", 11),
            bg="#0f1419",
            fg="#dbe7ef",
            selectbackground="#26323d",
            selectforeground="#ffffff",
            activestyle="none",
            relief="flat",
            borderwidth=0,
            highlightthickness=0,
        )
        self.log.grid(row=0, column=0, sticky="nsew")

        scrollbar = ttk.Scrollbar(log_frame, command=self.log.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.log.configure(yscrollcommand=scrollbar.set)

        self.log_colors = {
            "rx": "#dbe7ef",
            "rx_error": "#ff4d4f",
            "rx_warn": "#f6a13a",
            "rx_debug": "#2ecc71",
            "tx": "#89ddff",
            "status": "#c3e88d",
            "error": "#ff7a90",
        }

        command_bar = ttk.Frame(self, padding=(10, 6, 10, 10))
        command_bar.grid(row=2, column=0, sticky="ew")
        command_bar.columnconfigure(1, weight=1)

        ttk.Label(command_bar, text="CMD").grid(row=0, column=0, padx=(0, 8))
        command_entry = ttk.Entry(command_bar, textvariable=self.command_var)
        command_entry.grid(row=0, column=1, sticky="ew")
        command_entry.bind("<Return>", lambda _event: self.send_command())
        command_entry.bind("<Up>", self.show_previous_command)
        command_entry.bind("<Down>", self.show_next_command)

        ttk.Label(command_bar, text="Ending").grid(row=0, column=2, padx=(12, 6))
        ttk.Combobox(
            command_bar,
            textvariable=self.line_ending_var,
            values=("\\n", "\\r", "\\r\\n", "none"),
            width=7,
            state="readonly",
        ).grid(row=0, column=3)

        ttk.Button(command_bar, text="Send", command=self.send_command).grid(row=0, column=4, padx=(10, 0))

        self._append_log("status", "Ready. Select a COM port, then press Start.")

    def refresh_ports(self):
        ports = []
        if list_ports is not None:
            ports = [port.device for port in list_ports.comports()]

        if not ports:
            ports = ["DEMO"]

        self.port_combo.configure(values=ports)
        if self.port_var.get() not in ports:
            self.port_var.set(ports[0])

    def toggle_serial(self):
        if self.worker and self.worker.is_running:
            self.worker.stop()
            self.start_button.configure(text="Start")
            return

        port = self.port_var.get()
        if not port:
            messagebox.showwarning("No COM Port", "Choose a COM port first.")
            return

        try:
            baud = int(self.baud_var.get())
        except ValueError:
            messagebox.showwarning("Bad Baud Rate", "Enter a numeric baud rate.")
            return

        self.worker = SerialWorker(port, baud, self.line_queue)
        self.worker.start()
        self.start_button.configure(text="Stop")

    def send_command(self):
        command = self.command_var.get()
        if not command:
            return

        self._remember_command(command)

        ending = self.line_ending_var.get()
        suffix = "" if ending == "none" else ending.encode("utf-8").decode("unicode_escape")
        payload = command + suffix

        try:
            if self.worker and self.worker.is_running and self.worker.port != "DEMO":
                self.worker.send(payload)
            self._append_log("tx", f"> {command}")
            self.command_var.set("")
        except Exception as exc:
            self._append_log("error", f"Send failed: {exc}")

    def show_previous_command(self, _event):
        if not self.command_history:
            return "break"

        if self.history_index is None:
            self.history_draft = self.command_var.get()
            self.history_index = len(self.command_history) - 1
        elif self.history_index > 0:
            self.history_index -= 1

        self.command_var.set(self.command_history[self.history_index])
        return "break"

    def show_next_command(self, _event):
        if self.history_index is None:
            return "break"

        if self.history_index < len(self.command_history) - 1:
            self.history_index += 1
            self.command_var.set(self.command_history[self.history_index])
        else:
            self.history_index = None
            self.command_var.set(self.history_draft)
            self.history_draft = ""

        return "break"

    def _remember_command(self, command):
        if not self.command_history or self.command_history[-1] != command:
            self.command_history.append(command)
            self.command_history = self.command_history[-MAX_COMMAND_HISTORY:]
            self._save_command_history()

        self.history_index = None
        self.history_draft = ""

    def _get_history_file(self):
        base_dir = os.environ.get("LOCALAPPDATA")
        if base_dir:
            app_dir = Path(base_dir) / "CatTerminal"
        else:
            app_dir = Path.home() / ".catterminal"
        return app_dir / "command_history.json"

    def _load_command_history(self):
        try:
            with self.history_file.open("r", encoding="utf-8") as history:
                commands = json.load(history)
        except (OSError, json.JSONDecodeError):
            return []

        if not isinstance(commands, list):
            return []

        return [command for command in commands if isinstance(command, str)][-MAX_COMMAND_HISTORY:]

    def _save_command_history(self):
        try:
            self.history_file.parent.mkdir(parents=True, exist_ok=True)
            with self.history_file.open("w", encoding="utf-8") as history:
                json.dump(self.command_history, history, indent=2)
        except OSError as exc:
            self._append_log("error", f"Could not save command history: {exc}")

    def clear_log(self):
        self.log.delete(0, "end")
        self.log_line_count = 0

    def _drain_queue(self):
        lines = []
        try:
            while len(lines) < MAX_LOG_BATCH_LINES:
                tag, text = self.line_queue.get_nowait()
                lines.append((self._tag_for_line(tag, text), text))
                if tag == "status" and text in {"Disconnected", "Demo stopped"}:
                    self.start_button.configure(text="Start")
        except queue.Empty:
            pass

        if lines:
            self._append_log_batch(lines)

        if len(lines) == MAX_LOG_BATCH_LINES:
            self.after(1, self._drain_queue)
        else:
            self.after(25, self._drain_queue)

    def _tag_for_line(self, tag, text):
        if tag != "rx":
            return tag

        if text.startswith("[ERROR]") or text.startswith("[ERROR "):
            return "rx_error"
        if text.startswith("[WARN]") or text.startswith("[WARN "):
            return "rx_warn"
        if text.startswith("[DEBUG]") or text.startswith("[DEBUG "):
            return "rx_debug"
        return "rx"

    def _append_log(self, tag, text):
        self._append_log_batch([(tag, text)])

    def _append_log_batch(self, lines):
        if self.timestamp_var.get():
            timestamp = datetime.now().strftime("%H:%M:%S ")
        else:
            timestamp = ""

        should_follow = self.autoscroll_var.get() and self._is_log_at_bottom()

        for tag, text in lines:
            prefix = timestamp if timestamp else ""
            self.log.insert("end", f"{prefix}{text}")
            self.log.itemconfig("end", foreground=self.log_colors.get(tag, self.log_colors["rx"]))

        self.log_line_count += len(lines)
        excess_lines = self.log_line_count - MAX_LOG_LINES
        if excess_lines > 0:
            self.log.delete(0, excess_lines - 1)
            self.log_line_count -= excess_lines

        if should_follow:
            self.log.see("end")

    def _is_log_at_bottom(self):
        _first, last = self.log.yview()
        return last >= 0.995


if __name__ == "__main__":
    app = DebugApp()
    app.mainloop()
