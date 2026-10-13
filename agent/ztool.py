#!/usr/bin/env python3
"""Test tool.   python ztool.py sub robot/out/        (watch what the robot sends)
                python ztool.py pub robot/in/emotion happy
                python ztool.py pub robot/in/servo/cmd 0,45,60"""
import sys, time, zmq

XSUB, XPUB = "tcp://127.0.0.1:5555", "tcp://127.0.0.1:5556"
ctx = zmq.Context()
if len(sys.argv) >= 3 and sys.argv[1] == "sub":
    s = ctx.socket(zmq.SUB); s.connect(XPUB); s.setsockopt(zmq.SUBSCRIBE, sys.argv[2].encode())
    while True:
        t, p = s.recv_multipart()
        print(t.decode(errors="replace"), p.decode(errors="replace"))
elif len(sys.argv) >= 4 and sys.argv[1] == "pub":
    s = ctx.socket(zmq.PUB); s.connect(XSUB); time.sleep(0.3)
    s.send_multipart([sys.argv[2].encode(), " ".join(sys.argv[3:]).encode()]); time.sleep(0.2)
else:
    print(__doc__)
