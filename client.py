import tkinter as tk
from tkinter import messagebox, Listbox, Scrollbar
import websocket
import threading
import json
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from datetime import datetime
import numpy as np

class RingBuffer:
    def __init__(self, size=1000):
        self.size = size
        self.values = [None] * self.size
        self.timestamps = [None] * self.size
        self.index = 0

    def push_front(self, value, timestamp):
        self.values[self.index] = value
        self.timestamps[self.index] = timestamp
        self.index = (self.index + 1) % self.size

    def get_data(self):
        values = self.values[self.index:] + self.values[:self.index]
        timestamps = self.timestamps[self.index:] + self.timestamps[:self.index]
        return values, timestamps

    def get_front(self, count=1):
        if count > self.size:
            count = self.size
        start_index = (self.index - count) % self.size
        if start_index < self.index:
            return list(zip(self.timestamps[start_index:self.index], self.values[start_index:self.index]))
        else:
            return list(zip(self.timestamps[start_index:], self.values[start_index:])) + \
                   list(zip(self.timestamps[:self.index], self.values[:self.index]))

class WebSocketApp:
    def __init__(self, master):
        self.master = master
        self.master.title("WebSocket Client")
        self.master.geometry("800x600")
        
        self.uri_label = tk.Label(master, text="WebSocket URI:")
        self.uri_label.pack(pady=10)
        
        uri_frame = tk.Frame(master)
        uri_frame.pack(pady=10)

        self.uri_entry = tk.Entry(uri_frame, width=70)
        self.uri_entry.pack(side=tk.LEFT)
        self.uri_entry.insert(0, "ws://localhost:8080/ws")

        self.connection_status_label = tk.Label(uri_frame, text="", font=("Arial", 12), fg="green")
        self.connection_status_label.pack(side=tk.LEFT, padx=(10, 0))

        button_frame = tk.Frame(master)
        button_frame.pack(pady=10)
        
        self.start_stop_button = tk.Button(button_frame, text="Start", command=self.toggle_connection, width=20, height=2)
        self.start_stop_button.grid(row=0, column=0, padx=10)
        
        self.show_data_button = tk.Button(button_frame, text="Letzte 100 Datenpunkte anzeigen", command=self.show_last_data, width=20, height=2)
        self.show_data_button.grid(row=0, column=1, padx=10)

        self.show_latest_data_button = tk.Button(button_frame, text="Letzter Datenpunkt anzeigen", command=self.show_latest_data, width=20, height=2)
        self.show_latest_data_button.grid(row=0, column=2, padx=10)
        
        self.latest_data_label = tk.Label(master, text="", font=("Arial", 12))
        self.latest_data_label.pack(pady=10)
        
        self.figure, self.ax = plt.subplots(figsize=(8, 5))
        self.canvas = FigureCanvasTkAgg(self.figure, master)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        self.ring_buffer_id1 = RingBuffer()
        self.ring_buffer_id2 = RingBuffer()
        self.update_counter = 0

        self.connected = False
        self.thread = None
        self.running = False

    def on_message(self, ws, message):
        try:
            data = json.loads(message)
            timestamp, value, data_id = data['timestamp'], data['value'], data['id']
            dt = datetime.fromisoformat(timestamp)
            timestamp_unix = dt.timestamp()

            if data_id == 1:
                self.ring_buffer_id1.push_front(value, timestamp_unix)
            elif data_id == 2:
                self.ring_buffer_id2.push_front(value, timestamp_unix)

            self.update_counter += 1

            if self.update_counter >= 50:
                self.update_counter = 0
                self.update_plot()
        except (json.JSONDecodeError, KeyError):
            print("Fehler beim Verarbeiten der Nachricht:", message)

    def on_error(self, ws, error):
        print("WebSocket Fehler:", error)

    def on_close(self, ws):
        print("WebSocket Verbindung geschlossen")
        self.connected = False
        self.running = False
        self.connection_status_label.config(text="")

    def on_open(self, ws):
        print("WebSocket Verbindung hergestellt")
        self.connected = True
        self.running = True
        self.connection_status_label.config(text="✔️ Verbunden")

    def run(self):
        uri = self.uri_entry.get()
        websocket.enableTrace(False)
        ws = websocket.WebSocketApp(uri,
                                    on_message=self.on_message,
                                    on_error=self.on_error,
                                    on_close=self.on_close)
        ws.on_open = self.on_open
        self.ws = ws
        ws.run_forever()

    def toggle_connection(self):
        if self.connected:
            self.connected = False
            self.start_stop_button.config(text="Start")
            if self.thread:
                self.running = False
                self.ws.close()
                self.thread.join()
            self.connection_status_label.config(text="")
        else:
            uri = self.uri_entry.get()
            if not uri:
                messagebox.showerror("Fehler", "Bitte geben Sie eine gültige WebSocket-URI ein.")
                return

            self.thread = threading.Thread(target=self.run)
            self.thread.start()
            self.start_stop_button.config(text="Stop")

    def update_plot(self):
        self.ax.clear()

        values_id1, timestamps_id1 = self.ring_buffer_id1.get_data()
        values_id2, timestamps_id2 = self.ring_buffer_id2.get_data()

        valid_indices_id1 = [i for i, val in enumerate(values_id1) if val is not None]
        valid_values_id1 = [values_id1[i] for i in valid_indices_id1]
        valid_timestamps_id1 = [timestamps_id1[i] for i in valid_indices_id1]

        valid_indices_id2 = [i for i, val in enumerate(values_id2) if val is not None]
        valid_values_id2 = [values_id2[i] for i in valid_indices_id2]
        valid_timestamps_id2 = [timestamps_id2[i] for i in valid_indices_id2]

        if valid_timestamps_id1 and valid_timestamps_id2:
            min_timestamp = max(min(valid_timestamps_id1), min(valid_timestamps_id2))
            max_timestamp = min(max(valid_timestamps_id1), max(valid_timestamps_id2))
            self.ax.set_xlim(min_timestamp, max_timestamp)

        self.ax.step(valid_timestamps_id1, valid_values_id1, label="ID 1 (Sägezahn)", where='mid', linestyle='-', color='tab:orange', alpha=0.7)
        self.ax.plot(valid_timestamps_id2, valid_values_id2, label="ID 2 (Sinus)", linestyle='-', marker='o', color='tab:blue', alpha=0.8)

        self.ax.set_title("Empfangene Werte")
        self.ax.set_xlabel("Zeitstempel")
        self.ax.set_ylabel("Wert")
        self.ax.legend()

        self.ax.xaxis.set_major_formatter(plt.FuncFormatter(self.format_unix_timestamps))
        self.canvas.draw()

    def format_unix_timestamps(self, x, pos):
        return datetime.fromtimestamp(x).strftime("%Y-%m-%d %H:%M:%S")

    def show_last_data(self):
        values_id1, timestamps_id1 = self.ring_buffer_id1.get_data()
        values_id2, timestamps_id2 = self.ring_buffer_id2.get_data()

        top = tk.Toplevel(self.master)
        top.title("Letzte 100 Datensätze")

        listbox = Listbox(top, width=60, height=20)
        listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        scrollbar = Scrollbar(top)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        listbox.config(yscrollcommand=scrollbar.set)
        scrollbar.config(command=listbox.yview)

        for i in range(len(values_id1)):
            if timestamps_id1[i] is not None and values_id1[i] is not None:
                listbox.insert(tk.END, f"ID 1: {datetime.fromtimestamp(timestamps_id1[i])} - {values_id1[i]}")
            if timestamps_id2[i] is not None and values_id2[i] is not None:
                listbox.insert(tk.END, f"ID 2: {datetime.fromtimestamp(timestamps_id2[i])} - {values_id2[i]}")

    def show_latest_data(self):
        latest_id1 = self.ring_buffer_id1.get_front(1)
        latest_id2 = self.ring_buffer_id2.get_front(1)

        latest_text = "Letzter Wert:\n"
        if latest_id1 and latest_id1[0][0] is not None:
            latest_text += f"ID 1: {datetime.fromtimestamp(latest_id1[0][0])} - {latest_id1[0][1]}\n"
        if latest_id2 and latest_id2[0][0] is not None:
            latest_text += f"ID 2: {datetime.fromtimestamp(latest_id2[0][0])} - {latest_id2[0][1]}"

        self.latest_data_label.config(text=latest_text)

root = tk.Tk()
app = WebSocketApp(root)
root.mainloop()

