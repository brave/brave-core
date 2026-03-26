#ifndef WIREGUARD_TUN_INTERFACE_LINUX_H_
#define WIREGUARD_TUN_INTERFACE_LINUX_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_descriptor_watcher_posix.h"
#include "brave/browser/brave_vpn/poc_wireguard/tun_interface.h"

namespace wireguard {

enum class DnsManager { kUnknown, kResolvectl, kResolvconf, kResolveDotConf };

class LinuxTunInterface final : public TunInterface {
 public:
  LinuxTunInterface();
  ~LinuxTunInterface() override;

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

 private:
  bool RunIp(const std::string& args);
  void RestoreDns();

  int fd_ = -1;
  std::string name_;

  // Routing state
  std::string original_gateway_;
  std::string endpoint_route_;
  std::vector<std::string> added_routes_;

  // DNS state - we detect which resolver manager is active and use it.
  DnsManager dns_manager_ = DnsManager::kUnknown;
  std::string original_dns_;  // saved resolv.conf content (fallback path)

  // Watcher - valid only between WatchReadable() and StopWatching().
  std::unique_ptr<base::FileDescriptorWatcher::Controller> watcher_;
};

}  // namespace wireguard

#endif  // WIREGUARD_TUN_INTERFACE_LINUX_H_
