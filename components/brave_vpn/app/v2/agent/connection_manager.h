/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_

#include <optional>
#include <string>

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

   protected:
    ~Observer() override = default;
  };

  ConnectionManager();
  virtual ~ConnectionManager();

  ConnectionManager(const ConnectionManager&) = delete;
  ConnectionManager& operator=(const ConnectionManager&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  bool HasObservers() const;

  mojom::VpnConnectionState connection_state() const;
  const std::optional<std::string>& error_message() const;

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

 protected:
  // Records the new state and notifies observers. Does nothing if neither the
  // state nor the error message changed.
  void SetConnectionState(mojom::VpnConnectionState state,
                          std::optional<std::string> error_message);

 private:
  SEQUENCE_CHECKER(sequence_checker_);
  base::ObserverList<Observer> observers_;
  mojom::VpnConnectionState connection_state_ =
      mojom::VpnConnectionState::kDisconnected;
  std::optional<std::string> error_message_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_MANAGER_H_
