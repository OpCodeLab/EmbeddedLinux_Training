#!/usr/bin/env python3
"""Minimal test client for the training telemetry server."""
import socket

def main():
    with socket.create_connection(("127.0.0.1", 6000)) as s:
        print("connected, waiting for telemetry...")
        buf = b""
        while True:
            data = s.recv(4096)
            if not data:
                break
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                print("RECV:", line.decode())

if __name__ == "__main__":
    main()
