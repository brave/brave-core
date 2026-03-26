#ifndef WIREGUARD_WIREGUARD_TUNNEL_H_
#define WIREGUARD_WIREGUARD_TUNNEL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/threading/thread.h"
#include "base/timer/timer.h"
#include "brave/browser/brave_vpn/poc_wireguard/boringtun_api.h"
#include "brave/browser/brave_vpn/poc_wireguard/boringtun_loader.h"
#include "brave/browser/brave_vpn/poc_wireguard/tun_interface.h"
#include "brave/browser/brave_vpn/poc_wireguard/udp_transport.h"

namespace wireguard {

struct WireGuardConfig {
  WireGuardConfig();
  WireGuardConfig(const WireGuardConfig&);
  ~WireGuardConfig();

  std::string local_private_key;
  std::string peer_public_key;
  std::string preshared_key;  // Empty = unused.

  std::string peer_ip;
  uint16_t peer_port = 51820;

  std::string local_tun_ip;  // e.g. "10.0.0.2"

  std::vector<std::string> dns_servers = {"1.1.1.1", "8.8.8.8"};
  std::vector<std::string> allowed_ips = {"0.0.0.0/0"};

  uint16_t keepalive_seconds = 25;
  int mtu = 1420;
};

class WireGuardTunnel {
 public:
  // Invoked on the tunnel thread when the event loop exits (clean stop or
  // I/O error). Useful for notifying the main thread / XPC layer.
  using StoppedCallback = base::OnceClosure;

  WireGuardTunnel(BoringtunLoader* loader,
                  std::unique_ptr<TunInterface> tun,
                  std::unique_ptr<UdpTransport> udp);
  ~WireGuardTunnel();

  // Initializes crypto, TUN interface, routes, DNS, and UDP socket.
  // Must be called before Start(). Returns false on any failure.
  bool Init(const WireGuardConfig& cfg);

  // Starts the tunnel event loop on a dedicated IO thread.
  // Returns immediately - does not block the caller.
  // |on_stopped| is called on the tunnel thread when the loop exits.
  bool Start(StoppedCallback on_stopped = {});

  // Thread-safe. Posts teardown to the tunnel thread and joins it.
  // Safe to call multiple times or if Start() was never called.
  void Stop();

 private:
  void SetupOnThread();
  void TeardownOnThread();
  void StopAsync();

  void OnTunReadable();
  void OnUdpReadable();
  void OnTick();
  void SendToNetwork(const uint8_t* buf, size_t size);

  raw_ptr<BoringtunLoader> bt_;  // Not owned.
  std::unique_ptr<TunInterface> tun_;
  std::unique_ptr<UdpTransport> udp_;

  // new_tunnel() returns non-const; FFI ops take const implicitly;
  // tunnel_free() requires non-const.
  RAW_PTR_EXCLUSION wireguard_tunnel* tunnel_ = nullptr;

  StoppedCallback on_stopped_;
  std::unique_ptr<base::Thread> tunnel_thread_;
  std::unique_ptr<base::RepeatingTimer> tick_timer_;

  static constexpr base::TimeDelta kTickInterval = base::Milliseconds(250);
  static constexpr size_t kBufSize = MAX_WIREGUARD_PACKET_SIZE;
  uint8_t tun_buf_[kBufSize];
  uint8_t udp_buf_[kBufSize];
  uint8_t out_buf_[kBufSize];
};

}  // namespace wireguard

#endif  // WIREGUARD_WIREGUARD_TUNNEL_H_
