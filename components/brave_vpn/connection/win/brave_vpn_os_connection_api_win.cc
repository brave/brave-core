/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/connection/win/brave_vpn_os_connection_api_win.h"

#include <windows.h>

#include <ras.h>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/connection/win/utils.h"

// Most of Windows implementations are based on Brian Clifton
// (brian@clifton.me)'s work (https://github.com/bsclifton/winvpntool).

using brave_vpn::internal::CheckConnectionResult;
using brave_vpn::internal::CreateEntry;
using brave_vpn::internal::GetPhonebookPath;
using brave_vpn::internal::PrintRasError;
using brave_vpn::internal::RemoveEntry;

namespace brave_vpn {

namespace {

bool ConnectEntry(const std::wstring& name) {
  return brave_vpn::internal::ConnectEntry(name);
}

bool DisconnectEntry(const std::wstring& name) {
  return brave_vpn::internal::DisconnectEntry(name);
}

}  // namespace

// static
BraveVPNOSConnectionAPI* BraveVPNOSConnectionAPI::GetInstance() {
  static base::NoDestructor<BraveVPNOSConnectionAPIWin> s_manager;
  return s_manager.get();
}

BraveVPNOSConnectionAPIWin::BraveVPNOSConnectionAPIWin() {
  StartVPNConnectionChangeMonitoring();
}

BraveVPNOSConnectionAPIWin::~BraveVPNOSConnectionAPIWin() {
  CloseHandle(event_handle_for_connected_disconnected_);
}

void BraveVPNOSConnectionAPIWin::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&CreateEntry, base::UTF8ToWide(info.connection_name()),
                     base::UTF8ToWide(info.hostname()),
                     base::UTF8ToWide(info.username()),
                     base::UTF8ToWide(info.password())),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnCreated,
                     weak_factory_.GetWeakPtr(), info.connection_name()));
}

void BraveVPNOSConnectionAPIWin::ConnectImpl(const std::string& name) {
  // Connection state update from this call will be done by monitoring.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ConnectEntry, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnConnected,
                     weak_factory_.GetWeakPtr()));
}

void BraveVPNOSConnectionAPIWin::DisconnectImpl(const std::string& name) {
  // Connection state update from this call will be done by monitoring.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&DisconnectEntry, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnDisconnected,
                     weak_factory_.GetWeakPtr()));
}

void BraveVPNOSConnectionAPIWin::RemoveVPNConnectionImpl(
    const std::string& name) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&RemoveEntry, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnRemoved,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNOSConnectionAPIWin::CheckConnectionImpl(const std::string& name) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&internal::CheckConnection, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnCheckConnection,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNOSConnectionAPIWin::OnObjectSignaled(HANDLE object) {
  DCHECK(!target_vpn_entry_name().empty());

  // Check connection state for BraveVPN entry again when connected or
  // disconnected events are arrived because we can get both event from any os
  // vpn entry. All other events are sent by our code at utils_win.cc.
  if (object == event_handle_for_connected_disconnected_) {
    CheckConnectionImpl(target_vpn_entry_name());
    return;
  }
}

void BraveVPNOSConnectionAPIWin::OnCheckConnection(
    const std::string& name,
    CheckConnectionResult result) {
  switch (result) {
    case CheckConnectionResult::CONNECTED:
      BraveVPNOSConnectionAPIBase::OnConnected();
      break;
    case CheckConnectionResult::CONNECTING:
      OnIsConnecting();
      break;
    case CheckConnectionResult::CONNECT_FAILED:
      OnConnectFailed();
      break;
    case CheckConnectionResult::DISCONNECTED:
      BraveVPNOSConnectionAPIBase::OnDisconnected();
      break;
    case CheckConnectionResult::DISCONNECTING:
      OnIsDisconnecting();
      break;
    default:
      break;
  }
}

void BraveVPNOSConnectionAPIWin::OnCreated(const std::string& name,
                                           bool success) {
  if (!success) {
    OnCreateFailed();
    return;
  }

  BraveVPNOSConnectionAPIBase::OnCreated();
}

void BraveVPNOSConnectionAPIWin::OnConnected(bool success) {
  if (!success)
    BraveVPNOSConnectionAPIBase::OnConnectFailed();
}

void BraveVPNOSConnectionAPIWin::OnDisconnected(bool success) {
  // TODO(simonhong): Handle disconnect failed state.
  if (success)
    BraveVPNOSConnectionAPIBase::OnDisconnected();
}

void BraveVPNOSConnectionAPIWin::OnRemoved(const std::string& name,
                                           bool success) {}

void BraveVPNOSConnectionAPIWin::StartVPNConnectionChangeMonitoring() {
  DCHECK(!event_handle_for_connected_disconnected_);

  event_handle_for_connected_disconnected_ =
      CreateEvent(NULL, false, false, NULL);

  // Ase we pass INVALID_HANDLE_VALUE, we can get connected or disconnected
  // event from any os vpn entry. It's filtered by OnObjectSignaled().
  RasConnectionNotificationW(static_cast<HRASCONN>(INVALID_HANDLE_VALUE),
                             event_handle_for_connected_disconnected_,
                             RASCN_Connection | RASCN_Disconnection);
  connected_disconnected_event_watcher_.StartWatchingMultipleTimes(
      event_handle_for_connected_disconnected_, this);
}

}  // namespace brave_vpn
