/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/ikev2/win/brave_vpn_ras_connection_api_win.h"

#include <windows.h>

#include <netlistmgr.h>  // For CLSID_NetworkListManager

#include <wrl/client.h>
#include <memory>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/win/ras_utils.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
// Most of Windows implementations are based on Brian Clifton
// (brian@clifton.me)'s work (https://github.com/bsclifton/winvpntool).

using brave_vpn::ras::CheckConnectionResult;
using brave_vpn::ras::CreateEntry;
using brave_vpn::ras::RasOperationResult;
using brave_vpn::ras::RemoveEntry;

namespace brave_vpn {

namespace {

RasOperationResult ConnectEntry(const std::wstring& name) {
  return brave_vpn::ras::ConnectEntry(name);
}

RasOperationResult DisconnectEntry(const std::wstring& name) {
  return brave_vpn::ras::DisconnectEntry(name);
}

}  // namespace

std::unique_ptr<BraveVPNOSConnectionAPI> CreateBraveVPNIKEv2ConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel) {
  return std::make_unique<BraveVPNOSConnectionAPIWin>(url_loader_factory,
                                                      local_prefs, channel);
}

BraveVPNOSConnectionAPIWin::BraveVPNOSConnectionAPIWin(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : BraveVPNOSConnectionAPIBase(url_loader_factory, local_prefs, channel) {
  StartRasConnectionChangeMonitoring();
}

BraveVPNOSConnectionAPIWin::~BraveVPNOSConnectionAPIWin() {}

void BraveVPNOSConnectionAPIWin::CreateVPNConnectionImpl(
    const BraveVPNConnectionInfo& info) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()}, base::BindOnce(&CreateEntry, info),
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

void BraveVPNOSConnectionAPIWin::CheckConnectionImpl(const std::string& name) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ras::CheckConnection, base::UTF8ToWide(name)),
      base::BindOnce(&BraveVPNOSConnectionAPIWin::OnCheckConnection,
                     weak_factory_.GetWeakPtr(), name));
}

void BraveVPNOSConnectionAPIWin::OnRasConnectionStateChanged() {
  DCHECK(!target_vpn_entry_name().empty());

  // Check connection state for BraveVPN entry again when connected or
  // disconnected events are arrived because we can get both event from any os
  // vpn entry. All other events are sent by our code at utils_win.cc.
  CheckConnectionImpl(target_vpn_entry_name());
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
                                           const RasOperationResult& result) {
  if (!result.success) {
    SetLastConnectionError(result.error_description);
    OnCreateFailed();
    return;
  }

  BraveVPNOSConnectionAPIBase::OnCreated();
}

void BraveVPNOSConnectionAPIWin::OnConnected(const RasOperationResult& result) {
  if (!result.success) {
    SetLastConnectionError(result.error_description);
    BraveVPNOSConnectionAPIBase::OnConnectFailed();
  }
}

void BraveVPNOSConnectionAPIWin::OnDisconnected(
    const RasOperationResult& result) {
  // TODO(simonhong): Handle disconnect failed state.
  if (result.success) {
    BraveVPNOSConnectionAPIBase::OnDisconnected();
    return;
  }
  SetLastConnectionError(result.error_description);
}

bool BraveVPNOSConnectionAPIWin::IsPlatformNetworkAvailable() {
  // If any errors occur, return that internet connection is available.
  Microsoft::WRL::ComPtr<INetworkListManager> manager;
  HRESULT hr = ::CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(&manager));
  if (FAILED(hr)) {
    LOG(ERROR) << "CoCreateInstance(NetworkListManager) hr=" << std::hex << hr;
    return true;
  }

  VARIANT_BOOL is_connected;
  hr = manager->get_IsConnectedToInternet(&is_connected);
  if (FAILED(hr)) {
    LOG(ERROR) << "get_IsConnectedToInternet failed hr=" << std::hex << hr;
    return true;
  }

  // Normally VARIANT_TRUE/VARIANT_FALSE are used with the type VARIANT_BOOL
  // but in this case the docs explicitly say to use FALSE.
  // https://docs.microsoft.com/en-us/windows/desktop/api/Netlistmgr/
  //     nf-netlistmgr-inetworklistmanager-get_isconnectedtointernet
  return is_connected != FALSE;
}

}  // namespace brave_vpn
