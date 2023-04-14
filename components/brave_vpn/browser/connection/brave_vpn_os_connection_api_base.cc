/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api_base.h"

#include <algorithm>
#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/power_monitor/power_monitor.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_helper.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_data_types.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

namespace {

base::Value::Dict GetValueFromRegion(const mojom::Region& region) {
  base::Value::Dict region_dict;
  region_dict.Set(kRegionContinentKey, region.continent);
  region_dict.Set(kRegionNameKey, region.name);
  region_dict.Set(kRegionNamePrettyKey, region.name_pretty);
  region_dict.Set(kRegionCountryIsoCodeKey, region.country_iso_code);
  return region_dict;
}

bool IsValidRegionValue(const base::Value::Dict& value) {
  if (!value.FindString(kRegionContinentKey) ||
      !value.FindString(kRegionNameKey) ||
      !value.FindString(kRegionNamePrettyKey) ||
      !value.FindString(kRegionCountryIsoCodeKey)) {
    return false;
  }

  return true;
}

mojom::Region GetRegionFromValue(const base::Value::Dict& value) {
  mojom::Region region;
  if (auto* continent = value.FindString(brave_vpn::kRegionContinentKey)) {
    region.continent = *continent;
  }
  if (auto* name = value.FindString(brave_vpn::kRegionNameKey)) {
    region.name = *name;
  }
  if (auto* name_pretty = value.FindString(brave_vpn::kRegionNamePrettyKey)) {
    region.name_pretty = *name_pretty;
  }
  if (auto* country_iso_code =
          value.FindString(brave_vpn::kRegionCountryIsoCodeKey)) {
    region.country_iso_code = *country_iso_code;
  }
  return region;
}

bool ValidateCachedRegionData(const base::Value::List& region_value) {
  for (const auto& value : region_value) {
    // Make sure cached one has all latest properties.
    if (!value.is_dict() || !IsValidRegionValue(value.GetDict())) {
      return false;
    }
  }

  return true;
}

std::vector<mojom::Region> ParseRegionList(
    const base::Value::List& region_list) {
  std::vector<mojom::Region> regions;
  for (const auto& value : region_list) {
    DCHECK(value.is_dict());
    if (!value.is_dict()) {
      continue;
    }
    regions.push_back(GetRegionFromValue(value.GetDict()));
  }

  // Sort region list alphabetically
  std::sort(regions.begin(), regions.end(),
            [](mojom::Region& a, mojom::Region& b) {
              return (a.name_pretty < b.name_pretty);
            });
  return regions;
}

}  // namespace

using ConnectionState = mojom::ConnectionState;

BraveVPNOSConnectionAPIBase::BraveVPNOSConnectionAPIBase(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    version_info::Channel channel)
    : target_vpn_entry_name_(GetBraveVPNEntryName(channel)),
      local_prefs_(local_prefs),
      url_loader_factory_(url_loader_factory) {
  DCHECK(url_loader_factory && local_prefs);
  LoadCachedRegionData();
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

BraveVPNOSConnectionAPIBase::~BraveVPNOSConnectionAPIBase() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void BraveVPNOSConnectionAPIBase::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BraveVPNOSConnectionAPIBase::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void BraveVPNOSConnectionAPIBase::SetConnectionState(
    mojom::ConnectionState state) {
  UpdateAndNotifyConnectionStateChange(state);
}

void BraveVPNOSConnectionAPIBase::ResetConnectionState() {
  // Don't use UpdateAndNotifyConnectionStateChange() to update connection state
  // and set state directly because we have a logic to ignore disconnected state
  // when connect failed.
  connection_state_ = ConnectionState::DISCONNECTED;
  for (auto& obs : observers_) {
    obs.OnConnectionStateChanged(connection_state_);
  }
}

std::string BraveVPNOSConnectionAPIBase::GetLastConnectionError() const {
  return last_connection_error_;
}

const std::vector<mojom::Region>& BraveVPNOSConnectionAPIBase::GetRegions()
    const {
  return regions_;
}

bool BraveVPNOSConnectionAPIBase::IsRegionDataReady() const {
  return !regions_.empty();
}

void BraveVPNOSConnectionAPIBase::SetSelectedRegion(const std::string& name) {
  // Don't allow region change while operation is in-progress.
  auto connection_state = GetConnectionState();
  if (connection_state == ConnectionState::DISCONNECTING ||
      connection_state == ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << ": Current state: " << connection_state
            << " : prevent changing selected region while previous operation "
               "is in-progress";
    // This is workaround to prevent UI changes seleted region.
    // Early return by notify again with current region name.
    for (auto& obs : observers_) {
      obs.OnSelectedRegionChanged(GetSelectedRegion());
    }
    return;
  }

  local_prefs_->SetString(prefs::kBraveVPNSelectedRegion, name);

  for (auto& obs : observers_) {
    obs.OnSelectedRegionChanged(name);
  }

  // As new selected region is used, |connection_info_| for previous selected
  // should be cleared.
  ResetConnectionInfo();
}

std::string BraveVPNOSConnectionAPIBase::GetSelectedRegion() const {
  DCHECK(!regions_.empty())
      << "regions data must be prepared before panel asks.";

  auto region_name = local_prefs_->GetString(prefs::kBraveVPNSelectedRegion);
  if (region_name.empty()) {
    // Gives device region if there is no cached selected region.
    VLOG(2) << __func__ << " : give device region instead.";
    region_name = GetDeviceRegion();
  }

  DCHECK(!region_name.empty());
  return region_name;
}

bool BraveVPNOSConnectionAPIBase::IsInProgress() const {
  return connection_state_ == ConnectionState::DISCONNECTING ||
         connection_state_ == ConnectionState::CONNECTING;
}

void BraveVPNOSConnectionAPIBase::CreateVPNConnection() {
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (prevent_creation_) {
    CHECK_IS_TEST();
    return;
  }
  CreateVPNConnectionImpl(connection_info_);
}

void BraveVPNOSConnectionAPIBase::SetPreventCreationForTesting(bool value) {
  prevent_creation_ = value;
}

void BraveVPNOSConnectionAPIBase::Connect() {
  if (IsInProgress()) {
    VLOG(2) << __func__ << ": Current state: " << connection_state_
            << " : prevent connecting while previous operation is in-progress";
    return;
  }

  // Ignore connect request while cancelling is in-progress.
  if (cancel_connecting_)
    return;

  // User can ask connect again when user want to change region.
  if (GetConnectionState() == ConnectionState::CONNECTED) {
    // Disconnect first and then create again to setup for new region.
    // Set needs_connect to connect again after disconnected.
    needs_connect_ = true;
    Disconnect();
    return;
  }

  VLOG(2) << __func__ << " : start connecting!";
  last_connection_error_.clear();
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);

  if (connection_info_.IsValid()) {
    VLOG(2) << __func__
            << " : Create os vpn entry with cached connection_info.";
    CreateVPNConnectionImpl(connection_info_);
    return;
  }

  // If user doesn't select region explicitely, use default device region.
  std::string target_region_name = GetSelectedRegion();
  if (target_region_name.empty()) {
    target_region_name = GetDeviceRegion();
    VLOG(2) << __func__ << " : start connecting with valid default_region: "
            << target_region_name;
  }
  DCHECK(!target_region_name.empty());
  FetchHostnamesForRegion(target_region_name);
}

void BraveVPNOSConnectionAPIBase::Disconnect() {
  if (connection_state_ == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }

  if (connection_state_ == ConnectionState::DISCONNECTING) {
    VLOG(2) << __func__ << " : disconnecting in progress";
    return;
  }

  if (connection_state_ != ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << " : start disconnecting!";
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
    DisconnectImpl(target_vpn_entry_name_);
    return;
  }

  cancel_connecting_ = true;
  VLOG(2) << __func__ << " : Start cancelling connect request";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);

  if (QuickCancelIfPossible()) {
    VLOG(2) << __func__ << " : Do quick cancel";
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
  }
}

void BraveVPNOSConnectionAPIBase::ToggleConnection() {
  const bool can_disconnect =
      (connection_state_ == ConnectionState::CONNECTED ||
       connection_state_ == ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

void BraveVPNOSConnectionAPIBase::RemoveVPNConnection() {
  VLOG(2) << __func__;
  RemoveVPNConnectionImpl(target_vpn_entry_name_);
}

void BraveVPNOSConnectionAPIBase::CheckConnection() {
  CheckConnectionImpl(target_vpn_entry_name_);
}

void BraveVPNOSConnectionAPIBase::ResetConnectionInfo() {
  VLOG(2) << __func__;
  connection_info_.Reset();
}

std::string BraveVPNOSConnectionAPIBase::GetHostname() const {
  return hostname_ ? hostname_->hostname : "";
}

void BraveVPNOSConnectionAPIBase::OnCreated() {
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  // It's time to ask connecting to os after vpn entry is created.
  ConnectImpl(target_vpn_entry_name_);
}

void BraveVPNOSConnectionAPIBase::OnCreateFailed() {
  VLOG(2) << __func__;

  // Clear connecting cancel request.
  if (cancel_connecting_)
    cancel_connecting_ = false;

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_NOT_ALLOWED);
}

void BraveVPNOSConnectionAPIBase::OnConnected() {
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    // As connect is done, we don't need more for cancelling.
    // Just start normal Disconenct() process.
    cancel_connecting_ = false;
    DisconnectImpl(target_vpn_entry_name_);
    return;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTED);
}

void BraveVPNOSConnectionAPIBase::OnIsConnecting() {
  VLOG(2) << __func__;

  if (!cancel_connecting_)
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);
}

void BraveVPNOSConnectionAPIBase::OnConnectFailed() {
  cancel_connecting_ = false;

  // Clear previously used connection info if failed.
  connection_info_.Reset();

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVPNOSConnectionAPIBase::OnDisconnected() {
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);

  if (needs_connect_) {
    needs_connect_ = false;
    Connect();
  }
}

void BraveVPNOSConnectionAPIBase::OnIsDisconnecting() {
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVPNOSConnectionAPIBase::SetLastConnectionError(
    const std::string& error) {
  VLOG(2) << __func__ << " : " << error;
  last_connection_error_ = error;
}

void BraveVPNOSConnectionAPIBase::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // It's rare but sometimes Brave doesn't get vpn status update from OS.
  // Checking here will make vpn status update properly in that situation.
  VLOG(2) << __func__ << " : " << type;
  CheckConnection();
}

void BraveVPNOSConnectionAPIBase::UpdateAndNotifyConnectionStateChange(
    ConnectionState state) {
  // this is a simple state machine for handling connection state
  if (connection_state_ == state)
    return;

  // Ignore disconnected state while connecting is in-progress.
  // Network status can be changed during the vpn connection because
  // establishing vpn connection could make system network offline temporarily.
  // Whenever we get network status change, we check vpn connection state and
  // it could give disconnected vpn connection during that situation.
  // So, don't notify this disconnected state change while connecting because
  // it's temporal state.
  if (connection_state_ == ConnectionState::CONNECTING &&
      state == ConnectionState::DISCONNECTED && !cancel_connecting_) {
    VLOG(2) << __func__ << ": Ignore disconnected state while connecting";
    return;
  }
#if BUILDFLAG(IS_WIN)
  // On Windows, we could get disconnected state after connect failed.
  // To make connect failed state as a last state, ignore disconnected state.
  if (connection_state_ == ConnectionState::CONNECT_FAILED &&
      state == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << ": Ignore disconnected state after connect failed";
    return;
  }
#endif  // BUILDFLAG(IS_WIN)
  VLOG(2) << __func__ << " : changing from " << connection_state_ << " to "
          << state;

  connection_state_ = state;
  for (auto& obs : observers_)
    obs.OnConnectionStateChanged(connection_state_);
}

bool BraveVPNOSConnectionAPIBase::QuickCancelIfPossible() {
  if (!api_request_)
    return false;

  // We're waiting responce from vpn server.
  // Can do quick cancel in this situation by cancel that request.
  api_request_.reset();
  return true;
}

BraveVpnAPIRequest* BraveVPNOSConnectionAPIBase::GetAPIRequest() {
  if (!url_loader_factory_) {
    CHECK_IS_TEST();
    return nullptr;
  }

  if (!api_request_) {
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  }

  return api_request_.get();
}

std::string BraveVPNOSConnectionAPIBase::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEnvironment);
}

void BraveVPNOSConnectionAPIBase::FetchHostnamesForRegion(
    const std::string& name) {
  VLOG(2) << __func__;

  // Hostname will be replaced with latest one.
  hostname_.reset();

  if (!GetAPIRequest()) {
    CHECK_IS_TEST();
    return;
  }

  // Unretained is safe here becasue this class owns request helper.
  GetAPIRequest()->GetHostnamesForRegion(
      base::BindOnce(&BraveVPNOSConnectionAPIBase::OnFetchHostnames,
                     base::Unretained(this), name),
      name);
}

void BraveVPNOSConnectionAPIBase::OnFetchHostnames(const std::string& region,
                                                   const std::string& hostnames,
                                                   bool success) {
  // This response should not be called if cancelled.
  DCHECK(!cancel_connecting_);
  VLOG(2) << __func__;

  if (!success) {
    VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  api_request_.reset();

  absl::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, value->GetList());
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVPNOSConnectionAPIBase::ParseAndCacheHostnames(
    const std::string& region,
    const base::Value::List& hostnames_value) {
  std::vector<Hostname> hostnames = ParseHostnames(hostnames_value);

  if (hostnames.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  hostname_ = PickBestHostname(hostnames);
  if (hostname_->hostname.empty()) {
    VLOG(2) << __func__ << " : got empty hostnames list for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
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

  GetAPIRequest()->GetProfileCredentials(
      base::BindOnce(&BraveVPNOSConnectionAPIBase::OnGetProfileCredentials,
                     base::Unretained(this)),
      GetSubscriberCredential(local_prefs_), hostname_->hostname);
}

void BraveVPNOSConnectionAPIBase::OnGetProfileCredentials(
    const std::string& profile_credential,
    bool success) {
  DCHECK(!cancel_connecting_);

  if (!success) {
    VLOG(2) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  api_request_.reset();

  VLOG(2) << __func__ << " : received profile credential";

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value && value->is_dict()) {
    constexpr char kUsernameKey[] = "eap-username";
    constexpr char kPasswordKey[] = "eap-password";
    const auto& dict = value->GetDict();
    const std::string* username = dict.FindString(kUsernameKey);
    const std::string* password = dict.FindString(kPasswordKey);
    if (!username || !password) {
      VLOG(2) << __func__ << " : it's invalid profile credential";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }

    connection_info_.SetConnectionInfo(
        target_vpn_entry_name_, hostname_->hostname, *username, *password);
    // Let's create os vpn entry with |connection_info_|.
    CreateVPNConnection();
    return;
  }

  VLOG(2) << __func__ << " : it's invalid profile credential";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

const BraveVPNConnectionInfo& BraveVPNOSConnectionAPIBase::connection_info()
    const {
  return connection_info_;
}

mojom::ConnectionState BraveVPNOSConnectionAPIBase::GetConnectionState() const {
  return connection_state_;
}

std::string BraveVPNOSConnectionAPIBase::GetDeviceRegion() const {
  return local_prefs_->GetString(prefs::kBraveVPNDeviceRegion);
}

void BraveVPNOSConnectionAPIBase::SetDeviceRegion(const std::string& name) {
  local_prefs_->SetString(prefs::kBraveVPNDeviceRegion, name);
}

void BraveVPNOSConnectionAPIBase::SetFallbackDeviceRegion() {
  // Set first item in the region list as a |device_region_| as a fallback.
  DCHECK(!regions_.empty());
  SetDeviceRegion(regions_[0].name);
}

void BraveVPNOSConnectionAPIBase::SetDeviceRegionWithTimezone(
    const base::Value::List& timezones_value) {
  const std::string current_time_zone = GetCurrentTimeZone();
  if (current_time_zone.empty()) {
    return;
  }

  for (const auto& timezones : timezones_value) {
    DCHECK(timezones.is_dict());
    if (!timezones.is_dict()) {
      continue;
    }

    const std::string* region_name = timezones.GetDict().FindString("name");
    if (!region_name) {
      continue;
    }
    const auto* timezone_list_value = timezones.GetDict().FindList("timezones");
    if (!timezone_list_value) {
      continue;
    }

    for (const auto& timezone : *timezone_list_value) {
      DCHECK(timezone.is_string());
      if (!timezone.is_string()) {
        continue;
      }
      if (current_time_zone == timezone.GetString()) {
        VLOG(2) << "Found default region: " << *region_name;
        SetDeviceRegion(*region_name);
        // Use device region as a default selected region.
        if (GetSelectedRegion().empty()) {
          SetSelectedRegion(*region_name);
        }
        return;
      }
    }
  }
}

void BraveVPNOSConnectionAPIBase::LoadCachedRegionData() {
  // Already loaded from cache.
  if (!regions_.empty()) {
    return;
  }

  // Empty device region means it's initial state.
  if (GetDeviceRegion().empty()) {
    return;
  }

  auto* preference = local_prefs_->FindPreference(prefs::kBraveVPNRegionList);
  DCHECK(preference);
  // Early return when we don't have any cached region data.
  if (preference->IsDefaultValue()) {
    return;
  }

  // If cached one is outdated, don't use it.
  if (!ValidateCachedRegionData(preference->GetValue()->GetList())) {
    VLOG(2) << __func__ << " : Cached data is outdate. Will get fetch latest.";
    return;
  }

  if (ParseAndCacheRegionList(preference->GetValue()->GetList())) {
    VLOG(2) << __func__ << " : Loaded cached region list";
    return;
  }

  VLOG(2) << __func__ << " : Failed to load cached region list";
}

void BraveVPNOSConnectionAPIBase::FetchRegionDataIfNeeded() {
  if (region_data_api_request_) {
    VLOG(2) << __func__ << " : Region data fetching is in-progress";
    return;
  }

  const auto last_fetched_date =
      local_prefs_->GetTime(prefs::kBraveVPNRegionListFetchedDate);
  constexpr int kRegionDataFetchIntervalInHours = 5;
  // Skip checking region data update when we have cached one and its age is
  // younger than 5h.
  if (IsRegionDataReady() && !last_fetched_date.is_null() &&
      (base::Time::Now() - last_fetched_date).InHours() <
          kRegionDataFetchIntervalInHours) {
    VLOG(2) << __func__ << " : Don't need to fetch. Current one is valid";
    NotifyRegionDataReady();
    return;
  }

  region_data_api_request_ =
      std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  VLOG(2) << __func__ << " : Start fetching region data";

  // Unretained is safe here becasue this class owns |region_data_api_request_|.
  region_data_api_request_->GetAllServerRegions(base::BindOnce(
      &BraveVPNOSConnectionAPIBase::OnFetchRegionList, base::Unretained(this)));
}

void BraveVPNOSConnectionAPIBase::OnFetchRegionList(
    const std::string& region_list,
    bool success) {
  DCHECK(region_data_api_request_);
  region_data_api_request_.reset();

  absl::optional<base::Value> value = base::JSONReader::Read(region_list);
  if (value && value->is_list() &&
      ParseAndCacheRegionList(value->GetList(), true)) {
    VLOG(2) << "Got valid region list";
    // Set default device region and it'll be updated when received valid
    // timezone info.
    SetFallbackDeviceRegion();
    // Fetch timezones list to determine default region of this device.
    region_data_api_request_ =
        std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
    region_data_api_request_->GetTimezonesForRegions(
        base::BindOnce(&BraveVPNOSConnectionAPIBase::OnFetchTimezones,
                       base::Unretained(this)));
    return;
  }

  VLOG(2) << "Got invalid region list";
  NotifyRegionDataReady();
}

void BraveVPNOSConnectionAPIBase::NotifyRegionDataReady() const {
  for (auto& obs : observers_) {
    obs.OnRegionDataReady(!regions_.empty());
  }
}

bool BraveVPNOSConnectionAPIBase::ParseAndCacheRegionList(
    const base::Value::List& region_value,
    bool save_to_prefs) {
  const auto new_regions = ParseRegionList(region_value);
  VLOG(2) << __func__ << " : has regionlist: " << !new_regions.empty();

  // To avoid deleting current valid |regions_|, only assign when
  // |new_regions| is not empty.
  if (new_regions.empty()) {
    return false;
  }

  regions_ = new_regions;

  if (save_to_prefs) {
    SetRegionListToPrefs();
  }
  return true;
}

void BraveVPNOSConnectionAPIBase::OnFetchTimezones(
    const std::string& timezones_list,
    bool success) {
  region_data_api_request_.reset();

  absl::optional<base::Value> value = base::JSONReader::Read(timezones_list);
  if (success && value && value->is_list()) {
    VLOG(2) << "Got valid timezones list";
    SetDeviceRegionWithTimezone(value->GetList());
  } else {
    VLOG(2) << "Failed to get invalid timezones list";
  }

  // Can notify as ready now regardless of timezone fetching result.
  // We use default one picked from region list as a device region on failure.
  NotifyRegionDataReady();
}

void BraveVPNOSConnectionAPIBase::SetRegionListToPrefs() {
  DCHECK(!regions_.empty());

  base::Value::List regions_list;
  for (const auto& region : regions_) {
    regions_list.Append(GetValueFromRegion(region));
  }

  local_prefs_->Set(prefs::kBraveVPNRegionList,
                    base::Value(std::move(regions_list)));
  local_prefs_->SetTime(prefs::kBraveVPNRegionListFetchedDate,
                        base::Time::Now());
}

std::string BraveVPNOSConnectionAPIBase::GetCurrentTimeZone() {
  if (!test_timezone_.empty()) {
    return test_timezone_;
  }

  return GetTimeZoneName();
}

}  // namespace brave_vpn
