#ifndef WIREGUARD_BORINGTUN_LOADER_H_
#define WIREGUARD_BORINGTUN_LOADER_H_

#include <memory>
#include <string>

#include "brave/browser/brave_vpn/poc_wireguard/boringtun_api.h"

namespace wireguard {

class BoringtunLoader {
 public:
  BoringtunLoader();
  virtual ~BoringtunLoader() = default;

  virtual bool Load(const std::string& path) = 0;
  virtual void Unload() = 0;

  bool is_loaded() const { return loaded_; }

  SetLoggingFunctionFn set_logging_function = nullptr;
  NewTunnelFn new_tunnel = nullptr;
  TunnelFreeFn tunnel_free = nullptr;
  WireguardWriteFn wireguard_write = nullptr;
  WireguardReadFn wireguard_read = nullptr;
  WireguardTickFn wireguard_tick = nullptr;
  WireguardForceHandshakeFn wireguard_force_handshake = nullptr;
  WireguardStatsFn wireguard_stats = nullptr;

 protected:
  bool loaded_ = false;
};

std::unique_ptr<BoringtunLoader> CreateBoringtunLoader();
const char* BoringtunLibraryName();

}  // namespace wireguard

#endif  // WIREGUARD_BORINGTUN_LOADER_H_
