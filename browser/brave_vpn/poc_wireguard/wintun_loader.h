#ifndef WIREGUARD_WINTUN_LOADER_H_
#define WIREGUARD_WINTUN_LOADER_H_

#include <windows.h>

#include <memory>
#include <string>

#include "brave/third_party/wintun/include/wintun.h"

namespace wireguard {

// Dynamically loads wintun.dll and resolves all exported symbols.
// Mirrors the BoringtunLoader pattern so both loaders are managed the
// same way in the rest of the codebase.
class WintunLoader {
 public:
  WintunLoader();
  ~WintunLoader();

  bool Load(const std::string& path);
  void Unload();
  bool is_loaded() const { return loaded_; }

  WINTUN_CREATE_ADAPTER_FUNC* WintunCreateAdapter = nullptr;
  WINTUN_OPEN_ADAPTER_FUNC* WintunOpenAdapter = nullptr;
  WINTUN_CLOSE_ADAPTER_FUNC* WintunCloseAdapter = nullptr;
  WINTUN_GET_ADAPTER_LUID_FUNC* WintunGetAdapterLUID = nullptr;
  WINTUN_GET_RUNNING_DRIVER_VERSION_FUNC* WintunGetRunningDriverVersion =
      nullptr;
  WINTUN_SET_LOGGER_FUNC* WintunSetLogger = nullptr;
  WINTUN_START_SESSION_FUNC* WintunStartSession = nullptr;
  WINTUN_END_SESSION_FUNC* WintunEndSession = nullptr;
  WINTUN_GET_READ_WAIT_EVENT_FUNC* WintunGetReadWaitEvent = nullptr;
  WINTUN_RECEIVE_PACKET_FUNC* WintunReceivePacket = nullptr;
  WINTUN_RELEASE_RECEIVE_PACKET_FUNC* WintunReleaseReceivePacket = nullptr;
  WINTUN_ALLOCATE_SEND_PACKET_FUNC* WintunAllocateSendPacket = nullptr;
  WINTUN_SEND_PACKET_FUNC* WintunSendPacket = nullptr;

 private:
  template <typename FnPtr>
  bool Resolve(const char* sym, FnPtr& out);

  HMODULE handle_ = nullptr;
  bool loaded_ = false;
};

std::unique_ptr<WintunLoader> CreateWintunLoader();
const char* WintunLibraryName();

}  // namespace wireguard

#endif  // WIREGUARD_WINTUN_LOADER_H_
