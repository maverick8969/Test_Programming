import serial, time, re
from datetime import datetime

PORT = "/dev/cu.usbserial-FTE910QB"
BAUD = 9600

CMD = b"@P<CR><LF>"      # literal text, as requested

REPEATS_PER_BURST = 13
LINE_DELAY_MS     = 9
CHAR_DELAY_MS     = 7
READ_WINDOW_MS    = 160

RE_WEIGHT = re.compile(r'([+-]?\d+(?:\.\d+)?)\s*([a-zA-Z%]+)?')
ONLY_PRINT_ON_CHANGE = True

def send_cmd(ser, cmd, char_delay_s, line_delay_s):
    # send one command with per-char delay
    for b in cmd:
        ser.write(bytes([b]))
        time.sleep(char_delay_s)
    # gap between commands
    time.sleep(line_delay_s)

def read_window(ser, window_s):
    end = time.time() + window_s
    readings = []
    while time.time() < end:
        line = ser.readline()
        if not line:
            time.sleep(0.002)
            continue
        raw = line.decode(errors="ignore").strip()
        if not raw:
            continue
        m = RE_WEIGHT.search(raw)
        if m:
            w, u = m.groups()
            readings.append((w, u or "", raw))
    return readings

def main():
    char_delay_s = CHAR_DELAY_MS / 1000.0
    line_delay_s = LINE_DELAY_MS / 1000.0
    window_s     = READ_WINDOW_MS / 1000.0

    last_weight = None

    with serial.Serial(PORT, BAUD, timeout=0.02) as ser:
        print("Press Ctrl+C to stop.\n")
        try:
            while True:
                # 1) burst of commands
                for _ in range(REPEATS_PER_BURST):
                    send_cmd(ser, CMD, char_delay_s, line_delay_s)

                # 2) read replies
                readings = read_window(ser, window_s)

                # 3) print last reading if changed
                if readings:
                    w, u, raw = readings[-1]
                    if not ONLY_PRINT_ON_CHANGE or w != last_weight:
                        ts = datetime.now().isoformat(timespec="seconds")
                        print(f"{ts}  {w} {u}   (raw: {raw})")
                        last_weight = w
        except KeyboardInterrupt:
            print("\nStopped.")

if __name__ == "__main__":
    main()