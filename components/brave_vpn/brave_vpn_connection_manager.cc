/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager.h"

namespace brave_vpn {

BraveVPNConnectionManager::BraveVPNConnectionManager() = default;
BraveVPNConnectionManager::~BraveVPNConnectionManager() = default;

void BraveVPNConnectionManager::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNConnectionManager::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_vpn
