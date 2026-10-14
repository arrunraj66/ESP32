#!/usr/bin/env python3
"""Phone/web remote for the robot (like a Blynk page). Open  http://SERVER_IP:8080  on your phone.
Publishes commands to the ZeroMQ broker as robot/in/<topic>; the agent forwards them to the ESP32.
  sudo apt install python3-zmq
  MICROZMQ_WEB_PASS=yourpassword python3 web_control.py
"""
import argparse, hmac, json, os, re, threading, time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import zmq

# topic -> regex the payload must match (keeps the web page from sending arbitrary data)
ALLOWED = {
    "servo/cmd": re.compile(r"^[0-9],-?[0-9]{1,3}(\.[0-9]+)?(,[0-9]{1,3})?$"),
    "servo/center": re.compile(r"^$"),
    "emotion": re.compile(r"^(neutral|happy|sad|angry)$"),
    "face/look": re.compile(r"^-?[01](\.[0-9]+)?,-?[01](\.[0-9]+)?$"),
    "audio/beep": re.compile(r"^[0-9]{2,4},[0-9]{1,4}$"),
}

PAGE = r"""<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot Remote</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:16px;max-width:480px;margin:auto}
h1{font-size:20px} .s{margin:18px 0} .s label{display:flex;justify-content:space-between}
input[type=range]{width:100%;height:36px} button{font-size:16px;padding:12px 16px;margin:4px;border-radius:8px;border:0;background:#2b6cb0;color:#fff}
#st{font-size:13px;color:#9ae6b4;min-height:18px}
</style></head><body>
<h1>Robot Remote</h1><div id="st"></div>
<div id="sliders"></div>
<button onclick="send('servo/center','')">Center all</button>
<button onclick="send('audio/beep','880,150')">Beep</button>
<div>
<button onclick="send('emotion','happy')">Happy</button>
<button onclick="send('emotion','sad')">Sad</button>
<button onclick="send('emotion','angry')">Angry</button>
<button onclick="send('emotion','neutral')">Neutral</button>
</div>
<button onclick="localStorage.removeItem('pw');location.reload()" style="background:#444">Change password</button>
<script>
const SERVOS=[["Head pan",0,20,160],["Head tilt",1,60,120],["Left arm",2,10,170],["Right arm",3,10,170]];
let pw=localStorage.getItem('pw')||prompt('Password'); if(pw)localStorage.setItem('pw',pw);
const st=document.getElementById('st');
async function send(topic,payload){
  try{
    const r=await fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/json','X-Pass':pw},body:JSON.stringify({topic,payload})});
    st.textContent=r.ok?'sent: '+topic+' '+payload:(r.status==401?'wrong password':'error '+r.status);
  }catch(e){st.textContent='no connection';}
}
const box=document.getElementById('sliders');
SERVOS.forEach(([name,id,lo,hi])=>{
  const d=document.createElement('div');d.className='s';
  d.innerHTML=`<label><span>${name}</span><span id="v${id}">90</span></label><input type="range" min="${lo}" max="${hi}" value="90">`;
  const inp=d.querySelector('input'); let last=0;
  inp.oninput=()=>{document.getElementById('v'+id).textContent=inp.value;
    const n=Date.now(); if(n-last>80){last=n;send('servo/cmd',id+','+inp.value+',90');}};
  inp.onchange=()=>send('servo/cmd',id+','+inp.value+',90');
  box.appendChild(d);
});
</script></body></html>"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--listen", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--xsub", default="tcp://127.0.0.1:5555")
    ap.add_argument("--password", default=os.environ.get("MICROZMQ_WEB_PASS", ""))
    a = ap.parse_args()
    if not a.password:
        raise SystemExit("Set a password: MICROZMQ_WEB_PASS=... or --password ...")

    ctx = zmq.Context()
    pub = ctx.socket(zmq.PUB)
    pub.connect(a.xsub)
    lock = threading.Lock()
    time.sleep(0.3)   # let the connection come up

    class H(BaseHTTPRequestHandler):
        def _send(self, code, body=b"", ctype="text/plain"):
            self.send_response(code)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            if self.path in ("/", "/index.html"):
                self._send(200, PAGE.encode(), "text/html; charset=utf-8")
            else:
                self._send(404)

        def do_POST(self):
            if self.path != "/api/cmd":
                return self._send(404)
            if not hmac.compare_digest(self.headers.get("X-Pass", ""), a.password):
                time.sleep(1)   # slow down password guessing
                return self._send(401, b"bad password")
            try:
                n = int(self.headers.get("Content-Length", 0))
                msg = json.loads(self.rfile.read(min(n, 512)))
                topic, payload = msg["topic"], str(msg["payload"])
                if topic not in ALLOWED or not ALLOWED[topic].match(payload):
                    return self._send(400, b"not allowed")
            except Exception:
                return self._send(400, b"bad request")
            with lock:
                pub.send_multipart([("robot/in/" + topic).encode(), payload.encode()])
            self._send(200, b"ok")

        def log_message(self, *args):
            pass

    print(f"web control on http://{a.listen}:{a.port}  -> {a.xsub}")
    ThreadingHTTPServer((a.listen, a.port), H).serve_forever()


if __name__ == "__main__":
    main()
