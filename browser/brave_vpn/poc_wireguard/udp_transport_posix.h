#ifndef WIREGUARD_UDP_TRANSPORT_POSIX_H_
#define WIREGUARD_UDP_TRANSPORT_POSIX_H_

#include <netinet/in.h>

#include <memory>

#include "base/files/file_descriptor_watcher_posix.h"
#include "brave/browser/brave_vpn/poc_wireguard/udp_transport.h"

namespace wireguard {

class PosixUdpTransport final : public UdpTransport {
 public:
  PosixUdpTransport();
  ~PosixUdpTransport() override;

  bool Open(const std::string& peer_ip,
            uint16_t peer_port,
            uint16_t local_port) override;
  void Close() override;
  int Send(const uint8_t* buf, size_t size) override;
  int Recv(uint8_t* buf, size_t buf_size) override;
  void WatchReadable(base::RepeatingClosure on_readable) override;
  void StopWatching() override;

 private:
  int fd_ = -1;
  struct sockaddr_in peer_addr_ = {};

  // Watcher - valid only between WatchReadable() and StopWatching().
  std::unique_ptr<base::FileDescriptorWatcher::Controller> watcher_;
};

}  // namespace wireguard

#endif  // WIREGUARD_UDP_TRANSPORT_POSIX_H_
