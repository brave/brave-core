#include "brave/browser/brave_vpn/poc_wireguard/wireguard_tunnel.h"

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/compiler_specific.h"

namespace wireguard {

namespace {

void BoringtunLoggerCb(const char* msg) {
  LOG(INFO) << "[boringtun] " << msg;
}

}  // namespace

WireGuardConfig::WireGuardConfig() = default;
WireGuardConfig::WireGuardConfig(const WireGuardConfig&) = default;
WireGuardConfig::~WireGuardConfig() = default;

WireGuardTunnel::WireGuardTunnel(BoringtunLoader* loader,
                                 std::unique_ptr<TunInterface> tun,
                                 std::unique_ptr<UdpTransport> udp)
    : bt_(loader), tun_(std::move(tun)), udp_(std::move(udp)) {}

NO_SANITIZE("cfi-icall")
WireGuardTunnel::~WireGuardTunnel() {
  Stop();
  if (tunnel_) {
    bt_->tunnel_free(tunnel_);
    tunnel_ = nullptr;
  }
  tun_->RemoveRoutes();
  tun_->Close();
  udp_->Close();
}

NO_SANITIZE("cfi-icall")
bool WireGuardTunnel::Init(const WireGuardConfig& cfg) {
  // set_logging_function sets a global tracing subscriber - safe to call once
  // per process. Returns false if already set; non-fatal.
  if (!bt_->set_logging_function(BoringtunLoggerCb)) {
    LOG(WARNING) << "set_logging_function() returned false (already set?)";
  }

  const char* psk =
      cfg.preshared_key.empty() ? nullptr : cfg.preshared_key.c_str();
  tunnel_ = bt_->new_tunnel(cfg.local_private_key.c_str(),
                            cfg.peer_public_key.c_str(), psk,
                            cfg.keepalive_seconds, /*index=*/0);
  if (!tunnel_) {
    LOG(ERROR) << "new_tunnel() failed - verify keys are valid base64";
    return false;
  }
  LOG(INFO) << "boringtun tunnel created";

  if (!tun_->Open() || !tun_->Configure(cfg.local_tun_ip, cfg.mtu) ||
      !tun_->AddRoutes(cfg.allowed_ips, cfg.peer_ip) ||
      !tun_->SetDns(cfg.dns_servers) ||
      !udp_->Open(cfg.peer_ip, cfg.peer_port)) {
    return false;
  }

  wireguard_result r =
      bt_->wireguard_force_handshake(tunnel_, out_buf_, kBufSize);
  if (r.op == WIREGUARD_ERROR) {
    LOG(ERROR) << "wireguard_force_handshake() returned error";
  } else if (r.op == WRITE_TO_NETWORK && r.size > 0) {
    LOG(INFO) << "Sending initial handshake (" << r.size << " B)";
    SendToNetwork(out_buf_, r.size);
  }
  return true;
}

bool WireGuardTunnel::Start(StoppedCallback on_stopped) {
  on_stopped_ = std::move(on_stopped);

  tunnel_thread_ = std::make_unique<base::Thread>("wireguard-tunnel");
  base::Thread::Options opts(base::MessagePumpType::IO, /*stack_size=*/0);
  if (!tunnel_thread_->StartWithOptions(std::move(opts))) {
    LOG(ERROR) << "Failed to start tunnel thread";
    tunnel_thread_.reset();
    return false;
  }

  tunnel_thread_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&WireGuardTunnel::SetupOnThread, base::Unretained(this)));
  return true;
}

void WireGuardTunnel::Stop() {
  if (!tunnel_thread_ || !tunnel_thread_->IsRunning()) {
    return;
  }
  // Post teardown before joining - Thread::Stop() flushes pending tasks,
  // so TeardownOnThread is guaranteed to run before the join completes.
  tunnel_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&WireGuardTunnel::TeardownOnThread,
                                base::Unretained(this)));
  tunnel_thread_->Stop();
}

void WireGuardTunnel::SetupOnThread() {
  tun_->WatchReadable(base::BindRepeating(&WireGuardTunnel::OnTunReadable,
                                          base::Unretained(this)));
  udp_->WatchReadable(base::BindRepeating(&WireGuardTunnel::OnUdpReadable,
                                          base::Unretained(this)));
  tick_timer_ = std::make_unique<base::RepeatingTimer>();
  tick_timer_->Start(
      FROM_HERE, kTickInterval,
      base::BindRepeating(&WireGuardTunnel::OnTick, base::Unretained(this)));

  LOG(INFO) << "Event loop started on thread '" << tunnel_thread_->thread_name()
            << "' (tun=" << tun_->GetName() << ")";
}

void WireGuardTunnel::TeardownOnThread() {
  // Guard against double-teardown (I/O error path races with Stop()).
  if (!tick_timer_) {
    return;
  }
  tick_timer_->Stop();
  tick_timer_.reset();
  tun_->StopWatching();
  udp_->StopWatching();

  LOG(INFO) << "Event loop stopped";

  if (on_stopped_) {
    std::move(on_stopped_).Run();
  }
}

void WireGuardTunnel::StopAsync() {
  tunnel_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&WireGuardTunnel::TeardownOnThread,
                                base::Unretained(this)));
}

NO_SANITIZE("cfi-icall")
void WireGuardTunnel::OnTunReadable() {
  // Drain the entire TUN buffer. On Windows (Wintun), the read event is
  // edge-triggered — if we stop after one packet, remaining packets sit
  // in the ring buffer until the next edge, adding up to 100ms latency
  // per packet. On POSIX this is also more efficient than relying on the
  // watcher to re-fire for each packet.
  while (true) {
    const int n = tun_->Read(tun_buf_, kBufSize);
    if (n == 0) {
      break;
    }  // empty — stop draining
    if (n < 0) {
      LOG(ERROR) << "TUN read error - stopping tunnel";
      StopAsync();
      return;
    }
    wireguard_result r = bt_->wireguard_write(
        tunnel_, tun_buf_, static_cast<uint32_t>(n), out_buf_, kBufSize);
    switch (r.op) {
      case WRITE_TO_NETWORK:
        VLOG(2) << "TUN->WG " << n << " B -> " << r.size << " B";
        SendToNetwork(out_buf_, r.size);
        break;
      case WIREGUARD_DONE:
        VLOG(2) << "TUN->WG handshake pending";
        break;
      case WIREGUARD_ERROR:
        LOG(ERROR) << "wireguard_write error";
        break;
      default:
        LOG(WARNING) << "wireguard_write unexpected op=" << r.op;
    }
  }
}

NO_SANITIZE("cfi-icall")
void WireGuardTunnel::OnUdpReadable() {
  // Drain all pending datagrams for the same reason as OnTunReadable().
  while (true) {
    const int n = udp_->Recv(udp_buf_, kBufSize);
    if (n == 0) {
      break;  // WSAEWOULDBLOCK on Windows, or fd not ready on POSIX.
    }
    if (n < 0) {
      LOG(ERROR) << "UDP recv error - stopping tunnel";
      StopAsync();
      return;
    }
    wireguard_result r = bt_->wireguard_read(
        tunnel_, udp_buf_, static_cast<uint32_t>(n), out_buf_, kBufSize);

    // WRITE_TO_NETWORK means boringtun needs to send a handshake response
    // before it can produce plaintext. Drain until it's done.
    while (r.op == WRITE_TO_NETWORK) {
      VLOG(2) << "WG->NET handshake reply " << r.size << " B";
      SendToNetwork(out_buf_, r.size);
      r = bt_->wireguard_tick(tunnel_, out_buf_, kBufSize);
    }

    switch (r.op) {
      case WRITE_TO_TUNNEL_IPV4:
      case WRITE_TO_TUNNEL_IPV6:
        VLOG(2) << "WG->TUN " << n << " B -> " << r.size << " B";
        if (tun_->Write(out_buf_, r.size) < 0) {
          LOG(ERROR) << "TUN write error";
        }
        break;
      case WIREGUARD_DONE:
        VLOG(3) << "WG->TUN DONE";
        break;
      case WIREGUARD_ERROR:
        LOG(ERROR) << "wireguard_read error";
        break;
      default:
        LOG(WARNING) << "wireguard_read unexpected op=" << r.op;
    }
  }
}

NO_SANITIZE("cfi-icall")
void WireGuardTunnel::OnTick() {
  // Drain: wireguard_tick() produces at most one output packet per call.
  wireguard_result r;
  do {
    r = bt_->wireguard_tick(tunnel_, out_buf_, kBufSize);
    if (r.op == WRITE_TO_NETWORK && r.size > 0) {
      VLOG(3) << "Tick -> network " << r.size << " B";
      SendToNetwork(out_buf_, r.size);
    }
  } while (r.op == WRITE_TO_NETWORK);

  if (VLOG_IS_ON(2) && tick_timer_) {
    stats s = bt_->wireguard_stats(tunnel_);
    VLOG(2) << "stats: tx=" << s.tx_bytes << " rx=" << s.rx_bytes
            << " rtt=" << s.estimated_rtt << "ms"
            << " last_hs=" << s.time_since_last_handshake;
  }
}

void WireGuardTunnel::SendToNetwork(const uint8_t* buf, size_t size) {
  if (udp_->Send(buf, size) < 0) {
    LOG(ERROR) << "UDP send failed";
  }
}

}  // namespace wireguard
