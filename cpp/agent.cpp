// micro-ZMQ agent: bridges ESP32 TCP clients to ZeroMQ PUB/SUB (see docs/PROTOCOL.md).
//   usage: microzmq_agent [--listen 0.0.0.0] [--port 7400]
//                         [--xsub tcp://127.0.0.1:5555] [--xpub tcp://127.0.0.1:5556]
//                         [--token change-me] [--timeout 15]
// One thread per ESP32 client. Each thread polls the TCP socket and its ZeroMQ SUB
// socket together, so ZeroMQ sockets are only ever used from one thread.
#include <zmq.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::steady_clock;

namespace {

enum : uint8_t { T_HELLO = 1, T_SUB = 2, T_PUB = 3, T_PING = 4, T_PONG = 5, T_ACK = 6 };
constexpr size_t MAX_FRAME = 8192;
constexpr size_t MAX_OUT_FRAME = 1000;   // ESP32 accepts at most 1024; larger frames would disconnect it

struct Args {
  std::string listen = "0.0.0.0";
  int port = 7400;
  std::string xsub = "tcp://127.0.0.1:5555";   // publish here
  std::string xpub = "tcp://127.0.0.1:5556";   // subscribe here
  std::string token = "change-me";
  int timeout = 15;                            // drop a silent client after N seconds
};

std::atomic<bool> g_running{true};
std::atomic<int> g_clients{0};

void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logf(const char* fmt, ...) {
  char tbuf[32];
  time_t t = time(nullptr);
  strftime(tbuf, sizeof tbuf, "%H:%M:%S", localtime(&t));
  printf("%s ", tbuf);
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
  fflush(stdout);
}

bool sendAll(int fd, const uint8_t* d, size_t n) {
  while (n) {
    ssize_t w = send(fd, d, n, MSG_NOSIGNAL);
    if (w <= 0) return false;
    d += w;
    n -= (size_t)w;
  }
  return true;
}

bool sendFrame(int fd, uint8_t type, const uint8_t* body, size_t len) {
  std::vector<uint8_t> out(len + 3);
  uint16_t n = (uint16_t)(len + 1);
  out[0] = n & 0xFF;
  out[1] = n >> 8;
  out[2] = type;
  if (len) memcpy(&out[3], body, len);
  return sendAll(fd, out.data(), out.size());
}

void closeSock(void* s) {
  if (!s) return;
  int zero = 0;
  zmq_setsockopt(s, ZMQ_LINGER, &zero, sizeof zero);
  zmq_close(s);
}

void handleClient(int fd, std::string peer, void* ctx, Args a) {
  g_clients++;
  void* pub = nullptr;
  void* sub = nullptr;
  bool hello = false;
  std::string name = "?";
  std::vector<uint8_t> rx;
  auto last = Clock::now();
  int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);

  while (g_running) {
    zmq_pollitem_t items[2];
    items[0] = {nullptr, fd, ZMQ_POLLIN, 0};
    int nitems = 1;
    if (sub) { items[1] = {sub, 0, ZMQ_POLLIN, 0}; nitems = 2; }
    int pr = zmq_poll(items, nitems, 200);
    if (pr < 0) break;

    auto idle = std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - last).count();
    if (idle > (hello ? a.timeout : 5)) break;

    // ---------- ESP32 -> ZeroMQ ----------
    if (items[0].revents & ZMQ_POLLIN) {
      uint8_t buf[2048];
      ssize_t n = recv(fd, buf, sizeof buf, 0);
      if (n <= 0) break;
      rx.insert(rx.end(), buf, buf + n);
      last = Clock::now();

      bool bad = false;
      while (rx.size() >= 2 && !bad) {
        size_t flen = rx[0] | (rx[1] << 8);
        if (flen < 1 || flen > MAX_FRAME) { bad = true; break; }
        if (rx.size() < 2 + flen) break;
        uint8_t type = rx[2];
        const uint8_t* body = rx.data() + 3;
        size_t blen = flen - 1;

        if (!hello) {
          if (type != T_HELLO) { bad = true; break; }
          const uint8_t* z = (const uint8_t*)memchr(body, 0, blen);
          std::string tok((const char*)body, z ? (size_t)(z - body) : blen);
          if (!a.token.empty() && tok != a.token) {
            logf("rejected %s: bad token", peer.c_str());
            bad = true;
            break;
          }
          if (z) name.assign((const char*)z + 1, blen - (size_t)(z - body) - 1);
          pub = zmq_socket(ctx, ZMQ_PUB);
          sub = zmq_socket(ctx, ZMQ_SUB);
          if (zmq_connect(pub, a.xsub.c_str()) != 0 || zmq_connect(sub, a.xpub.c_str()) != 0) {
            logf("zmq connect failed: %s", zmq_strerror(zmq_errno()));
            bad = true;
            break;
          }
          std::this_thread::sleep_for(std::chrono::milliseconds(100));   // slow joiner
          if (!sendFrame(fd, T_ACK, nullptr, 0)) { bad = true; break; }
          hello = true;
          logf("node '%s' connected from %s", name.c_str(), peer.c_str());
        } else if (type == T_SUB) {
          zmq_setsockopt(sub, ZMQ_SUBSCRIBE, body, blen);
          logf("[%s] subscribe '%.*s'", name.c_str(), (int)blen, (const char*)body);
        } else if (type == T_PUB && blen >= 1) {
          size_t tl = body[0];
          if (1 + tl <= blen) {
            zmq_send(pub, body + 1, tl, ZMQ_SNDMORE);
            zmq_send(pub, body + 1 + tl, blen - 1 - tl, 0);
          }
        } else if (type == T_PING) {
          if (!sendFrame(fd, T_PONG, nullptr, 0)) { bad = true; break; }
        }
        rx.erase(rx.begin(), rx.begin() + 2 + flen);
      }
      if (bad) break;
    }

    // ---------- ZeroMQ -> ESP32 ----------
    if (sub && nitems == 2 && (items[1].revents & ZMQ_POLLIN)) {
      bool ok = true;
      for (int k = 0; k < 32 && ok; k++) {
        zmq_msg_t m1;
        zmq_msg_init(&m1);
        if (zmq_msg_recv(&m1, sub, ZMQ_DONTWAIT) < 0) { zmq_msg_close(&m1); break; }
        std::vector<uint8_t> topic((uint8_t*)zmq_msg_data(&m1), (uint8_t*)zmq_msg_data(&m1) + zmq_msg_size(&m1));
        bool more = zmq_msg_more(&m1);
        zmq_msg_close(&m1);

        std::vector<uint8_t> payload;
        int extra = 0;
        while (more) {                       // keep first payload part, drop extras
          zmq_msg_t m2;
          zmq_msg_init(&m2);
          if (zmq_msg_recv(&m2, sub, 0) < 0) { zmq_msg_close(&m2); break; }
          if (extra++ == 0)
            payload.assign((uint8_t*)zmq_msg_data(&m2), (uint8_t*)zmq_msg_data(&m2) + zmq_msg_size(&m2));
          more = zmq_msg_more(&m2);
          zmq_msg_close(&m2);
        }
        if (extra != 1 || topic.size() > 255 || topic.size() + payload.size() + 2 > MAX_OUT_FRAME) continue;

        std::vector<uint8_t> body;
        body.push_back((uint8_t)topic.size());
        body.insert(body.end(), topic.begin(), topic.end());
        body.insert(body.end(), payload.begin(), payload.end());
        ok = sendFrame(fd, T_PUB, body.data(), body.size());
      }
      if (!ok) break;
    }
  }

  if (hello) logf("node '%s' disconnected", name.c_str());
  closeSock(pub);
  closeSock(sub);
  close(fd);
  g_clients--;
}

}  // namespace

int main(int argc, char** argv) {
  Args a;
  if (const char* t = getenv("MICROZMQ_TOKEN")) a.token = t;
  for (int i = 1; i < argc; i++) {
    std::string k = argv[i];
    auto next = [&]() -> const char* { return (i + 1 < argc) ? argv[++i] : ""; };
    if (k == "--listen") a.listen = next();
    else if (k == "--port") a.port = atoi(next());
    else if (k == "--xsub") a.xsub = next();
    else if (k == "--xpub") a.xpub = next();
    else if (k == "--token") a.token = next();
    else if (k == "--timeout") a.timeout = atoi(next());
    else {
      fprintf(stderr, "usage: %s [--listen IP] [--port N] [--xsub URL] [--xpub URL] [--token T] [--timeout S]\n", argv[0]);
      return 1;
    }
  }

  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, [](int) { g_running = false; });
  signal(SIGTERM, [](int) { g_running = false; });

  void* ctx = zmq_ctx_new();

  int srv = socket(AF_INET, SOCK_STREAM, 0);
  int one = 1;
  setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((uint16_t)a.port);
  if (inet_pton(AF_INET, a.listen.c_str(), &addr.sin_addr) != 1) {
    fprintf(stderr, "bad --listen address\n");
    return 1;
  }
  if (bind(srv, (sockaddr*)&addr, sizeof addr) != 0 || listen(srv, 8) != 0) {
    perror("bind/listen");
    return 1;
  }
  logf("agent listening on %s:%d  -> ZeroMQ %s / %s", a.listen.c_str(), a.port, a.xsub.c_str(), a.xpub.c_str());

  while (g_running) {
    pollfd pfd{srv, POLLIN, 0};
    if (poll(&pfd, 1, 500) <= 0) continue;
    sockaddr_in pa{};
    socklen_t pl = sizeof pa;
    int fd = accept(srv, (sockaddr*)&pa, &pl);
    if (fd < 0) continue;
    char ip[64];
    inet_ntop(AF_INET, &pa.sin_addr, ip, sizeof ip);
    std::string peer = std::string(ip) + ":" + std::to_string(ntohs(pa.sin_port));
    std::thread(handleClient, fd, peer, ctx, a).detach();
  }

  logf("shutting down");
  close(srv);
  while (g_clients > 0) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  zmq_ctx_term(ctx);
  return 0;
}
