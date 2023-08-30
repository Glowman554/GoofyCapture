import sys
import serial
from time import time
from json import loads
from datetime import datetime
from os import system

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} <serial device> <capture info file> <output?>")
    raise Exception("Invalid arguments")

if len(sys.argv) < 4:
    output = "/tmp/capture.vcd"
else:
    output = sys.argv[3]

class Tunnel:
    def __init__(self, port):
        self.port = serial.Serial(port, 115200)
        print("Connecting...")
        # self.port.read_until(b'READY\n')
        self.command("PING")

    def _recv(self):
        tmp = self.port.readline()
        # print("<-", tmp)
        return tmp

    def _send(self, b):
        print("->", b)
        self.port.write(b)

    def command(self, command):
        self._send(str.encode(command + "\n"))
        tmp = self._recv().decode()
        if tmp == "":
            raise Exception("Command failed")
        return tmp

tunnel = Tunnel(sys.argv[1])

with open(sys.argv[2], "r") as f:
    capture_info = loads(f.read())

    sample_pre_s = capture_info["sample_pre_s"]
    sample_count = capture_info["sample_count"]
    trigger_pin = capture_info["trigger_pin"]
    trigger_polarity = capture_info["trigger_polarity"]

if (sample_count * 4) / 1024 > 200:
    print(f"Samples buffer needs to be {(sample_count * 4) / 1024} kb big! That might be too much :o")
    if input("Continue (y/n)? ") == "n":
        raise Exception("Capture failed") 

tunnel.command(f"PREPARE {int(sample_pre_s)} {int(sample_count)}")

start = time()
res = tunnel.command(f"RUN {trigger_pin} {int(trigger_polarity)}")
end = time()

if not res.startswith("OK"):
    raise Exception(f"Capture failed '{res.strip()}'")

print(f"Capture took {round(end - start, 4)}S")

start = time()
with open(output, "w") as f:
    timestr = datetime.now().strftime("%m/%d/%Y, %H:%M:%S")
    f.write(f"$date {timestr} $end\n")
    f.write("$version Goofy Capture $end\n")
    f.write("$timescale 1 us $end\n")
    f.write("$scope module top $end\n")
    for i in range(0, 32):
        f.write(f"\t$var wire 1 p{i} pin{i} $end\n")
    f.write("$upscope $end\n")
    f.write("$enddefinitions $end\n")

    res = tunnel.command("DUMP")

    progress = 0
    percent = 0
    while True:
        tmp = tunnel._recv().decode().strip()
        progress += 1

        new_percent = round((progress / sample_count) * 100)
        if percent != new_percent:
            percent = new_percent
            print(f"{percent}%", end="\r")

        if tmp.startswith("END"):
            break

        f.write(f"#{(progress / (sample_pre_s / (1000 * 1000)))}\n")

        # print(tmp)

        for j in range(0, 32):
            if int(tmp) & (1 << j):
                f.write(f"1p{j} ")
            else:
                f.write(f"0p{j} ")
        f.write(f"\n")

end = time()

print(f"Dump took {round(end - start, 4)}S")

system(f"gtkwave {output}")
