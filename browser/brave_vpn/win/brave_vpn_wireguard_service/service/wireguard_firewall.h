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
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"

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
//
// Thread-safe: the two phases are driven from different threads and can
// overlap, so the public methods serialize access to the filtering engine
// themselves.
class ScopedWireguardFirewall {
 public:
  // Phase one: everything that does not depend on the tunnel adapter, block-all
  // included. Call before starting the tunnel. Returns nullptr if any part of
  // it failed; callers must treat that as fatal.
  //
  // This installs a temporary DNS permit locked to the DNS Client service
  // (svchost.exe). tunnel.dll has to resolve the endpoint hostname, which
  // happens outside our own process. PermitTunnelInterface() withdraws this
  // allowance once the tunnel is up.
  static std::unique_ptr<ScopedWireguardFirewall> Create();

  ScopedWireguardFirewall(const ScopedWireguardFirewall&) = delete;
  ScopedWireguardFirewall& operator=(const ScopedWireguardFirewall&) = delete;
  ~ScopedWireguardFirewall();

  // Phase two, once the adapter exists: permits traffic on it, blocks DNS that
  // would go anywhere else, and drops the temporary DNS allowance from phase
  // one. Until this runs the tunnel carries nothing, which is the safe
  // direction to fail. Returns false if the policy could not be completed.
  //
  // Runs on the interface-change thread while the caller is inside tunnel.dll.
  // WithdrawTemporaryDns() may run concurrently on the firewall watchdog
  // thread, so this serializes against it internally.
  bool PermitTunnelInterface(const NET_LUID& tunnel_luid);

  // Withdraws the temporary DNS allowance granted during phase one.
  // Called automatically by PermitTunnelInterface(), but must be called
  // manually on failure paths to prevent DNS leaks if the process survives.
  // Returns false if the allowance could not be confirmed withdrawn, in
  // which case it may still be active.
  bool WithdrawTemporaryDns();

 private:
  ScopedWireguardFirewall(HANDLE engine,
                          const GUID& provider_key,
                          const GUID& sublayer_key,
                          std::vector<UINT64> temporary_dns_filter_ids);

  // The work behind WithdrawTemporaryDns(), for callers that already hold the
  // lock. Deletes the filters but leaves `temporary_dns_filter_ids_` alone:
  // PermitTunnelInterface() runs this inside a transaction, and an abort would
  // restore the filters, so the ids may not be dropped until it commits.
  bool WithdrawTemporaryDnsLocked() EXCLUSIVE_LOCKS_REQUIRED(lock_);

  // Serializes the filter engine work between the interface-change thread and
  // the watchdog thread. `engine_` itself is never reassigned after
  // construction; what needs serializing is the operations run through it,
  // because a WFP transaction belongs to the session rather than the thread
  // that opened it.
  base::Lock lock_;
  HANDLE engine_ = nullptr;
  const GUID provider_key_;
  const GUID sublayer_key_;
  std::vector<UINT64> temporary_dns_filter_ids_ GUARDED_BY(lock_);
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
// tear the tunnel down and return from WireGuardTunnelService().
// Returns true if the stop signal was successfully sent to the SCM.
bool RequestTunnelShutdown();

}  // namespace brave_vpn::wireguard

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_FIREWALL_H_
