// micro-ZMQ test tool.
//   microzmq_tool sub robot/out/                  watch what the robot sends
//   microzmq_tool pub robot/in/emotion happy      change the robot's face
//   microzmq_tool pub robot/in/servo/cmd 0,45,60  move the head
// Optional: --xsub URL / --xpub URL (defaults tcp://127.0.0.1:5555 / :5556)
#include <zmq.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
  std::string xsub = "tcp://127.0.0.1:5555", xpub = "tcp://127.0.0.1:5556";
  std::vector<std::string> pos;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--xsub") && i + 1 < argc) xsub = argv[++i];
    else if (!strcmp(argv[i], "--xpub") && i + 1 < argc) xpub = argv[++i];
    else pos.push_back(argv[i]);
  }

  void* ctx = zmq_ctx_new();
  if (pos.size() >= 2 && pos[0] == "sub") {
    void* s = zmq_socket(ctx, ZMQ_SUB);
    zmq_connect(s, xpub.c_str());
    zmq_setsockopt(s, ZMQ_SUBSCRIBE, pos[1].data(), pos[1].size());
    for (;;) {
      char t[256], p[4096];
      int tn = zmq_recv(s, t, sizeof t - 1, 0);
      if (tn < 0) break;
      int more = 0; size_t ms = sizeof more;
      zmq_getsockopt(s, ZMQ_RCVMORE, &more, &ms);
      int pn = more ? zmq_recv(s, p, sizeof p - 1, 0) : 0;
      if (pn < 0) break;
      t[tn < (int)sizeof t ? tn : sizeof t - 1] = 0;
      p[pn < (int)sizeof p ? pn : sizeof p - 1] = 0;
      printf("%s %s\n", t, p);
      fflush(stdout);
    }
  } else if (pos.size() >= 3 && pos[0] == "pub") {
    std::string payload;
    for (size_t i = 2; i < pos.size(); i++) payload += (i > 2 ? " " : "") + pos[i];
    void* s = zmq_socket(ctx, ZMQ_PUB);
    zmq_connect(s, xsub.c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    zmq_send(s, pos[1].data(), pos[1].size(), ZMQ_SNDMORE);
    zmq_send(s, payload.data(), payload.size(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int zero = 0;
    zmq_setsockopt(s, ZMQ_LINGER, &zero, sizeof zero);
    zmq_close(s);
  } else {
    fprintf(stderr, "usage:\n  %s sub <topic-prefix>\n  %s pub <topic> <payload...>\n", argv[0], argv[0]);
    return 1;
  }
  zmq_ctx_destroy(ctx);
  return 0;
}
