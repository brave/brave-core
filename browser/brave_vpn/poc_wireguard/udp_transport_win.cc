#include "brave/browser/brave_vpn/poc_wireguard/udp_transport_win.h"

#include "base/logging.h"
#include "base/no_destructor.h"

namespace wireguard {

namespace {

// Winsock is process-global and ref-counted. We initialise it once when the
// first transport is created and clean up when it is destroyed.
// In the privileged helper there will only ever be one transport, so a simple
// RAII guard is sufficient.
struct WsaInit {
  WsaInit() {
    WSADATA wsa_data;
    const int err = ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (err != 0) {
      LOG(ERROR) << "WSAStartup failed: " << err;
      ok = false;
    }
  }
  ~WsaInit() {
    if (ok) {
      ::WSACleanup();
    }
  }
  bool ok = true;
};

}  // namespace

WinUdpTransport::WinUdpTransport() {
  // Initialise Winsock for this process.  Safe to construct multiple times -
  // WSAStartup is ref-counted by the OS, and WSACleanup in ~WsaInit decrements
  // it correctly.
  static base::NoDestructor<WsaInit> wsa_init;
}

WinUdpTransport::~WinUdpTransport() {
  Close();
}

bool WinUdpTransport::Open(const std::string& peer_ip,
                           uint16_t peer_port,
                           uint16_t local_port) {
  socket_ =
      ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
                   /*lpProtocolInfo=*/nullptr, /*g=*/0, WSA_FLAG_OVERLAPPED);
  if (socket_ == INVALID_SOCKET) {
    LOG(ERROR) << "UDP: WSASocketW failed: " << ::WSAGetLastError();
    return false;
  }

  if (local_port > 0) {
    struct sockaddr_in local = {};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(local_port);
    if (::bind(socket_, reinterpret_cast<struct sockaddr*>(&local),
               sizeof(local)) == SOCKET_ERROR) {
      LOG(ERROR) << "UDP: bind() to port " << local_port
                 << " failed: " << ::WSAGetLastError();
      Close();
      return false;
    }
  }

  peer_addr_.sin_family = AF_INET;
  peer_addr_.sin_port = htons(peer_port);
  if (::inet_pton(AF_INET, peer_ip.c_str(), &peer_addr_.sin_addr) != 1) {
    LOG(ERROR) << "UDP: invalid peer IP: " << peer_ip;
    Close();
    return false;
  }

  LOG(INFO) << "UDP transport open -> " << peer_ip << ":" << peer_port;
  return true;
}

void WinUdpTransport::Close() {
  StopWatching();
  if (socket_ != INVALID_SOCKET) {
    ::closesocket(socket_);
    socket_ = INVALID_SOCKET;
  }
}

int WinUdpTransport::Send(const uint8_t* buf, size_t size) {
  const int n = ::sendto(
      socket_, reinterpret_cast<const char*>(buf), static_cast<int>(size),
      /*flags=*/0, reinterpret_cast<const struct sockaddr*>(&peer_addr_),
      sizeof(peer_addr_));
  if (n == SOCKET_ERROR) {
    LOG(ERROR) << "UDP sendto error: " << ::WSAGetLastError();
  }
  return n;
}

int WinUdpTransport::Recv(uint8_t* buf, size_t buf_size) {
  const int n = ::recvfrom(socket_, reinterpret_cast<char*>(buf),
                           static_cast<int>(buf_size), /*flags=*/0,
                           /*from=*/nullptr, /*fromlen=*/nullptr);
  if (n == SOCKET_ERROR) {
    const int err = ::WSAGetLastError();
    if (err == WSAEWOULDBLOCK) {
      // Normal after WSAEventSelect — the event fired but the datagram was
      // already consumed by a prior call. Not an error.
      return 0;
    }
    LOG(ERROR) << "UDP recvfrom error: " << err;
    return -1;
  }
  return n;
}

void WinUdpTransport::WatchReadable(base::RepeatingClosure on_readable) {
  DCHECK(socket_ != INVALID_SOCKET);
  on_readable_ = std::move(on_readable);

  wsa_event_ = ::WSACreateEvent();
  if (wsa_event_ == WSA_INVALID_EVENT) {
    LOG(ERROR) << "WSACreateEvent failed: " << ::WSAGetLastError();
    return;
  }

  // Associate the event with FD_READ on the socket. From this point any
  // datagram arriving on the socket will signal wsa_event_.
  if (::WSAEventSelect(socket_, wsa_event_, FD_READ) == SOCKET_ERROR) {
    LOG(ERROR) << "WSAEventSelect failed: " << ::WSAGetLastError();
    ::WSACloseEvent(wsa_event_);
    wsa_event_ = WSA_INVALID_EVENT;
    return;
  }

  // ObjectWatcher wraps a Win32 HANDLE and fires the delegate callback on the
  // current thread's message loop each time the handle is signaled -
  // equivalent to FileDescriptorWatcher::WatchReadable on POSIX.
  // StartWatchingMultipleTimes keeps re-arming after each signal.
  watcher_.StartWatchingMultipleTimes(wsa_event_, this);
}

void WinUdpTransport::StopWatching() {
  watcher_.StopWatching();
  if (wsa_event_ != WSA_INVALID_EVENT) {
    // Dissociate the event from the socket before closing it.
    if (socket_ != INVALID_SOCKET) {
      ::WSAEventSelect(socket_, wsa_event_, 0);
    }
    ::WSACloseEvent(wsa_event_);
    wsa_event_ = WSA_INVALID_EVENT;
  }
  on_readable_.Reset();
}

void WinUdpTransport::OnObjectSignaled(HANDLE /*object*/) {
  // WSAResetEvent must be called before reading - if we reset after reading,
  // a datagram that arrives in the window between read and reset would be
  // missed. Resetting first is safe: if no datagram is available, Recv()
  // returns WSAEWOULDBLOCK and the next arrival re-signals the event.
  ::WSAResetEvent(wsa_event_);

  if (on_readable_) {
    on_readable_.Run();
  }
}

std::unique_ptr<UdpTransport> CreateUdpTransport() {
  return std::make_unique<WinUdpTransport>();
}

}  // namespace wireguard
