/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"

#include <optional>
#include <vector>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_helper.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_vpn {

BraveVPNOSConnectionAPI::BraveVPNOSConnectionAPI(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs)
    : local_prefs_(local_prefs),
      url_loader_factory_(url_loader_factory),
      region_data_manager_(url_loader_factory, local_prefs),
      weak_factory_(this) {
  DCHECK(url_loader_factory_);
  // Safe to use Unretained here because |region_data_manager_| is owned
  // instance.
  region_data_manager_.set_selected_region_changed_callback(
      base::BindRepeating(&BraveVPNOSConnectionAPI::NotifySelectedRegionChanged,
                          base::Unretained(this)));
  region_data_manager_.set_region_data_ready_callback(base::BindRepeating(
      &BraveVPNOSConnectionAPI::NotifyRegionDataReady, base::Unretained(this)));
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

BraveVPNOSConnectionAPI::~BraveVPNOSConnectionAPI() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

mojom::ConnectionState BraveVPNOSConnectionAPI::GetConnectionState() const {
  return connection_state_;
}

BraveVPNRegionDataManager& BraveVPNOSConnectionAPI::GetRegionDataManager() {
  return region_data_manager_;
}

void BraveVPNOSConnectionAPI::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNOSConnectionAPI::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVPNOSConnectionAPI::SetConnectionStateForTesting(
    mojom::ConnectionState state) {
  UpdateAndNotifyConnectionStateChange(state);
}

void BraveVPNOSConnectionAPI::NotifyRegionDataReady(bool ready) const {
  for (auto& obs : observers_) {
    obs.OnRegionDataReady(ready);
  }
}

void BraveVPNOSConnectionAPI::NotifySelectedRegionChanged(
    const std::string& name) const {
  for (auto& obs : observers_) {
    obs.OnSelectedRegionChanged(name);
  }
}

void BraveVPNOSConnectionAPI::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  VLOG(1) << __func__ << " : " << type;
  CheckConnection();
}

BraveVpnAPIRequest* BraveVPNOSConnectionAPI::GetAPIRequest() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_) {
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  }

  return api_request_.get();
}

void BraveVPNOSConnectionAPI::ResetHostname() {
  hostname_.reset();
}

void BraveVPNOSConnectionAPI::ResetConnectionState() {
  // Don't use UpdateAndNotifyConnectionStateChange() to update connection state
  // and set state directly because we have a logic to ignore disconnected state
  // when connect failed.
  connection_state_ = mojom::ConnectionState::DISCONNECTED;
  for (auto& obs : observers_) {
    obs.OnConnectionStateChanged(connection_state_);
  }
}

void BraveVPNOSConnectionAPI::UpdateAndNotifyConnectionStateChange(
    mojom::ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state) {
    return;
  }

  connection_state_ = state;
  for (auto& obs : observers_) {
    obs.OnConnectionStateChanged(connection_state_);
  }
}

bool BraveVPNOSConnectionAPI::QuickCancelIfPossible() {
  if (!api_request_) {
    return false;
  }

  // We're waiting responce from vpn server.
  // Can do quick cancel in this situation by cancel that request.
  ResetAPIRequestInstance();
  return true;
}
std::string BraveVPNOSConnectionAPI::GetHostname() const {
  return hostname_ ? hostname_->hostname : "";
}

void BraveVPNOSConnectionAPI::ResetAPIRequestInstance() {
  api_request_.reset();
}

std::string BraveVPNOSConnectionAPI::GetLastConnectionError() const {
  return last_connection_error_;
}

void BraveVPNOSConnectionAPI::SetLastConnectionError(const std::string& error) {
  VLOG(2) << __func__ << " : " << error;
  last_connection_error_ = error;
}

void BraveVPNOSConnectionAPI::FetchHostnamesForRegion(const std::string& name) {
  // Hostname will be replaced with latest one.
  hostname_.reset();

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  // Unretained is safe here becasue this class owns request helper.
  GetAPIRequest()->GetHostnamesForRegion(
      base::BindOnce(&BraveVPNOSConnectionAPI::OnFetchHostnames,
                     base::Unretained(this), name),
      name);
}

void BraveVPNOSConnectionAPI::OnFetchHostnames(const std::string& region,
                                               const std::string& hostnames,
                                               bool success) {
  if (!success) {
    VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }

  ResetAPIRequestInstance();

  std::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, value->GetList());
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(mojom::ConnectionState::CONNECT_FAILED);
}

void BraveVPNOSConnectionAPI::ParseAndCacheHostnames(
    const std::string& region,
    const base::Value::List& hostnames_value) {
  std::vector<Hostname> hostnames = ParseHostnames(hostnames_value);

  if (hostnames.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }

  hostname_ = PickBestHostname(hostnames);
  if (hostname_->hostname.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(
        mojom::ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : Picked " << hostname_->hostname << ", "
          << hostname_->display_name << ", " << hostname_->is_offline << ", "
          << hostname_->capacity_score;

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  // Get profile credentials it to create OS VPN entry.
  VLOG(2) << __func__ << " : request profile credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());
  FetchProfileCredentials();
}

std::string BraveVPNOSConnectionAPI::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNOSConnectionAPI::ToggleConnection() {
  const bool can_disconnect =
      (GetConnectionState() == mojom::ConnectionState::CONNECTED ||
       GetConnectionState() == mojom::ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

void BraveVPNOSConnectionAPI::MaybeInstallSystemServices() {
  if (!install_system_service_callback_) {
    VLOG(2) << __func__ << " : no install system service callback set";
    return;
  }

  // Installation should only be called once per session.
  // It's safe to call more than once because the install itself checks if
  // the services are already registered before doing anything.
  if (system_service_installed_event_.is_signaled()) {
    VLOG(2)
        << __func__
        << " : installation has already been performed this session; exiting";
    return;
  }

  // This API could be called more than once because BraveVpnService is a
  // per-profile service. If service install is in-progress now, just return.
  if (install_in_progress_) {
    VLOG(2) << __func__ << " : install already in progress; exiting";
    return;
  }

#if BUILDFLAG(IS_WIN)
  install_in_progress_ = true;
  base::ThreadPool::CreateCOMSTATaskRunner({base::MayBlock()})
      ->PostTaskAndReplyWithResult(
          FROM_HERE, install_system_service_callback_,
          base::BindOnce(
              &BraveVPNOSConnectionAPI::OnInstallSystemServicesCompleted,
              weak_factory_.GetWeakPtr()));
#endif
}

void BraveVPNOSConnectionAPI::OnInstallSystemServicesCompleted(bool success) {
  VLOG(1) << "OnInstallSystemServicesCompleted: success=" << success;
  if (success) {
#if BUILDFLAG(IS_WIN)
    // Update prefs first before signaling the event because the event could
    // check the prefs.
    EnableWireguardIfPossible(local_prefs_);
#endif
    system_service_installed_event_.Signal();
  }
  install_in_progress_ = false;
}

bool BraveVPNOSConnectionAPI::ScheduleConnectRequestIfNeeded() {
  if (!install_in_progress_) {
    return false;
  }

  system_service_installed_event_.Post(
      FROM_HERE, base::BindOnce(&BraveVPNOSConnectionAPI::Connect,
                                weak_factory_.GetWeakPtr()));
  return true;
}

}  // namespace brave_vpn
