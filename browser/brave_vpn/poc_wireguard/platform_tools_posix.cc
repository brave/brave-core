#include <signal.h>
#include <unistd.h>

#include "brave/browser/brave_vpn/poc_wireguard/platform_tools.h"

namespace wireguard {

namespace {

sigset_t g_sigset;

}  // namespace

void InitShutdownSignal() {
  sigemptyset(&g_sigset);
  sigaddset(&g_sigset, SIGINT);
  sigaddset(&g_sigset, SIGTERM);
  // Block before any threads are spawned - mask is inherited by all children,
  // so no thread other than the main thread will ever handle these signals.
  pthread_sigmask(SIG_BLOCK, &g_sigset, nullptr);
}

int WaitForShutdownSignal() {
  int sig = 0;
  sigwait(&g_sigset, &sig);
  return sig;
}

bool IsElevated() {
  return ::getuid() == 0;
}

}  // namespace wireguard
