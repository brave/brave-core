/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_os_connection_api_win.h"

#include <windows.h>

#include <ras.h>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/utils_win.h"

// Most of Windows implementations are based on Brian Clifton
// (brian@clifton.me)'s work (https://github.com/bsclifton/winvpntool).

using brave_vpn::internal::CheckConnectionResult;
using brave_vpn::internal::CloseEventHandleForConnectFailed;
using brave_vpn::internal::CloseEventHandleForConnecting;
using brave_vpn::internal::CloseEventHandleForDisconnecting;
using brave_vpn::internal::CreateEntry;
using brave_vpn::internal::GetEventHandleForConnectFailed;
using brave_vpn::internal::GetEventHandleForConnecting;
using brave_vpn::internal::GetEventHandleForDisconnecting;
using brave_vpn::internal::GetPhonebookPath;
using brave_vpn::internal::PrintRasError;
using brave_vpn::internal::RemoveEntry;

namespace brave_vpn {

namespace {

void ConnectEntry(const std::wstring& name) {
  brave_vpn::internal::ConnectEntry(name);
}

void DisconnectEntry(const std::wstring& name) {
  brave_vpn::internal::DisconnectEntry(name);
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
  CloseHandle(event_handle_for_connected_);
  CloseHandle(event_handle_for_disconnected_);
  CloseEventHandleForConnecting();
  CloseEventHandleForDisconnecting();
  CloseEventHandleForConnectFailed();
}

void BraveVPNOSConnectionAPIWin::CreateVPNConnection(
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

void BraveVPNOSConnectionAPIWin::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
}

void BraveVPNOSConnectionAPIWin::Connect(const std::string& name) {
  // Connection state update from this call will be done by monitoring.
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ConnectEntry, base::UTF8ToWide(name)));
}

void BraveVPNOSConnectionAPIWin::Disconnect(const std::string& name) {
  // Connection state update from this call will be done by monitoring.
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&DisconnectEntry, base::UTF8ToWide(name)));
}

void BraveVPNOSConnectionAPIWin::RemoveVPNConnection(const std::string& name) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&RemoveEntry, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnRemoved,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNOSConnectionAPIWin::CheckConnection(const std::string& name) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&internal::CheckConnection, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnCheckConnection,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNOSConnectionAPIWin::OnObjectSignaled(HANDLE object) {
  DCHECK(!target_vpn_entry_name().empty());

  CheckConnectionResult result = CheckConnectionResult::DISCONNECTING;
  if (object == GetEventHandleForConnecting()) {
    result = CheckConnectionResult::CONNECTING;
  } else if (object == GetEventHandleForConnectFailed()) {
    result = CheckConnectionResult::CONNECT_FAILED;
  } else if (object == GetEventHandleForDisconnecting()) {
    result = CheckConnectionResult::DISCONNECTING;
  } else if (object == event_handle_for_connected_) {
    result = CheckConnectionResult::CONNECTED;
  } else if (object == event_handle_for_disconnected_) {
    result = CheckConnectionResult::DISCONNECTED;
  } else {
    NOTREACHED();
  }

  OnCheckConnection(target_vpn_entry_name(), result);
}

void BraveVPNOSConnectionAPIWin::OnCheckConnection(
    const std::string& name,
    CheckConnectionResult result) {
  for (Observer& obs : observers_) {
    switch (result) {
      case CheckConnectionResult::CONNECTED:
        obs.OnConnected();
        break;
      case CheckConnectionResult::CONNECTING:
        obs.OnIsConnecting();
        break;
      case CheckConnectionResult::CONNECT_FAILED:
        obs.OnConnectFailed();
        break;
      case CheckConnectionResult::DISCONNECTED:
        obs.OnDisconnected();
        break;
      case CheckConnectionResult::DISCONNECTING:
        obs.OnIsDisconnecting();
        break;
      default:
        break;
    }
  }
}

void BraveVPNOSConnectionAPIWin::OnCreated(const std::string& name,
                                           bool success) {
  if (!success) {
    for (Observer& obs : observers_)
      obs.OnCreateFailed();
    return;
  }

  for (Observer& obs : observers_)
    obs.OnCreated();
}

void BraveVPNOSConnectionAPIWin::OnRemoved(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnRemoved();
}

void BraveVPNOSConnectionAPIWin::StartVPNConnectionChangeMonitoring() {
  DCHECK(!event_handle_for_connected_ && !event_handle_for_disconnected_);

  event_handle_for_connected_ = CreateEvent(NULL, false, false, NULL);
  event_handle_for_disconnected_ = CreateEvent(NULL, false, false, NULL);

  // We don't need to check current connection state again if monitor each event
  // separately.
  RasConnectionNotificationW(static_cast<HRASCONN>(INVALID_HANDLE_VALUE),
                             event_handle_for_connected_, RASCN_Connection);
  RasConnectionNotificationW(static_cast<HRASCONN>(INVALID_HANDLE_VALUE),
                             event_handle_for_disconnected_,
                             RASCN_Disconnection);

  connected_event_watcher_.StartWatchingMultipleTimes(
      event_handle_for_connected_, this);
  disconnected_event_watcher_.StartWatchingMultipleTimes(
      event_handle_for_disconnected_, this);
  connecting_event_watcher_.StartWatchingMultipleTimes(
      GetEventHandleForConnecting(), this);
  disconnecting_event_watcher_.StartWatchingMultipleTimes(
      GetEventHandleForDisconnecting(), this);
  connect_failed_event_watcher_.StartWatchingMultipleTimes(
      GetEventHandleForConnectFailed(), this);
}

}  // namespace brave_vpn
