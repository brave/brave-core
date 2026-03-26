#include "brave/browser/brave_vpn/poc_wireguard/tun_interface_linux.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace wireguard {

namespace {

// Detect which DNS resolver manager is available on this system.
// Ubuntu 18.04+: systemd-resolved via resolvectl
// Older Debian/Ubuntu: openresolv via resolvconf
// Fallback: direct /etc/resolv.conf edit
DnsManager DetectDnsManager() {
  if (::system("which resolvectl > /dev/null 2>&1") == 0) {
    return DnsManager::kResolvectl;
  }
  if (::system("which resolvconf > /dev/null 2>&1") == 0) {
    return DnsManager::kResolvconf;
  }
  return DnsManager::kResolveDotConf;
}

std::string GetDefaultGateway() {
  // Parse /proc/net/route - more reliable than shelling out to `ip route`.
  // Each line: Iface Dest Gateway Flags RefCnt Use Metric Mask ...
  // All values are hex, little-endian. Dest==0 is the default route.
  std::ifstream f("/proc/net/route");
  if (!f.is_open()) {
    return "";
  }
  std::string line;
  std::getline(f, line);  // skip header
  while (std::getline(f, line)) {
    std::istringstream ss(line);
    std::string iface, dest_hex, gw_hex;
    ss >> iface >> dest_hex >> gw_hex;
    if (dest_hex == "00000000") {
      // Gateway is 4 hex bytes in host byte order (little-endian on x86).
      uint32_t gw = static_cast<uint32_t>(std::stoul(gw_hex, nullptr, 16));
      struct in_addr addr;
      addr.s_addr = gw;  // already in network byte order after stoul on LE
      char buf[INET_ADDRSTRLEN];
      ::inet_ntop(AF_INET, &addr, buf, sizeof(buf));
      return buf;
    }
  }
  return "";
}

}  // namespace

LinuxTunInterface::LinuxTunInterface() = default;
LinuxTunInterface::~LinuxTunInterface() {
  Close();
}

// ── TunInterface ─────────────────────────────────────────────────────────────

bool LinuxTunInterface::Open() {
  // Linux TUN devices are created by opening /dev/net/tun and calling
  // TUNSETIFF. IFF_NO_PI suppresses the 4-byte protocol-info header that
  // macOS utun prepends - packets on Linux are raw IP with no prefix.
  int fd =
      HANDLE_EINTR(::open("/dev/net/tun", O_RDWR | O_NONBLOCK | O_CLOEXEC));
  if (fd < 0) {
    PLOG(ERROR) << "open(/dev/net/tun) failed - is tun.ko loaded?";
    return false;
  }

  struct ifreq ifr = {};
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  // Leave ifr_name zeroed - kernel assigns the next available tunN name.
  if (::ioctl(fd, TUNSETIFF, &ifr) < 0) {
    PLOG(ERROR) << "TUNSETIFF failed";
    ::close(fd);
    return false;
  }

  fd_ = fd;
  name_ = ifr.ifr_name;
  LOG(INFO) << "Opened TUN interface: " << name_ << " (fd=" << fd_ << ")";
  return true;
}

bool LinuxTunInterface::Configure(const std::string& local_ip, int mtu) {
  if (fd_ < 0) {
    LOG(ERROR) << "Configure() called before Open()";
    return false;
  }
  // Point-to-point: assign local_ip as both local and peer address so the
  // interface gets a /32 host route automatically.
  const std::string cmd =
      "ip addr add " + local_ip + " peer " + local_ip + " dev " + name_;
  LOG(INFO) << "Configuring interface: " << cmd;
  if (::system(cmd.c_str()) != 0) {
    LOG(ERROR) << "ip addr add failed";
    return false;
  }
  const std::string up_cmd =
      "ip link set dev " + name_ + " up mtu " + std::to_string(mtu);
  LOG(INFO) << "Bringing up: " << up_cmd;
  if (::system(up_cmd.c_str()) != 0) {
    LOG(ERROR) << "ip link set up failed";
    return false;
  }
  return true;
}

bool LinuxTunInterface::RunIp(const std::string& args) {
  const std::string cmd = "ip " + args;
  LOG(INFO) << "ip: " << cmd;
  if (::system(cmd.c_str()) != 0) {
    LOG(ERROR) << "ip command failed: " << cmd;
    return false;
  }
  return true;
}

bool LinuxTunInterface::AddRoutes(const std::vector<std::string>& allowed_ips,
                                  const std::string& endpoint_ip) {
  original_gateway_ = GetDefaultGateway();
  if (original_gateway_.empty()) {
    LOG(ERROR) << "AddRoutes: could not determine default gateway";
    return false;
  }
  LOG(INFO) << "Original gateway: " << original_gateway_;

  // Protect the WireGuard endpoint - must route via the original gateway.
  if (!RunIp("route add " + endpoint_ip + "/32 via " + original_gateway_)) {
    return false;
  }
  endpoint_route_ = endpoint_ip;

  for (const auto& cidr : allowed_ips) {
    if (cidr == "0.0.0.0/0") {
      // Split into two /1s to avoid replacing the default route entry.
      if (!RunIp("route add 0.0.0.0/1 dev " + name_)) {
        return false;
      }
      if (!RunIp("route add 128.0.0.0/1 dev " + name_)) {
        return false;
      }
      added_routes_.push_back("0.0.0.0/1");
      added_routes_.push_back("128.0.0.0/1");
    } else {
      if (!RunIp("route add " + cidr + " dev " + name_)) {
        return false;
      }
      added_routes_.push_back(cidr);
    }
  }
  return true;
}

bool LinuxTunInterface::SetDns(const std::vector<std::string>& servers) {
  if (servers.empty()) {
    return true;
  }

  dns_manager_ = DetectDnsManager();

  if (dns_manager_ == DnsManager::kResolvectl) {
    // systemd-resolved: set DNS per-interface, flush cache.
    std::string cmd = "resolvectl dns " + name_;
    for (const auto& s : servers) {
      cmd += " " + s;
    }
    if (::system(cmd.c_str()) != 0) {
      LOG(ERROR) << "resolvectl dns failed";
      return false;
    }
    // Make the tunnel interface the default DNS route.
    if (::system(("resolvectl domain " + name_ + " ~.").c_str()) != 0) {
      LOG(ERROR) << "resolvectl domain failed";
      return false;
    }
    ::system("resolvectl flush-caches");
    LOG(INFO) << "Set DNS via resolvectl on " << name_;
    return true;
  }

  if (dns_manager_ == DnsManager::kResolvconf) {
    // openresolv: pipe a resolv.conf fragment into resolvconf.
    std::string fragment;
    for (const auto& s : servers) {
      fragment += "nameserver " + s + "\n";
    }
    const std::string cmd =
        "echo '" + fragment + "' | resolvconf -a " + name_ + ".vpn -m 0";
    if (::system(cmd.c_str()) != 0) {
      LOG(ERROR) << "resolvconf -a failed";
      return false;
    }
    LOG(INFO) << "Set DNS via resolvconf on " << name_;
    return true;
  }

  // Fallback: write /etc/resolv.conf directly.
  // Save the original content so RestoreDns() can put it back.
  {
    std::ifstream in("/etc/resolv.conf");
    if (in.is_open()) {
      std::ostringstream ss;
      ss << in.rdbuf();
      original_dns_ = ss.str();
    }
  }
  {
    std::ofstream out("/etc/resolv.conf", std::ios::trunc);
    if (!out.is_open()) {
      LOG(ERROR) << "SetDns: cannot write /etc/resolv.conf";
      return false;
    }
    out << "# Written by BraveVPN - restored on tunnel teardown\n";
    for (const auto& s : servers) {
      out << "nameserver " << s << "\n";
    }
  }
  LOG(INFO) << "Set DNS via /etc/resolv.conf";
  return true;
}

void LinuxTunInterface::RestoreDns() {
  switch (dns_manager_) {
    case DnsManager::kResolvectl:
      // Remove per-interface DNS config - systemd-resolved reverts to global.
      ::system(("resolvectl revert " + name_).c_str());
      ::system("resolvectl flush-caches");
      break;
    case DnsManager::kResolvconf:
      ::system(("resolvconf -d " + name_ + ".vpn").c_str());
      break;
    case DnsManager::kResolveDotConf:
      if (!original_dns_.empty()) {
        std::ofstream out("/etc/resolv.conf", std::ios::trunc);
        if (out.is_open()) {
          out << original_dns_;
        }
        original_dns_.clear();
      }
      break;
    case DnsManager::kUnknown:
      break;
  }
  dns_manager_ = DnsManager::kUnknown;
}

void LinuxTunInterface::RemoveRoutes() {
  RestoreDns();

  for (auto it = added_routes_.rbegin(); it != added_routes_.rend(); ++it) {
    RunIp("route del " + *it + " dev " + name_);
  }
  added_routes_.clear();

  if (!endpoint_route_.empty()) {
    RunIp("route del " + endpoint_route_ + "/32 via " + original_gateway_);
    endpoint_route_.clear();
  }
  original_gateway_.clear();
}

void LinuxTunInterface::Close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
    LOG(INFO) << "Closed TUN interface: " << name_;
  }
}

int LinuxTunInterface::Read(uint8_t* buf, size_t buf_size) {
  // Linux IFF_NO_PI: packets are raw IP, no header to strip.
  ssize_t n = HANDLE_EINTR(::read(fd_, buf, buf_size));
  if (n < 0) {
    // If the error is just "no data available right now", return 0.
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }

    // An actual error occurred (e.g., device disconnected)
    PLOG(ERROR) << "tun read error";
    return -1;
  }

  return static_cast<int>(n);
}

int LinuxTunInterface::Write(const uint8_t* buf, size_t size) {
  // Linux IFF_NO_PI: write raw IP directly, no header to prepend.
  ssize_t n = HANDLE_EINTR(::write(fd_, buf, size));
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // The OS tunnel queue is full.
      // Drop the packet and let the higher-level protocol (like TCP) handle it.
      VLOG(1) << "TUN write buffer full, dropping packet";
      return 0;
    }
    PLOG(ERROR) << "tun write error";
    return -1;
  }

  return static_cast<int>(n);
}

std::string LinuxTunInterface::GetName() const {
  return name_;
}

void LinuxTunInterface::WatchReadable(base::RepeatingClosure on_readable) {
  watcher_ =
      base::FileDescriptorWatcher::WatchReadable(fd_, std::move(on_readable));
}

void LinuxTunInterface::StopWatching() {
  watcher_.reset();
}

std::unique_ptr<TunInterface> CreateTunInterface() {
  return std::make_unique<LinuxTunInterface>();
}

}  // namespace wireguard
