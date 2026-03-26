#ifdef UNSAFE_BUFFERS_BUILD
// Interfaces directly with kernel control sockets and utun, requiring raw
// buffer operations. TODO: Migrate to base::span when productionising.
#pragma allow_unsafe_buffers
#endif

#include "brave/browser/brave_vpn/poc_wireguard/tun_interface_mac.h"

#include <arpa/inet.h>
#include <net/if_utun.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/socket.h>
#include <sys/sys_domain.h>
#include <unistd.h>

#include <vector>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "brave/browser/brave_vpn/poc_wireguard/boringtun_api.h"

namespace wireguard {

namespace {

constexpr size_t kUtunHeaderSize = 4;

std::string GetDefaultGateway() {
  FILE* f = ::popen("route -n get default 2>/dev/null", "r");
  if (!f) {
    return "";
  }
  char line[256];
  std::string gw;
  while (::fgets(line, sizeof(line), f)) {
    std::string s(line);
    const auto pos = s.find("gateway:");
    if (pos != std::string::npos) {
      gw = s.substr(pos + 8);
      gw.erase(0, gw.find_first_not_of(" \t"));
      gw.erase(gw.find_last_not_of(" \t\n\r") + 1);
      break;
    }
  }
  ::pclose(f);
  return gw;
}

}  // namespace

MacTunInterface::MacTunInterface() = default;

MacTunInterface::~MacTunInterface() {
  Close();
}

bool MacTunInterface::Open() {
  int fd = ::socket(AF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
  if (fd < 0) {
    PLOG(ERROR) << "utun: socket(AF_SYSTEM) failed";
    return false;
  }
  int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags == -1 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    PLOG(ERROR) << "utun: failed to set O_NONBLOCK";
    ::close(fd);
    return false;
  }

  struct ctl_info ci = {};
  ::strlcpy(ci.ctl_name, UTUN_CONTROL_NAME, sizeof(ci.ctl_name));
  if (::ioctl(fd, CTLIOCGINFO, &ci) < 0) {
    PLOG(ERROR) << "utun: CTLIOCGINFO failed";
    ::close(fd);
    return false;
  }

  struct sockaddr_ctl sc = {};
  sc.sc_len = sizeof(sc);
  sc.sc_family = AF_SYSTEM;
  sc.ss_sysaddr = AF_SYS_CONTROL;
  sc.sc_id = ci.ctl_id;
  sc.sc_unit = 0;  // 0 = let kernel assign

  if (::connect(fd, reinterpret_cast<struct sockaddr*>(&sc), sizeof(sc)) < 0) {
    PLOG(ERROR) << "utun: connect failed";
    ::close(fd);
    return false;
  }

  char ifname[IFNAMSIZ] = {};
  socklen_t len = sizeof(ifname);
  if (::getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &len) < 0) {
    PLOG(ERROR) << "utun: getsockopt(UTUN_OPT_IFNAME) failed";
    ::close(fd);
    return false;
  }

  fd_ = fd;
  name_ = ifname;
  LOG(INFO) << "Opened TUN interface: " << name_ << " (fd=" << fd_ << ")";
  return true;
}

bool MacTunInterface::Configure(const std::string& local_ip, int mtu) {
  if (fd_ < 0) {
    LOG(ERROR) << "utun: Configure() called before Open()";
    return false;
  }
  const std::string cmd = "ifconfig " + name_ + " inet " + local_ip + " " +
                          local_ip + " up mtu " + std::to_string(mtu);
  LOG(INFO) << "Configuring interface: " << cmd;
  if (::system(cmd.c_str()) != 0) {
    LOG(ERROR) << "ifconfig failed";
    return false;
  }
  return true;
}

bool MacTunInterface::RunRoute(const std::string& args) {
  const std::string cmd = "route -q " + args;
  LOG(INFO) << "route: " << cmd;
  if (::system(cmd.c_str()) != 0) {
    LOG(ERROR) << "route command failed: " << cmd;
    return false;
  }
  return true;
}

bool MacTunInterface::AddRoutes(const std::vector<std::string>& allowed_ips,
                                const std::string& endpoint_ip) {
  original_gateway_ = GetDefaultGateway();
  if (original_gateway_.empty()) {
    LOG(ERROR) << "AddRoutes: could not determine default gateway";
    return false;
  }
  LOG(INFO) << "Original gateway: " << original_gateway_;

  // The WireGuard endpoint must bypass the tunnel - encrypted UDP packets
  // must reach the peer via the original gateway, not be re-encrypted.
  if (!RunRoute("add -host " + endpoint_ip + " " + original_gateway_)) {
    return false;
  }
  endpoint_route_ = endpoint_ip;

  for (const auto& cidr : allowed_ips) {
    if (cidr == "0.0.0.0/0") {
      // Split 0/0 into two /1s to avoid replacing the default route entry.
      if (!RunRoute("add -net 0.0.0.0/1 -interface " + name_)) {
        return false;
      }
      if (!RunRoute("add -net 128.0.0.0/1 -interface " + name_)) {
        return false;
      }
      added_routes_.push_back("0.0.0.0/1");
      added_routes_.push_back("128.0.0.0/1");
    } else {
      if (!RunRoute("add -net " + cidr + " -interface " + name_)) {
        return false;
      }
      added_routes_.push_back(cidr);
    }
  }
  return true;
}

bool MacTunInterface::SetDns(const std::vector<std::string>& servers) {
  if (servers.empty()) {
    return true;
  }

  // Detect the active network interface from the default route.
  FILE* f = ::popen(
      "route -n get default 2>/dev/null | awk '/interface:/{print $2}'", "r");
  char ifname[64] = {};
  if (f) {
    ::fgets(ifname, sizeof(ifname), f);
    ::pclose(f);
    ifname[::strcspn(ifname, "\n")] = 0;
  }
  if (!*ifname) {
    LOG(ERROR) << "SetDns: could not detect default interface";
    return false;
  }
  original_interface_ = ifname;

  // Map BSD interface name (e.g. "en0") to networksetup service name
  // (e.g. "Wi-Fi").
  const std::string svc_cmd =
      "networksetup -listallhardwareports | "
      "awk '/Hardware Port/{port=$NF} /Device: " +
      std::string(ifname) + "/{print port}'";
  FILE* f2 = ::popen(svc_cmd.c_str(), "r");
  char svc[128] = {};
  if (f2) {
    ::fgets(svc, sizeof(svc), f2);
    ::pclose(f2);
    svc[::strcspn(svc, "\n")] = 0;
  }
  if (!*svc) {
    LOG(ERROR) << "SetDns: could not map interface '" << ifname
               << "' to service name";
    return false;
  }
  original_dns_service_ = svc;

  // Save existing DNS servers so we can restore them on teardown.
  const std::string get_cmd =
      "networksetup -getdnsservers \"" + original_dns_service_ + "\"";
  FILE* f3 = ::popen(get_cmd.c_str(), "r");
  if (f3) {
    char line[128];
    while (::fgets(line, sizeof(line), f3)) {
      line[::strcspn(line, "\n")] = 0;
      // "There aren't any DNS Servers set" -> system default (DHCP).
      if (::strncmp(line, "There aren't", 12) != 0) {
        original_dns_servers_.emplace_back(line);
      }
    }
    ::pclose(f3);
  }

  std::string cmd =
      "networksetup -setdnsservers \"" + original_dns_service_ + "\"";
  for (const auto& s : servers) {
    cmd += " " + s;
  }
  LOG(INFO) << "Setting DNS: " << cmd;
  return ::system(cmd.c_str()) == 0;
}

void MacTunInterface::RestoreDns() {
  if (original_dns_service_.empty()) {
    return;
  }
  std::string cmd =
      "networksetup -setdnsservers \"" + original_dns_service_ + "\"";
  if (original_dns_servers_.empty()) {
    cmd += " \"Empty\"";  // Restore to DHCP-provided default.
  } else {
    for (const auto& s : original_dns_servers_) {
      cmd += " " + s;
    }
  }
  LOG(INFO) << "Restoring DNS: " << cmd;
  ::system(cmd.c_str());
  original_dns_service_.clear();
  original_dns_servers_.clear();
}

void MacTunInterface::RemoveRoutes() {
  RestoreDns();

  for (auto it = added_routes_.rbegin(); it != added_routes_.rend(); ++it) {
    RunRoute("delete -net " + *it);
  }
  added_routes_.clear();

  if (!endpoint_route_.empty()) {
    RunRoute("delete -host " + endpoint_route_);
    endpoint_route_.clear();
  }
  original_gateway_.clear();
}

void MacTunInterface::Close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
    LOG(INFO) << "Closed TUN interface: " << name_;
  }
}

int MacTunInterface::Read(uint8_t* buf, size_t buf_size) {
  // utun prepends a 4-byte AF header; read into a temporary buffer and strip.
  uint8_t raw[MAX_WIREGUARD_PACKET_SIZE + kUtunHeaderSize];
  ssize_t n = HANDLE_EINTR(::read(fd_, raw, sizeof(raw)));
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return 0;
    }
    PLOG(ERROR) << "utun read error";
    return -1;
  }
  if (static_cast<size_t>(n) <= kUtunHeaderSize) {
    return 0;
  }
  const size_t ip_len = static_cast<size_t>(n) - kUtunHeaderSize;
  if (ip_len > buf_size) {
    LOG(ERROR) << "utun read: packet (" << ip_len << " B) exceeds buffer";
    return -1;
  }
  ::memcpy(buf, raw + kUtunHeaderSize, ip_len);
  return static_cast<int>(ip_len);
}

int MacTunInterface::Write(const uint8_t* buf, size_t size) {
  if (size < 1) {
    return 0;
  }
  const uint8_t ip_version = (buf[0] >> 4) & 0x0F;
  uint32_t af_header;
  if (ip_version == 4) {
    af_header = htonl(AF_INET);
  } else if (ip_version == 6) {
    af_header = htonl(AF_INET6);
  } else {
    LOG(ERROR) << "utun write: unknown IP version "
               << static_cast<int>(ip_version);
    return -1;
  }

  std::vector<uint8_t> pkt(kUtunHeaderSize + size);
  ::memcpy(pkt.data(), &af_header, kUtunHeaderSize);
  ::memcpy(pkt.data() + kUtunHeaderSize, buf, size);

  ssize_t n = HANDLE_EINTR(::write(fd_, pkt.data(), pkt.size()));
  if (n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // The OS tunnel queue is full.
      // Drop the packet and let the higher-level protocol (like TCP) handle it.
      VLOG(1) << "TUN write buffer full, dropping packet";
      return 0;
    }
    PLOG(ERROR) << "utun write error";
    return -1;
  }
  return static_cast<int>(n) - static_cast<int>(kUtunHeaderSize);
}
std::string MacTunInterface::GetName() const {
  return name_;
}

void MacTunInterface::WatchReadable(base::RepeatingClosure on_readable) {
  watcher_ =
      base::FileDescriptorWatcher::WatchReadable(fd_, std::move(on_readable));
}

void MacTunInterface::StopWatching() {
  watcher_.reset();
}

std::unique_ptr<TunInterface> CreateTunInterface() {
  return std::make_unique<MacTunInterface>();
}

}  // namespace wireguard
