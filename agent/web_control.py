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
    "drive/stop": re.compile(r"^$"),
    "servo/center": re.compile(r"^$"),
    "emotion": re.compile(r"^(neutral|happy|sad|angry)$"),
    "face/look": re.compile(r"^-?[01](\.[0-9]+)?,-?[01](\.[0-9]+)?$"),
    "drive/cmd": re.compile(r"^-?[01](\.[0-9]{1,2})?,-?[01](\.[0-9]{1,2})?$"),   # x (turn), y (forward)
    "audio/beep": re.compile(r"^[0-9]{2,4},[0-9]{1,4}$"),
}

PAGE = r"""<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot Remote</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:16px;max-width:480px;margin:auto}
h1{font-size:20px} .s{margin:18px 0} .s label{display:flex;justify-content:space-between}
input[type=range]{width:100%;height:36px} button{font-size:16px;padding:12px 16px;margin:4px;border-radius:8px;border:0;background:#2b6cb0;color:#fff}
#joy{display:block;margin:12px auto;touch-action:none;background:#1a202c;border-radius:50%}
#st{font-size:13px;color:#9ae6b4;min-height:18px}
</style></head><body>
<h1>Robot Remote</h1><div id="st"></div>
<canvas id="joy" width="260" height="260"></canvas>
<div id="drv" style="text-align:center;font-size:13px;color:#a0aec0">drive: 0.00, 0.00</div>
<div id="sliders"></div>
<button onclick="send('drive/stop','')" style="background:#c53030">STOP</button>
<button onclick="send('servo/center','')">Center scanner</button>
<button onclick="send('audio/beep','880,150')">Beep</button>
<div>
<button onclick="send('emotion','happy')">Happy</button>
<button onclick="send('emotion','sad')">Sad</button>
<button onclick="send('emotion','angry')">Angry</button>
<button onclick="send('emotion','neutral')">Neutral</button>
</div>
<button onclick="localStorage.removeItem('pw');location.reload()" style="background:#444">Change password</button>
<script>
const SERVOS=[["Camera / laser scanner",0,0,180]];
let pw=localStorage.getItem('pw')||prompt('Password'); if(pw)localStorage.setItem('pw',pw);
const st=document.getElementById('st');
async function send(topic,payload){
  try{
    const r=await fetch('/api/cmd',{method:'POST',headers:{'Content-Type':'application/json','X-Pass':pw},body:JSON.stringify({topic,payload})});
    st.textContent=r.ok?'sent: '+topic+' '+payload:(r.status==401?'wrong password':'error '+r.status);
  }catch(e){st.textContent='no connection';}
}
// ---- round joystick: x = turn (right +), y = forward (up +). Sends 10x/s while held, 0,0 on release.
const cv=document.getElementById('joy'), cx=cv.getContext('2d'), R=130, KNOB=38;
let jx=0, jy=0, held=false, timer=null;
function draw(){
  cx.clearRect(0,0,260,260);
  cx.strokeStyle='#4a5568'; cx.lineWidth=2;
  cx.beginPath(); cx.arc(R,R,R-3,0,7); cx.stroke();
  cx.beginPath(); cx.moveTo(R,10); cx.lineTo(R,250); cx.moveTo(10,R); cx.lineTo(250,R); cx.stroke();
  const px=R+jx*(R-KNOB), py=R-jy*(R-KNOB);
  cx.fillStyle=held?'#63b3ed':'#2b6cb0'; cx.beginPath(); cx.arc(px,py,KNOB,0,7); cx.fill();
}
function sendDrive(){
  document.getElementById('drv').textContent='drive: '+jx.toFixed(2)+', '+jy.toFixed(2);
  send('drive/cmd', jx.toFixed(2)+','+jy.toFixed(2));
}
function move(e){
  const r=cv.getBoundingClientRect();
  let x=((e.clientX-r.left)/r.width*260-R)/(R-KNOB), y=-((e.clientY-r.top)/r.height*260-R)/(R-KNOB);
  const m=Math.hypot(x,y); if(m>1){x/=m;y/=m;}
  jx=Math.round(x*100)/100; jy=Math.round(y*100)/100; draw();
}
cv.onpointerdown=e=>{cv.setPointerCapture(e.pointerId); held=true; move(e); sendDrive();
  timer=setInterval(sendDrive,100);};
cv.onpointermove=e=>{if(held)move(e);};
function release(){ if(!held)return; held=false; clearInterval(timer); jx=jy=0; draw(); sendDrive(); setTimeout(sendDrive,150); }
cv.onpointerup=release; cv.onpointercancel=release;
draw();

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
