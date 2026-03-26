#ifndef WIREGUARD_UDP_TRANSPORT_WIN_H_
#define WIREGUARD_UDP_TRANSPORT_WIN_H_

#include <winsock2.h>
#include <ws2tcpip.h>

#include "base/functional/callback.h"
#include "base/win/object_watcher.h"
#include "brave/browser/brave_vpn/poc_wireguard/udp_transport.h"

namespace wireguard {

class WinUdpTransport final : public UdpTransport,
                              public base::win::ObjectWatcher::Delegate {
 public:
  WinUdpTransport();
  ~WinUdpTransport() override;

  // UdpTransport:
  bool Open(const std::string& peer_ip,
            uint16_t peer_port,
            uint16_t local_port) override;
  void Close() override;
  int Send(const uint8_t* buf, size_t size) override;
  int Recv(uint8_t* buf, size_t buf_size) override;
  void WatchReadable(base::RepeatingClosure on_readable) override;
  void StopWatching() override;

  // base::win::ObjectWatcher::Delegate:
  void OnObjectSignaled(HANDLE object) override;

 private:
  SOCKET socket_ = INVALID_SOCKET;
  struct sockaddr_in peer_addr_ = {};

  // WSAEventSelect creates a Win32 event that is signaled when FD_READ fires.
  // ObjectWatcher watches the HANDLE and calls OnObjectSignaled on the
  // thread's message loop - the same role FileDescriptorWatcher plays on POSIX.
  WSAEVENT wsa_event_ = WSA_INVALID_EVENT;
  base::win::ObjectWatcher watcher_;
  base::RepeatingClosure on_readable_;
};

}  // namespace wireguard

#endif  // WIREGUARD_UDP_TRANSPORT_WIN_H_
