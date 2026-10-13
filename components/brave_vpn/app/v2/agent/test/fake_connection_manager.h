/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_MANAGER_H_

#include "brave/components/brave_vpn/app/v2/agent/connection_manager.h"

namespace brave_vpn::v2 {

// ConnectionManager for tests: its operations do nothing, and tests can drive
// state changes directly with SetConnectionState().
class FakeConnectionManager : public ConnectionManager {
 public:
  FakeConnectionManager();
  ~FakeConnectionManager() override;

  FakeConnectionManager(const FakeConnectionManager&) = delete;
  FakeConnectionManager& operator=(const FakeConnectionManager&) = delete;

  // ConnectionManager overrides:
  void Connect() override;
  void Disconnect() override;
  void ResetConnectionState() override;

  // Lets tests check that observers registered and unregistered.
  using ConnectionManager::HasObservers;

  // Lets tests simulate the VPN changing state.
  using ConnectionManager::SetConnectionState;

  int connect_calls() const { return connect_calls_; }
  int disconnect_calls() const { return disconnect_calls_; }
  int reset_connection_state_calls() const {
    return reset_connection_state_calls_;
  }

 private:
  int connect_calls_ = 0;
  int disconnect_calls_ = 0;
  int reset_connection_state_calls_ = 0;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_MANAGER_H_
