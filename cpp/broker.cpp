// micro-ZMQ broker: ZeroMQ XSUB/XPUB proxy.
// Publishers connect to the XSUB port, subscribers connect to the XPUB port.
// If your AUV middleware already has a broker, you do not need this.
//   usage: microzmq_broker [--bind 127.0.0.1] [--xsub-port 5555] [--xpub-port 5556]
#include <zmq.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
  std::string bind = "127.0.0.1";
  int xsubPort = 5555, xpubPort = 5556;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--bind") && i + 1 < argc) bind = argv[++i];
    else if (!strcmp(argv[i], "--xsub-port") && i + 1 < argc) xsubPort = atoi(argv[++i]);
    else if (!strcmp(argv[i], "--xpub-port") && i + 1 < argc) xpubPort = atoi(argv[++i]);
    else { fprintf(stderr, "usage: %s [--bind IP] [--xsub-port N] [--xpub-port N]\n", argv[0]); return 1; }
  }

  void* ctx = zmq_ctx_new();
  void* xsub = zmq_socket(ctx, ZMQ_XSUB);
  void* xpub = zmq_socket(ctx, ZMQ_XPUB);
  std::string a = "tcp://" + bind + ":" + std::to_string(xsubPort);
  std::string b = "tcp://" + bind + ":" + std::to_string(xpubPort);
  if (zmq_bind(xsub, a.c_str()) != 0 || zmq_bind(xpub, b.c_str()) != 0) {
    fprintf(stderr, "bind failed: %s\n", zmq_strerror(zmq_errno()));
    return 1;
  }
  printf("broker: publishers -> %s | subscribers -> %s\n", a.c_str(), b.c_str());
  fflush(stdout);
  zmq_proxy(xsub, xpub, nullptr);   // blocks until the process is stopped
  zmq_close(xsub); zmq_close(xpub); zmq_ctx_term(ctx);
  return 0;
}
