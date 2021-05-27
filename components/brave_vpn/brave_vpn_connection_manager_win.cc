/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager_win.h"

#include <windows.h>

#include <MprApi.h>
#include <ipsectypes.h>
#include <ras.h>
#include <raserror.h>
#include <stdio.h>
#include <winerror.h>
#include <winsock.h>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/utils_win.h"

// Most of Windows implementations are based on Brian Clifton
// (brian@clifton.me)'s work (https://github.com/bsclifton/winvpntool).

using brave_vpn::internal::ConnectEntry;
using brave_vpn::internal::CreateEntry;
using brave_vpn::internal::DisconnectEntry;
using brave_vpn::internal::GetPhonebookPath;
using brave_vpn::internal::PrintRasError;
using brave_vpn::internal::RemoveEntry;

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
  const std::wstring name = base::UTF8ToWide(info.name);
  const std::wstring host = base::UTF8ToWide(info.url);
  const std::wstring user = base::UTF8ToWide(info.id);
  const std::wstring password = base::UTF8ToWide(info.pwd);
  return CreateEntry(name.c_str(), host.c_str(), user.c_str(),
                     password.c_str());
}

bool BraveVPNConnectionManagerWin::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
  return true;
}

bool BraveVPNConnectionManagerWin::Connect(const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  return ConnectEntry(w_name.c_str());
}

bool BraveVPNConnectionManagerWin::Disconnect(const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  return DisconnectEntry(w_name.c_str());
}

bool BraveVPNConnectionManagerWin::RemoveVPNConnection(
    const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  return RemoveEntry(w_name.c_str());
}

}  // namespace brave_vpn
