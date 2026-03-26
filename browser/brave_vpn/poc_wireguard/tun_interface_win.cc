#ifdef UNSAFE_BUFFERS_BUILD
// Interfaces directly with kernel control sockets and utun, requiring raw
// buffer operations. TODO: Migrate to base::span when productionising.
#pragma allow_unsafe_buffers
#endif

#include "brave/browser/brave_vpn/poc_wireguard/tun_interface_win.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <iphlpapi.h>
#include <netioapi.h>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"

namespace wireguard {

namespace {

// 4 MiB ring buffer - Wintun's documented minimum useful capacity.
constexpr DWORD kSessionCapacity = 0x400000;

constexpr wchar_t kAdapterName[] = L"BraveVPN";
constexpr wchar_t kTunnelType[] = L"WireGuard";

// Fixed GUID so the adapter survives a helper crash and can be reopened
// rather than leaving an orphaned device in Device Manager.
// {A5C5E3A5-6F1D-4B7E-8D3A-1C2B4E5F6A7B}
const GUID kAdapterGuid = {0xa5c5e3a5,
                           0x6f1d,
                           0x4b7e,
                           {0x8d, 0x3a, 0x1c, 0x2b, 0x4e, 0x5f, 0x6a, 0x7b}};

void CALLBACK WintunLogCallback(WINTUN_LOGGER_LEVEL level,
                                DWORD64 /*timestamp*/,
                                LPCWSTR message) {
  switch (level) {
    case WINTUN_LOG_INFO:
      LOG(INFO) << "[wintun] " << message;
      break;
    case WINTUN_LOG_WARN:
      LOG(WARNING) << "[wintun] " << message;
      break;
    case WINTUN_LOG_ERR:
      LOG(ERROR) << "[wintun] " << message;
      break;
  }
}

// Parses "a.b.c.d/n" into a network-byte-order address and prefix length.
bool ParseCidr(const std::string& cidr, uint32_t* addr, uint8_t* prefix_len) {
  const auto slash = cidr.find('/');
  if (slash == std::string::npos) {
    return false;
  }
  IN_ADDR in;
  if (::inet_pton(AF_INET, cidr.substr(0, slash).c_str(), &in) != 1) {
    return false;
  }
  const int len = std::stoi(cidr.substr(slash + 1));
  if (len < 0 || len > 32) {
    return false;
  }
  *addr = in.s_addr;
  *prefix_len = static_cast<uint8_t>(len);
  return true;
}

}  // namespace

// ── Construction / destruction
// ────────────────────────────────────────────────

WinTunInterface::WinTunInterface() {
  // Locate and load wintun.dll from the same directory as the executable,
  // mirroring how the boringtun DLL is loaded in main.cc.
  base::FilePath exe_dir;
  base::PathService::Get(base::DIR_EXE, &exe_dir);
  const std::string lib_path =
      exe_dir.AppendASCII(WintunLibraryName()).AsUTF8Unsafe();

  wintun_ = CreateWintunLoader();
  if (!wintun_->Load(lib_path)) {
    LOG(ERROR) << "Failed to load Wintun from: " << lib_path;
  }
}

WinTunInterface::~WinTunInterface() {
  Close();
}

// ── TunInterface ─────────────────────────────────────────────────────────────

bool WinTunInterface::Open() {
  if (!wintun_->is_loaded()) {
    LOG(ERROR) << "Wintun not loaded";
    return false;
  }

  wintun_->WintunSetLogger(WintunLogCallback);

  const DWORD ver = wintun_->WintunGetRunningDriverVersion();
  if (ver != 0) {
    LOG(INFO) << "Wintun driver v" << (ver >> 16) << "." << (ver & 0xFFFF);
  }

  // WintunCreateAdapter with a fixed GUID reuses the existing adapter if it
  // already exists, avoiding orphaned devices after an unclean shutdown.
  adapter_ =
      wintun_->WintunCreateAdapter(kAdapterName, kTunnelType, &kAdapterGuid);
  if (!adapter_) {
    LOG(ERROR) << "WintunCreateAdapter failed: " << ::GetLastError();
    return false;
  }

  NET_LUID luid;
  wintun_->WintunGetAdapterLUID(adapter_, &luid);
  tun_luid_ = luid.Value;

  session_ = wintun_->WintunStartSession(adapter_, kSessionCapacity);
  if (!session_) {
    LOG(ERROR) << "WintunStartSession failed: " << ::GetLastError();
    wintun_->WintunCloseAdapter(adapter_);
    adapter_ = nullptr;
    return false;
  }

  LOG(INFO) << "Opened Wintun adapter: BraveVPN";
  return true;
}

bool WinTunInterface::Configure(const std::string& local_ip, int mtu) {
  DCHECK(adapter_);

  NET_LUID luid;
  luid.Value = tun_luid_;

  // Assign the tunnel IP address to the interface.
  MIB_UNICASTIPADDRESS_ROW addr_row;
  ::InitializeUnicastIpAddressEntry(&addr_row);
  addr_row.InterfaceLuid = luid;
  addr_row.Address.Ipv4.sin_family = AF_INET;
  if (::inet_pton(AF_INET, local_ip.c_str(), &addr_row.Address.Ipv4.sin_addr) !=
      1) {
    LOG(ERROR) << "Configure: invalid local IP: " << local_ip;
    return false;
  }
  addr_row.OnLinkPrefixLength = 32;  // point-to-point
  addr_row.DadState = IpDadStatePreferred;

  const DWORD addr_err = ::CreateUnicastIpAddressEntry(&addr_row);
  if (addr_err != NO_ERROR && addr_err != ERROR_OBJECT_ALREADY_EXISTS) {
    LOG(ERROR) << "CreateUnicastIpAddressEntry failed: " << addr_err;
    return false;
  }

  // Set the MTU on the interface.
  MIB_IPINTERFACE_ROW iface_row;
  ::InitializeIpInterfaceEntry(&iface_row);
  iface_row.InterfaceLuid = luid;
  iface_row.Family = AF_INET;
  DWORD err = ::GetIpInterfaceEntry(&iface_row);
  if (err != NO_ERROR) {
    LOG(ERROR) << "GetIpInterfaceEntry failed: " << err;
    return false;
  }
  iface_row.NlMtu = static_cast<ULONG>(mtu);
  iface_row.SitePrefixLength = 0;  // required field when calling Set
  err = ::SetIpInterfaceEntry(&iface_row);
  if (err != NO_ERROR) {
    LOG(ERROR) << "SetIpInterfaceEntry (MTU) failed: " << err;
    return false;
  }

  LOG(INFO) << "Configured BraveVPN: ip=" << local_ip << " mtu=" << mtu;
  return true;
}

bool WinTunInterface::FindDefaultGateway() {
  // Ask the kernel for the best route to 0.0.0.0 - that gives us both the
  // current default gateway address and the LUID of the uplink interface.
  SOCKADDR_INET dest = {};
  dest.Ipv4.sin_family = AF_INET;
  dest.Ipv4.sin_addr.s_addr = 0;  // 0.0.0.0

  MIB_IPFORWARD_ROW2 best_route = {};
  SOCKADDR_INET best_src = {};

  const DWORD err = ::GetBestRoute2(
      /*InterfaceLuid=*/nullptr, /*InterfaceIndex=*/0,
      /*SourceAddress=*/nullptr, &dest, /*AddSortOptions=*/0, &best_route,
      &best_src);
  if (err != NO_ERROR) {
    LOG(ERROR) << "GetBestRoute2 failed: " << err;
    return false;
  }

  original_luid_ = best_route.InterfaceLuid.Value;
  original_gw_ = best_route.NextHop.Ipv4.sin_addr.s_addr;

  char gw_str[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &best_route.NextHop.Ipv4.sin_addr, gw_str,
              sizeof(gw_str));
  LOG(INFO) << "Default gateway: " << gw_str;
  return true;
}

bool WinTunInterface::AddRoute(uint32_t dest,
                               uint8_t prefix_len,
                               bool via_tunnel) {
  NET_LUID luid;
  luid.Value = via_tunnel ? tun_luid_ : original_luid_;

  MIB_IPFORWARD_ROW2 row;
  ::InitializeIpForwardEntry(&row);
  row.InterfaceLuid = luid;
  row.DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
  row.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = dest;
  row.DestinationPrefix.PrefixLength = prefix_len;
  row.NextHop.Ipv4.sin_family = AF_INET;
  // Tunnel routes are on-link (next hop 0.0.0.0).
  // Gateway routes use the saved original gateway address.
  row.NextHop.Ipv4.sin_addr.s_addr = via_tunnel ? 0 : original_gw_;
  row.Metric = 0;  // auto
  row.Protocol = MIB_IPPROTO_NETMGMT;

  const DWORD err = ::CreateIpForwardEntry2(&row);
  if (err != NO_ERROR && err != ERROR_OBJECT_ALREADY_EXISTS) {
    char dst[INET_ADDRSTRLEN];
    IN_ADDR a;
    a.s_addr = dest;
    ::inet_ntop(AF_INET, &a, dst, sizeof(dst));
    LOG(ERROR) << "CreateIpForwardEntry2 " << dst << "/"
               << static_cast<int>(prefix_len) << " failed: " << err;
    return false;
  }
  return true;
}

bool WinTunInterface::DeleteRoute(uint32_t dest,
                                  uint8_t prefix_len,
                                  bool via_tunnel) {
  NET_LUID luid;
  luid.Value = via_tunnel ? tun_luid_ : original_luid_;

  MIB_IPFORWARD_ROW2 row;
  ::InitializeIpForwardEntry(&row);
  row.InterfaceLuid = luid;
  row.DestinationPrefix.Prefix.Ipv4.sin_family = AF_INET;
  row.DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr = dest;
  row.DestinationPrefix.PrefixLength = prefix_len;
  row.NextHop.Ipv4.sin_family = AF_INET;
  row.NextHop.Ipv4.sin_addr.s_addr = via_tunnel ? 0 : original_gw_;

  const DWORD err = ::DeleteIpForwardEntry2(&row);
  if (err != NO_ERROR && err != ERROR_NOT_FOUND) {
    LOG(ERROR) << "DeleteIpForwardEntry2 failed: " << err;
    return false;
  }
  return true;
}

bool WinTunInterface::AddRoutes(const std::vector<std::string>& allowed_ips,
                                const std::string& endpoint_ip) {
  if (!FindDefaultGateway()) {
    return false;
  }

  // Protect the WireGuard endpoint - encrypted UDP must reach the peer via
  // the original gateway, not be re-encrypted back into the tunnel.
  IN_ADDR ep;
  if (::inet_pton(AF_INET, endpoint_ip.c_str(), &ep) != 1) {
    LOG(ERROR) << "AddRoutes: invalid endpoint IP: " << endpoint_ip;
    return false;
  }
  if (!AddRoute(ep.s_addr, 32, /*via_tunnel=*/false)) {
    return false;
  }
  added_routes_.push_back({ep.s_addr, 32, false});

  for (const auto& cidr : allowed_ips) {
    uint32_t dest;
    uint8_t prefix_len;
    if (!ParseCidr(cidr, &dest, &prefix_len)) {
      LOG(ERROR) << "AddRoutes: cannot parse CIDR: " << cidr;
      return false;
    }

    if (cidr == "0.0.0.0/0") {
      // Split 0/0 into two /1s so the default route entry is not replaced.
      IN_ADDR a;
      ::inet_pton(AF_INET, "0.0.0.0", &a);
      if (!AddRoute(a.s_addr, 1, /*via_tunnel=*/true)) {
        return false;
      }
      added_routes_.push_back({a.s_addr, 1, true});

      ::inet_pton(AF_INET, "128.0.0.0", &a);
      if (!AddRoute(a.s_addr, 1, /*via_tunnel=*/true)) {
        return false;
      }
      added_routes_.push_back({a.s_addr, 1, true});
    } else {
      if (!AddRoute(dest, prefix_len, /*via_tunnel=*/true)) {
        return false;
      }
      added_routes_.push_back({dest, prefix_len, true});
    }
  }
  return true;
}

bool WinTunInterface::SetDns(const std::vector<std::string>& servers) {
  if (servers.empty()) {
    return true;
  }

  // Set DNS on the VPN adapter itself. Windows routes DNS queries through
  // whichever interface carries the traffic, so setting DNS here is sufficient
  // when all traffic is tunnelled. The adapter is destroyed on Close(), so
  // there is nothing to restore.
  std::string cmd = "netsh interface ip set dns name=\"BraveVPN\" static " +
                    servers[0] + " validate=no";
  if (::system(cmd.c_str()) != 0) {
    LOG(ERROR) << "SetDns: failed to set primary DNS";
    return false;
  }
  for (size_t i = 1; i < servers.size(); ++i) {
    cmd = "netsh interface ip add dns name=\"BraveVPN\" " + servers[i] +
          " index=" + std::to_string(i + 1) + " validate=no";
    ::system(cmd.c_str());  // best-effort for secondary servers
  }

  LOG(INFO) << "Set DNS on BraveVPN: " << servers[0];
  return true;
}

void WinTunInterface::RemoveRoutes() {
  // Remove in reverse insertion order to undo cleanly.
  for (auto it = added_routes_.rbegin(); it != added_routes_.rend(); ++it) {
    DeleteRoute(it->dest, it->prefix_len, it->via_tunnel);
  }
  added_routes_.clear();
  original_luid_ = 0;
  original_gw_ = 0;
}

void WinTunInterface::Close() {
  StopWatching();
  if (session_) {
    wintun_->WintunEndSession(session_);
    session_ = nullptr;
  }
  if (adapter_) {
    // WintunCloseAdapter releases the handle but keeps the adapter installed
    // in Device Manager, avoiding driver reinstallation on the next launch.
    wintun_->WintunCloseAdapter(adapter_);
    adapter_ = nullptr;
  }
  tun_luid_ = 0;
  LOG(INFO) << "Closed Wintun adapter";
}

int WinTunInterface::Read(uint8_t* buf, size_t buf_size) {
  DCHECK(session_);
  DWORD packet_size;
  BYTE* packet = wintun_->WintunReceivePacket(session_, &packet_size);
  if (!packet) {
    if (::GetLastError() == ERROR_NO_MORE_ITEMS) {
      // Ring buffer empty - normal after ObjectWatcher fires and the last
      // packet has already been consumed.
      return 0;
    }
    LOG(ERROR) << "WintunReceivePacket failed: " << ::GetLastError();
    return -1;
  }

  if (packet_size > buf_size) {
    LOG(ERROR) << "Wintun packet (" << packet_size << " B) exceeds buffer";
    wintun_->WintunReleaseReceivePacket(session_, packet);
    return -1;
  }

  ::memcpy(buf, packet, packet_size);
  wintun_->WintunReleaseReceivePacket(session_, packet);
  return static_cast<int>(packet_size);
}

int WinTunInterface::Write(const uint8_t* buf, size_t size) {
  DCHECK(session_);
  BYTE* packet =
      wintun_->WintunAllocateSendPacket(session_, static_cast<DWORD>(size));
  if (!packet) {
    LOG(ERROR) << "WintunAllocateSendPacket failed: " << ::GetLastError();
    return -1;
  }
  ::memcpy(packet, buf, size);
  wintun_->WintunSendPacket(session_, packet);
  return static_cast<int>(size);
}

std::string WinTunInterface::GetName() const {
  return "BraveVPN";
}

void WinTunInterface::WatchReadable(base::RepeatingClosure on_readable) {
  DCHECK(session_);
  on_readable_ = std::move(on_readable);
  // WintunGetReadWaitEvent returns a handle owned by the session - we must
  // not call CloseHandle on it.
  read_event_ = wintun_->WintunGetReadWaitEvent(session_);
  // StartWatchingMultipleTimes re-arms after each signal so we keep receiving
  // callbacks as long as the session is active.
  watcher_.StartWatchingMultipleTimes(read_event_, this);
}

void WinTunInterface::StopWatching() {
  watcher_.StopWatching();
  read_event_ = nullptr;  // not owned - do not CloseHandle
  on_readable_.Reset();
}

void WinTunInterface::OnObjectSignaled(HANDLE /*object*/) {
  // Wintun resets the event automatically when the ring buffer becomes empty,
  // so we don't need to reset it manually here (unlike the UDP case with
  // WSAResetEvent). Just invoke the callback and let WireGuardTunnel drain
  // via Read().
  if (on_readable_) {
    on_readable_.Run();
  }
}

std::unique_ptr<TunInterface> CreateTunInterface() {
  return std::make_unique<WinTunInterface>();
}

}  // namespace wireguard
