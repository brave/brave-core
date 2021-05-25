/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager_win.h"

#include "base/notreached.h"

namespace brave_vpn {

// static
BraveVPNConnectionManager* BraveVPNConnectionManager::GetInstance() {
  static base::NoDestructor<BraveVPNConnectionManagerWin> s_manager;
  return s_manager.get();
}

BraveVPNConnectionManagerWin::BraveVPNConnectionManagerWin() = default;
BraveVPNConnectionManagerWin::~BraveVPNConnectionManagerWin() = default;

bool BraveVPNConnectionManagerWin::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::Connect(const std::string& name) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::Disconnect(const std::string& name) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::RemoveVPNConnection(
    const std::string& name) {
  NOTIMPLEMENTED();
  return true;
}

}  // namespace brave_vpn
