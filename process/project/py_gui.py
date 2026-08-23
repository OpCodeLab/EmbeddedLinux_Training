#!/usr/bin/env python3
"""
gui.py -- Educational Linux IPC GUI (PyQt5)
============================================
Talks to the companion C program (ipc_server.c) over four different
Linux IPC mechanisms so students can watch them side by side:

  1. A signal              -- SIGUSR1, sent via os.kill() to a PID you enter
  2. A POSIX message queue -- /edu_ipc_mq
  3. A Unix domain socket  -- /tmp/edu_ipc.sock
  4. POSIX shared memory   -- /edu_ipc_shm, guarded by semaphore /edu_ipc_sem

Every message that arrives on the queue, the socket, or shared memory
increments that channel's counter, shows the latest payload, and computes
a latency (in ms) from the CLOCK_MONOTONIC timestamp embedded by the C
program, since both processes share the same monotonic clock domain on
one machine, that difference is a meaningful, comparable metric across
the three channels.

Install dependencies:
    pip install PyQt5 posix_ipc --break-system-packages

python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt    

Run:
    1) In one terminal:  make && ./ipc_server
    2) In another:       python3 gui.py
    3) Copy the PID printed by ipc_server into "Target PID" and click
       "Send SIGUSR1" -- watch sig_count climb in every channel's messages.

Note (WSL only): if ipc_server reports mq_open failing with "Function not
implemented", POSIX message queues aren't mounted yet:
    sudo mkdir -p /dev/mqueue && sudo mount -t mqueue none /dev/mqueue
"""

import os
import sys
import time
import struct
import socket
import signal
import mmap

import posix_ipc
from PyQt5 import QtCore, QtWidgets

MQ_NAME   = "/edu_ipc_mq"
SHM_NAME  = "/edu_ipc_shm"
SEM_NAME  = "/edu_ipc_sem"
SOCK_PATH = "/tmp/edu_ipc.sock"

# Must match the C struct byte-for-byte (see ipc_server.c, #pragma pack(1)):
#   uint32_t seq; double timestamp; int32_t sig_count; char text[64];
MSG_STRUCT = struct.Struct("<Idi64s")
MSG_SIZE = MSG_STRUCT.size  # 80 bytes


def unpack_msg(raw):
    seq, ts, sig_count, text = MSG_STRUCT.unpack(raw)
    text = text.split(b"\x00", 1)[0].decode("utf-8", errors="replace")
    return seq, ts, sig_count, text


def now_monotonic():
    # Same clock domain as the C side's clock_gettime(CLOCK_MONOTONIC, ...),
    # so subtracting the two is a valid cross-process latency measurement
    # as long as both processes are on the same machine.
    return time.clock_gettime(time.CLOCK_MONOTONIC)


class MQueueWorker(QtCore.QThread):
    message = QtCore.pyqtSignal(int, float, int, str, float)
    status = QtCore.pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self._running = True

    def stop(self):
        self._running = False
        self.wait(1500)

    def run(self):
        mq = None
        while self._running and mq is None:
            try:
                mq = posix_ipc.MessageQueue(MQ_NAME)
                self.status.emit("connected")
            except posix_ipc.ExistentialError:
                self.status.emit("waiting for ipc_server...")
                time.sleep(0.5)

        while self._running and mq is not None:
            try:
                raw, _priority = mq.receive(timeout=1)
            except posix_ipc.BusyError:
                continue  # just a receive timeout, loop and check _running
            except Exception as exc:
                self.status.emit(f"error: {exc}")
                break
            seq, ts, sig_count, text = unpack_msg(raw)
            latency_ms = (now_monotonic() - ts) * 1000.0
            self.message.emit(seq, ts, sig_count, text, latency_ms)

        if mq is not None:
            mq.close()


class SocketWorker(QtCore.QThread):
    message = QtCore.pyqtSignal(int, float, int, str, float)
    status = QtCore.pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self._running = True

    def stop(self):
        self._running = False
        self.wait(1500)

    @staticmethod
    def _recv_exact(sock, n):
        buf = b""
        while len(buf) < n:
            chunk = sock.recv(n - len(buf))
            if not chunk:
                return None  # peer closed
            buf += chunk
        return buf

    def run(self):
        while self._running:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.settimeout(1.0)
            try:
                sock.connect(SOCK_PATH)
                self.status.emit("connected")
            except (FileNotFoundError, ConnectionRefusedError):
                self.status.emit("waiting for ipc_server...")
                sock.close()
                time.sleep(0.5)
                continue

            while self._running:
                try:
                    raw = self._recv_exact(sock, MSG_SIZE)
                except socket.timeout:
                    continue
                except OSError as exc:
                    self.status.emit(f"disconnected: {exc}")
                    break
                if raw is None:
                    self.status.emit("server closed connection")
                    break
                seq, ts, sig_count, text = unpack_msg(raw)
                latency_ms = (now_monotonic() - ts) * 1000.0
                self.message.emit(seq, ts, sig_count, text, latency_ms)

            sock.close()
            time.sleep(0.5)


class ShmWorker(QtCore.QThread):
    message = QtCore.pyqtSignal(int, float, int, str, float)
    status = QtCore.pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self._running = True

    def stop(self):
        self._running = False
        self.wait(1500)

    def run(self):
        shm = None
        sem = None
        while self._running and shm is None:
            try:
                shm = posix_ipc.SharedMemory(SHM_NAME)
                sem = posix_ipc.Semaphore(SEM_NAME)
                self.status.emit("connected")
            except posix_ipc.ExistentialError:
                self.status.emit("waiting for ipc_server...")
                time.sleep(0.5)

        if shm is None:
            return

        mapfile = mmap.mmap(shm.fd, MSG_SIZE)
        shm.close_fd()  # the mapping keeps the memory alive; fd no longer needed

        # Shared memory has no built-in notification (unlike the queue or the
        # socket), so we must poll it ourselves, and use the semaphore the
        # writer also uses to avoid reading a half-written (torn) struct.
        last_seq = -1
        while self._running:
            sem.acquire()
            raw = mapfile[:MSG_SIZE]
            sem.release()

            seq, ts, sig_count, text = unpack_msg(raw)
            if seq != last_seq:
                last_seq = seq
                latency_ms = (now_monotonic() - ts) * 1000.0
                self.message.emit(seq, ts, sig_count, text, latency_ms)
            time.sleep(0.1)

        mapfile.close()


class ChannelPanel(QtWidgets.QGroupBox):
    """One IPC channel's live view: status, counter, latest message, latency, log."""

    def __init__(self, title):
        super().__init__(title)
        self.count = 0

        self.status_label = QtWidgets.QLabel("not connected")
        self.count_label = QtWidgets.QLabel("Messages received: 0")
        self.latest_label = QtWidgets.QLabel("-")
        self.latest_label.setWordWrap(True)
        self.latency_label = QtWidgets.QLabel("Latency: - ms")
        self.log = QtWidgets.QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(200)

        layout = QtWidgets.QVBoxLayout()
        layout.addWidget(self.status_label)
        layout.addWidget(self.count_label)
        layout.addWidget(self.latest_label)
        layout.addWidget(self.latency_label)
        layout.addWidget(self.log)
        self.setLayout(layout)

    def on_status(self, text):
        self.status_label.setText(text)

    def on_message(self, seq, ts, sig_count, text, latency_ms):
        del ts  # not shown directly; latency_ms is the derived, useful value
        self.count += 1
        self.count_label.setText(f"Messages received: {self.count}")
        self.latest_label.setText(f'seq={seq}  sig_count={sig_count}  text="{text}"')
        self.latency_label.setText(f"Latency: {latency_ms:.2f} ms")
        self.log.appendPlainText(f"#{seq}  ({latency_ms:6.2f} ms)  {text}")


class MainWindow(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Linux IPC Demo: Python (PyQt5) <-> C")
        self.resize(1000, 620)

        # --- signal section ---
        self.pid_edit = QtWidgets.QLineEdit()
        self.pid_edit.setPlaceholderText("PID of ipc_server")
        self.send_button = QtWidgets.QPushButton("Send SIGUSR1")
        self.send_button.clicked.connect(self.send_signal)
        self.signal_status = QtWidgets.QLabel("")

        signal_row = QtWidgets.QHBoxLayout()
        signal_row.addWidget(QtWidgets.QLabel("Target PID:"))
        signal_row.addWidget(self.pid_edit)
        signal_row.addWidget(self.send_button)
        signal_row.addWidget(self.signal_status, stretch=1)

        # --- channel panels ---
        self.mq_panel = ChannelPanel("POSIX Message Queue (/edu_ipc_mq)")
        self.sock_panel = ChannelPanel("Unix Domain Socket (/tmp/edu_ipc.sock)")
        self.shm_panel = ChannelPanel("Shared Memory (/edu_ipc_shm)")

        panels_row = QtWidgets.QHBoxLayout()
        panels_row.addWidget(self.mq_panel)
        panels_row.addWidget(self.sock_panel)
        panels_row.addWidget(self.shm_panel)

        main_layout = QtWidgets.QVBoxLayout()
        main_layout.addLayout(signal_row)
        main_layout.addLayout(panels_row)
        self.setLayout(main_layout)

        # --- background workers, one per IPC channel ---
        self.mq_worker = MQueueWorker()
        self.mq_worker.message.connect(self.mq_panel.on_message)
        self.mq_worker.status.connect(self.mq_panel.on_status)

        self.sock_worker = SocketWorker()
        self.sock_worker.message.connect(self.sock_panel.on_message)
        self.sock_worker.status.connect(self.sock_panel.on_status)

        self.shm_worker = ShmWorker()
        self.shm_worker.message.connect(self.shm_panel.on_message)
        self.shm_worker.status.connect(self.shm_panel.on_status)

        self.mq_worker.start()
        self.sock_worker.start()
        self.shm_worker.start()

    def send_signal(self):
        text = self.pid_edit.text().strip()
        try:
            pid = int(text)
        except ValueError:
            self.signal_status.setText("Enter a valid integer PID")
            return
        try:
            os.kill(pid, signal.SIGUSR1)
        except OSError as exc:
            self.signal_status.setText(f"Error: {exc}")
        else:
            self.signal_status.setText(f"Sent SIGUSR1 to PID {pid}")

    def closeEvent(self, event):
        self.mq_worker.stop()
        self.sock_worker.stop()
        self.shm_worker.stop()
        event.accept()


def main():
    app = QtWidgets.QApplication(sys.argv)
    win = MainWindow()
    win.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()