#ifndef WIREGUARD_TUN_INTERFACE_H_
#define WIREGUARD_TUN_INTERFACE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"

namespace wireguard {

class TunInterface {
 public:
  virtual ~TunInterface() = default;

  virtual bool Open() = 0;
  virtual bool Configure(const std::string& local_ip, int mtu) = 0;
  virtual bool AddRoutes(const std::vector<std::string>& allowed_ips,
                         const std::string& endpoint_ip) = 0;
  virtual bool SetDns(const std::vector<std::string>& servers) = 0;
  virtual void RemoveRoutes() = 0;
  virtual void Close() = 0;

  virtual int Read(uint8_t* buf, size_t buf_size) = 0;
  virtual int Write(const uint8_t* buf, size_t size) = 0;

  virtual std::string GetName() const = 0;

  // Register a callback invoked on the calling thread's message loop whenever
  // the interface becomes readable. Must be called on a thread with an IO
  // message pump. Replaces any prior watcher.
  virtual void WatchReadable(base::RepeatingClosure on_readable) = 0;
  virtual void StopWatching() = 0;
};

std::unique_ptr<TunInterface> CreateTunInterface();

}  // namespace wireguard

#endif  // WIREGUARD_TUN_INTERFACE_H_
