/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_connection_manager_win.h"

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
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

void BraveVPNConnectionManagerWin::CreateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  const std::wstring name = base::UTF8ToWide(info.connection_name());
  const std::wstring host = base::UTF8ToWide(info.hostname());
  const std::wstring user = base::UTF8ToWide(info.username());
  const std::wstring password = base::UTF8ToWide(info.password());

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&CreateEntry, name.c_str(), host.c_str(), user.c_str(),
                     password.c_str()),
      base::BindOnce(&BraveVPNConnectionManagerWin::OnCreated,
                     weak_factory_.GetWeakPtr(), info.connection_name()));
}

void BraveVPNConnectionManagerWin::UpdateVPNConnection(
    const BraveVPNConnectionInfo& info) {
  NOTIMPLEMENTED();
}

void BraveVPNConnectionManagerWin::Connect(const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ConnectEntry, w_name.c_str()),
      base::BindOnce(&BraveVPNConnectionManagerWin::OnConnected,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNConnectionManagerWin::Disconnect(const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&DisconnectEntry, w_name.c_str()),
      base::BindOnce(&BraveVPNConnectionManagerWin::OnDisconnected,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNConnectionManagerWin::RemoveVPNConnection(
    const std::string& name) {
  const std::wstring w_name = base::UTF8ToWide(name);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&RemoveEntry, w_name.c_str()),
      base::BindOnce(&BraveVPNConnectionManagerWin::OnRemoved,
                     weak_factory_.GetWeakPtr(), name));
  RemoveEntry(w_name.c_str());
}

void BraveVPNConnectionManagerWin::OnCreated(const std::string& name,
                                             bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnCreated(name);
}

void BraveVPNConnectionManagerWin::OnConnected(const std::string& name,
                                               bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnConnected(name);
}

void BraveVPNConnectionManagerWin::OnDisconnected(const std::string& name,
                                                  bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnDisconnected(name);
}

void BraveVPNConnectionManagerWin::OnRemoved(const std::string& name,
                                             bool success) {
  if (!success)
    return;

  for (Observer& obs : observers_)
    obs.OnRemoved(name);
}

}  // namespace brave_vpn
