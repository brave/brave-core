/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_FIREWALL_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_FIREWALL_H_

// Order matters here: netioapi.h only declares NotifyIpInterfaceChange and
// friends once the winsock address types are visible.
// clang-format off
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
// clang-format on

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"

namespace base {
class FilePath;
}  // namespace base

namespace brave_vpn::wireguard {

// Installs the WFP filters that keep traffic inside the WireGuard tunnel while
// leaving the local network reachable.
//
// tunnel.dll installs an equivalent set of its own, but only when a peer routes
// a literal default route, and its set has no way to permit LAN traffic. Our
// config deliberately splits the default route to opt out of that behavior (see
// components/brave_vpn/common/wireguard/wireguard_utils.cc), so this class owns
// the policy instead.
//
// The filters live in a dynamic WFP session, so they are removed when this
// object is destroyed and when the hosting process exits, and they never
// survive a reboot. That is what keeps a crash from leaving a machine with a
// broken network.
// The policy goes in two phases, because the block-all filter has to be in
// force before the tunnel adapter exists -- otherwise traffic escapes during
// the window between the service starting and the adapter appearing -- but the
// filter that permits the tunnel itself needs the adapter's LUID.
class ScopedWireguardFirewall {
 public:
  // Phase one: everything that does not depend on the tunnel adapter, block-all
  // included. Call before starting the tunnel. Returns nullptr if any part of
  // it failed; callers must treat that as fatal.
  //
  // This deliberately leaves DNS permitted: tunnel.dll still has to resolve the
  // endpoint hostname, and on Windows that resolution comes from the DNS Client
  // service rather than our own process, so no app-based permit would cover it.
  // PermitTunnelInterface() withdraws that allowance.
  static std::unique_ptr<ScopedWireguardFirewall> Create();

  ScopedWireguardFirewall(const ScopedWireguardFirewall&) = delete;
  ScopedWireguardFirewall& operator=(const ScopedWireguardFirewall&) = delete;
  ~ScopedWireguardFirewall();

  // Phase two, once the adapter exists: permits traffic on it, blocks DNS that
  // would go anywhere else, and drops the temporary DNS allowance from phase
  // one. Until this runs the tunnel carries nothing, which is the safe
  // direction to fail. Returns false if the policy could not be completed.
  //
  // Runs on the interface-change thread while the caller is inside
  // tunnel.dll; nothing else touches the engine until this object is destroyed.
  bool PermitTunnelInterface(const NET_LUID& tunnel_luid);

  // Withdraws the temporary DNS allowance granted during phase one.
  // Called automatically on success by PermitTunnelInterface(), but
  // must be called manually on failure paths to prevent DNS leaks if the
  // process survives.
  void WithdrawTemporaryDns();

 private:
  ScopedWireguardFirewall(HANDLE engine,
                          const GUID& provider_key,
                          const GUID& sublayer_key,
                          std::vector<UINT64> temporary_dns_filter_ids);

  HANDLE engine_ = nullptr;
  const GUID provider_key_;
  const GUID sublayer_key_;
  std::vector<UINT64> temporary_dns_filter_ids_;
};

// Watches for the arrival of the tunnel adapter that tunnel.dll creates and
// runs `on_ready` with its LUID. tunnel.dll names the adapter after the base
// name of the config file it is given, which we generate ourselves.
//
// `on_ready` runs on an OS worker thread, at most once. Destroying the watcher
// cancels the subscription.
class TunnelInterfaceWatcher {
 public:
  using ReadyCallback = base::OnceCallback<void(const NET_LUID&)>;

  // `adapter_name` is the interface alias to wait for. Returns nullptr if the
  // change notification could not be registered.
  static std::unique_ptr<TunnelInterfaceWatcher> Create(
      const std::wstring& adapter_name,
      ReadyCallback on_ready);

  TunnelInterfaceWatcher(const TunnelInterfaceWatcher&) = delete;
  TunnelInterfaceWatcher& operator=(const TunnelInterfaceWatcher&) = delete;
  ~TunnelInterfaceWatcher();

 private:
  class State;

  explicit TunnelInterfaceWatcher(std::unique_ptr<State> state);

  std::unique_ptr<State> state_;
};

// Returns the interface alias tunnel.dll will give the adapter for `config`,
// i.e. the config file name without its `.conf` suffix.
std::wstring GetTunnelInterfaceAlias(const base::FilePath& config);

// Asks the SCM to stop the WireGuard tunnel service, which makes tunnel.dll
// tear the tunnel down and return from WireGuardTunnelService(). Used to fail
// closed when the firewall cannot be installed.
void RequestTunnelShutdown();

}  // namespace brave_vpn::wireguard

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_FIREWALL_H_
