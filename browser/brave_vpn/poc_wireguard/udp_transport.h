#ifndef WIREGUARD_UDP_TRANSPORT_H_
#define WIREGUARD_UDP_TRANSPORT_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/functional/callback.h"

namespace wireguard {

class UdpTransport {
 public:
  virtual ~UdpTransport() = default;

  virtual bool Open(const std::string& peer_ip,
                    uint16_t peer_port,
                    uint16_t local_port = 0) = 0;
  virtual void Close() = 0;
  virtual int Send(const uint8_t* buf, size_t size) = 0;
  virtual int Recv(uint8_t* buf, size_t buf_size) = 0;

  // Register a callback invoked on the calling thread's message loop whenever
  // the socket becomes readable. Must be called on a thread with an IO message
  // pump. Replaces any prior watcher.
  virtual void WatchReadable(base::RepeatingClosure on_readable) = 0;
  virtual void StopWatching() = 0;
};

std::unique_ptr<UdpTransport> CreateUdpTransport();

}  // namespace wireguard

#endif  // WIREGUARD_UDP_TRANSPORT_H_
