/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "brave/components/brave_vpn/common/mojom/browser_agent.mojom.h"

namespace brave_vpn::v2 {

// ConnectionManager owns the agent's VPN connection and tracks its state. There
// is one instance per agent process, shared by every connected browser through
// its BrowserHostImpl, so a call from any browser changes the state all of them
// observe. Must be created, used and destroyed on the main sequence.
class ConnectionManager {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnConnectionStateChanged(
        mojom::VpnConnectionState state,
        std::optional<std::string> error_message) = 0;
    virtual void OnSelectedRegionChanged(const mojom::VpnRegion& region) = 0;
    virtual void OnAutoReconnectStateChanged(bool enabled) = 0;
    virtual void OnSmartProxyRoutingStateChanged(bool enabled) = 0;
    virtual void OnAllowLanTrafficStateChanged(bool available,
                                               bool enabled) = 0;
    virtual void OnLastHostnameChanged(const std::string& hostname) = 0;

   protected:
    ~Observer() override = default;
  };

  using GetAllRegionsCallback =
      base::OnceCallback<void(std::vector<mojom::VpnRegionPtr>)>;

  ConnectionManager();
  virtual ~ConnectionManager();

  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObservers() const;

  mojom::VpnConnectionState connection_state() const;
  const std::optional<std::string>& error_message() const;
  const mojom::VpnRegion& current_region() const;
  bool auto_reconnect_enabled() const;
  bool smart_proxy_routing_enabled() const;
  bool allow_lan_traffic_available() const;
  bool allow_lan_traffic_enabled() const;
  const std::string& last_hostname() const;

  // Starts connecting the VPN; completion is reported asynchronously through
  // observer's OnConnectionStateChanged.
  virtual void Connect();
  // Disconnects the VPN; completion is reported asynchronously through
  // observer's OnConnectionStateChanged.
  virtual void Disconnect();
  // Clears a failure state and its error message, returning to kDisconnected
  // without connecting; completion is reported asynchronously through
  // observer's OnConnectionStateChanged.
  virtual void ResetConnectionState();
  // Gets information about all available regions from an internal cache which
  // is fetched from the VPN provider. |callback| always runs asynchronously,
  // and runs with an empty list if they can't be fetched.
  virtual void GetAllRegions(GetAllRegionsCallback callback);
  // Sets the desired region which is currently selected in the UI.
  virtual void SetSelectedRegion(mojom::VpnRegionPtr region);
  // Sets the desired auto-reconnect state; this is a suggestion for the
  // agent and can be rejected; if the actual state gets changed, an update
  // will be dispatched via the corresponding observer callback.
  virtual void SetAutoReconnectState(bool enable);
  // Sets the desired Smart Proxy routing state; this is a suggestion for the
  // agent and can be rejected; if the actual state gets changed, an update
  // will be dispatched via the corresponding observer callback.
  virtual void SetSmartProxyRoutingState(bool enable);
  // Sets the desired LAN traffic allowance state; this is a suggestion for the
  // agent and can be rejected; if the actual state gets changed, an update
  // will be dispatched via the corresponding observer callback.
  virtual void SetAllowLanTrafficState(bool enable);

 protected:
  // Each records the new value and notifies observers. Does nothing if the
  // value didn't change.
  void UpdateConnectionState(mojom::VpnConnectionState state,
                             std::optional<std::string> error_message);
  void UpdateSelectedRegion(mojom::VpnRegionPtr region);
  void UpdateAutoReconnectState(bool enabled);
  void UpdateSmartProxyRoutingState(bool enabled);
  void UpdateAllowLanTrafficState(bool available, bool enabled);
  void UpdateLastHostname(std::string hostname);

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  base::ObserverList<Observer> observers_;
  mojom::VpnConnectionState connection_state_ =
      mojom::VpnConnectionState::kDisconnected;
  std::optional<std::string> error_message_;
  mojom::VpnRegionPtr current_region_ = mojom::VpnRegion::New();
  bool auto_reconnect_enabled_ = false;
  bool smart_proxy_routing_enabled_ = false;
  bool allow_lan_traffic_available_ = false;
  bool allow_lan_traffic_enabled_ = false;
  std::string last_hostname_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_
