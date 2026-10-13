#!/usr/bin/env python3
"""ZeroMQ XSUB/XPUB broker. Publishers connect to XSUB, subscribers connect to XPUB.
If your AUV middleware already has a broker, skip this and point agent.py at it."""
import argparse
import zmq

p = argparse.ArgumentParser()
p.add_argument("--bind", default="127.0.0.1", help="use 0.0.0.0 to allow other PCs")
p.add_argument("--xsub-port", type=int, default=5555)
p.add_argument("--xpub-port", type=int, default=5556)
a = p.parse_args()

ctx = zmq.Context()
xsub = ctx.socket(zmq.XSUB)
xsub.bind(f"tcp://{a.bind}:{a.xsub_port}")
xpub = ctx.socket(zmq.XPUB)
xpub.bind(f"tcp://{a.bind}:{a.xpub_port}")
print(f"broker: publishers -> tcp://{a.bind}:{a.xsub_port} | subscribers -> tcp://{a.bind}:{a.xpub_port}")
zmq.proxy(xsub, xpub)
