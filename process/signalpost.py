#!/usr/bin/env python3
import os
import signal
import sys
def main():
    try:
        pid = int(input("Enter the target process ID: "))
    except ValueError:
        print("Error: please enter a valid integer PID", file=sys.stderr)
        return

    try:
        os.kill(pid, signal.SIGUSR1)
    except OSError as e:
        print(f"Error sending signal: {e}", file=sys.stderr) 
    else:
        print(f"Sent SIGUSR1 to process {pid}")

if __name__ == "__main__":
    main()