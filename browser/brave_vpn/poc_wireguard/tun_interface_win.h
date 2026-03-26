#ifndef WIREGUARD_TUN_INTERFACE_WIN_H_
#define WIREGUARD_TUN_INTERFACE_WIN_H_

#include <windows.h>
#include <winsock2.h>

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/win/object_watcher.h"
#include "brave/browser/brave_vpn/poc_wireguard/tun_interface.h"
#include "brave/browser/brave_vpn/poc_wireguard/wintun_loader.h"

namespace wireguard {

class WinTunInterface final : public TunInterface,
                              public base::win::ObjectWatcher::Delegate {
 public:
  WinTunInterface();
  ~WinTunInterface() override;

  // TunInterface:
  bool Open() override;
  bool Configure(const std::string& local_ip, int mtu) override;
  bool AddRoutes(const std::vector<std::string>& allowed_ips,
                 const std::string& endpoint_ip) override;
  bool SetDns(const std::vector<std::string>& servers) override;
  void RemoveRoutes() override;
  void Close() override;
  int Read(uint8_t* buf, size_t buf_size) override;
  int Write(const uint8_t* buf, size_t size) override;
  std::string GetName() const override;
  void WatchReadable(base::RepeatingClosure on_readable) override;
  void StopWatching() override;

  // base::win::ObjectWatcher::Delegate:
  void OnObjectSignaled(HANDLE object) override;

 private:
  // Locates the default gateway and the LUID of the interface it is on.
  // Must be called before AddRoute().
  bool FindDefaultGateway();

  // Adds/removes a single route. dest and prefix_len describe the destination
  // prefix. via_tunnel=true routes via the Wintun adapter; false routes via
  // the original gateway.
  bool AddRoute(uint32_t dest, uint8_t prefix_len, bool via_tunnel);
  bool DeleteRoute(uint32_t dest, uint8_t prefix_len, bool via_tunnel);

  // Lightweight record of every route we installed so RemoveRoutes() can
  // tear them all down without re-computing anything.
  struct AddedRoute {
    uint32_t dest;  // network byte order
    uint8_t prefix_len;
    bool via_tunnel;
  };

  std::unique_ptr<WintunLoader> wintun_;
  WINTUN_ADAPTER_HANDLE adapter_ = nullptr;
  WINTUN_SESSION_HANDLE session_ = nullptr;

  // Stored as UINT64 (== NET_LUID.Value) so this header doesn't need to pull
  // in <netioapi.h>. Cast back to NET_LUID in the .cc.
  uint64_t tun_luid_ = 0;
  uint64_t original_luid_ = 0;
  uint32_t original_gw_ = 0;  // network byte order

  std::vector<AddedRoute> added_routes_;

  // WatchReadable state.
  // read_event_ is owned by the session - do NOT call CloseHandle on it.
  HANDLE read_event_ = nullptr;
  base::win::ObjectWatcher watcher_;
  base::RepeatingClosure on_readable_;
};

}  // namespace wireguard

#endif  // WIREGUARD_TUN_INTERFACE_WIN_H_
