/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/test/fake_connection_manager.h"

namespace brave_vpn::v2 {

FakeConnectionManager::FakeConnectionManager() = default;

FakeConnectionManager::~FakeConnectionManager() = default;

void FakeConnectionManager::Connect() {
  ++connect_calls_;
}

void FakeConnectionManager::Disconnect() {
  ++disconnect_calls_;
}

void FakeConnectionManager::ResetConnectionState() {
  ++reset_connection_state_calls_;
}

}  // namespace brave_vpn::v2
