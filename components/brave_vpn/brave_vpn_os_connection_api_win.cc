/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_os_connection_api_win.h"

#include <ras.h>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/utils_win.h"

// Most of Windows implementations are based on Brian Clifton
// (brian@clifton.me)'s work (https://github.com/bsclifton/winvpntool).

using brave_vpn::internal::CheckConnectionResult;
using brave_vpn::internal::CreateEntry;
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
  CloseHandle(event_handle_);
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

  CheckConnection(target_vpn_entry_name());
}

void BraveVPNOSConnectionAPIWin::OnCheckConnection(
    const std::string& name,
    CheckConnectionResult result) {
  if (result == CheckConnectionResult::UNKNOWN)
    return;

  const bool connected = result == CheckConnectionResult::CONNECTED;
  for (Observer& obs : observers_) {
    connected ? obs.OnConnected(name) : obs.OnDisconnected(name);
  }
}

void BraveVPNOSConnectionAPIWin::OnCreated(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnCreated(name);
}

void BraveVPNOSConnectionAPIWin::OnRemoved(const std::string& name,
                                           bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnRemoved(name);
}

void BraveVPNOSConnectionAPIWin::StartVPNConnectionChangeMonitoring() {
  DCHECK(!event_handle_);
  event_handle_ = CreateEvent(nullptr, false, false, nullptr);

  RasConnectionNotificationW(static_cast<HRASCONN>(INVALID_HANDLE_VALUE),
                             event_handle_,
                             (RASCN_Connection | RASCN_Disconnection));
  watcher_.StartWatchingMultipleTimes(event_handle_, this);
}

}  // namespace brave_vpn
