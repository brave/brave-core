// Includes the real boringtun FFI header and adds function-pointer typedefs
// for every exported symbol.  All dlsym casts are done against these types.

#ifndef WIREGUARD_BORINGTUN_API_H_
#define WIREGUARD_BORINGTUN_API_H_

// The canonical header - structs, enums, and inline constants live here.
#include "brave/third_party/boringtun/include/wireguard_ffi.h"

// Function-pointer typedefs
using SetLoggingFunctionFn = bool (*)(void (*log_func)(const char*));

using NewTunnelFn = wireguard_tunnel* (*)(const char* static_private,
                                          const char* server_static_public,
                                          const char* preshared_key,
                                          uint16_t keep_alive,
                                          uint32_t index);
using TunnelFreeFn = void (*)(wireguard_tunnel*);

using WireguardWriteFn = wireguard_result (*)(const wireguard_tunnel* tunnel,
                                              const uint8_t* src,
                                              uint32_t src_size,
                                              uint8_t* dst,
                                              uint32_t dst_size);

using WireguardReadFn = wireguard_result (*)(const wireguard_tunnel* tunnel,
                                             const uint8_t* src,
                                             uint32_t src_size,
                                             uint8_t* dst,
                                             uint32_t dst_size);

using WireguardTickFn = wireguard_result (*)(const wireguard_tunnel* tunnel,
                                             uint8_t* dst,
                                             uint32_t dst_size);

using WireguardForceHandshakeFn =
    wireguard_result (*)(const wireguard_tunnel* tunnel,
                         uint8_t* dst,
                         uint32_t dst_size);

using WireguardStatsFn = stats (*)(const wireguard_tunnel* tunnel);

#endif  // WIREGUARD_BORINGTUN_API_H_
