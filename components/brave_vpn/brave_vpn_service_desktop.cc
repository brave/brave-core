/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_desktop.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/brave_vpn/switches.h"
#include "brave/components/brave_vpn/url_constants.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace {

constexpr char kBraveVPNEntryName[] = "BraveVPN";

constexpr char kRegionContinentKey[] = "continent";
constexpr char kRegionNameKey[] = "name";
constexpr char kRegionNamePrettyKey[] = "name-pretty";

std::string GetManageUrl() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(brave_vpn::switches::kBraveVPNAccountHost))
    return brave_vpn::kManageUrlProd;

  const std::string value =
      cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNAccountHost);
  if (value == "prod")
    return brave_vpn::kManageUrlProd;
  if (value == "staging")
    return brave_vpn::kManageUrlStaging;
  if (value == "dev")
    return brave_vpn::kManageUrlDev;

  NOTREACHED();
  return brave_vpn::kManageUrlProd;
}

bool GetVPNCredentialsFromSwitch(brave_vpn::BraveVPNConnectionInfo* info) {
  DCHECK(info);
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(brave_vpn::switches::kBraveVPNTestCredentials))
    return false;

  std::string value =
      cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNTestCredentials);
  std::vector<std::string> tokens = base::SplitString(
      value, ":", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  if (tokens.size() == 4) {
    info->SetConnectionInfo(tokens[0], tokens[1], tokens[2], tokens[3]);
    return true;
  }

  LOG(ERROR) << __func__ << ": Invalid credentials";
  return false;
}

brave_vpn::BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI() {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(brave_vpn::switches::kBraveVPNSimulation))
    return brave_vpn::BraveVPNOSConnectionAPI::GetInstanceForTest();
  return brave_vpn::BraveVPNOSConnectionAPI::GetInstance();
}

}  // namespace

BraveVpnServiceDesktop::BraveVpnServiceDesktop(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : BraveVpnService(url_loader_factory), prefs_(prefs) {
  DCHECK(brave_vpn::IsBraveVPNEnabled());

  observed_.Observe(GetBraveVPNConnectionAPI());

  GetBraveVPNConnectionAPI()->set_target_vpn_entry_name(kBraveVPNEntryName);
  GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);

  LoadCachedRegionData();
  FetchRegionData();
  CheckPurchasedStatus();

  constexpr int kRegionDataUpdateIntervalInHours = 5;
  region_data_update_timer_.Start(
      FROM_HERE, base::TimeDelta::FromHours(kRegionDataUpdateIntervalInHours),
      this, &BraveVpnServiceDesktop::FetchRegionData);
}

BraveVpnServiceDesktop::~BraveVpnServiceDesktop() = default;

void BraveVpnServiceDesktop::Shutdown() {
  BraveVpnService::Shutdown();

  observed_.Reset();
  receivers_.Clear();
  observers_.Clear();
}

void BraveVpnServiceDesktop::OnCreated(const std::string& name) {
  for (const auto& obs : observers_)
    obs->OnConnectionCreated();
}

void BraveVpnServiceDesktop::OnRemoved(const std::string& name) {
  for (const auto& obs : observers_)
    obs->OnConnectionRemoved();
}

void BraveVpnServiceDesktop::OnConnected(const std::string& name) {
  if (connection_state_ == ConnectionState::CONNECTED)
    return;

  connection_state_ = ConnectionState::CONNECTED;

  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(ConnectionState::CONNECTED);
}

void BraveVpnServiceDesktop::OnIsConnecting(const std::string& name) {
  if (connection_state_ == ConnectionState::CONNECTING)
    return;

  connection_state_ = ConnectionState::CONNECTING;

  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(ConnectionState::CONNECTING);
}

void BraveVpnServiceDesktop::OnConnectFailed(const std::string& name) {
  if (connection_state_ == ConnectionState::CONNECT_FAILED)
    return;

  connection_state_ = ConnectionState::CONNECT_FAILED;

  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(ConnectionState::CONNECT_FAILED);
}

void BraveVpnServiceDesktop::OnDisconnected(const std::string& name) {
  if (connection_state_ == ConnectionState::DISCONNECTED)
    return;

  connection_state_ = ConnectionState::DISCONNECTED;

  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(ConnectionState::DISCONNECTED);
}

void BraveVpnServiceDesktop::OnIsDisconnecting(const std::string& name) {
  if (connection_state_ == ConnectionState::DISCONNECTING)
    return;

  connection_state_ = ConnectionState::DISCONNECTING;

  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(ConnectionState::DISCONNECTING);
}

void BraveVpnServiceDesktop::CreateVPNConnection() {
  GetBraveVPNConnectionAPI()->CreateVPNConnection(GetConnectionInfo());
}

void BraveVpnServiceDesktop::RemoveVPNConnnection() {
  GetBraveVPNConnectionAPI()->RemoveVPNConnection(
      GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Connect() {
  if (connection_state_ == ConnectionState::CONNECTING)
    return;

  GetBraveVPNConnectionAPI()->Connect(GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::Disconnect() {
  if (connection_state_ == ConnectionState::DISCONNECTING)
    return;

  GetBraveVPNConnectionAPI()->Disconnect(GetConnectionInfo().connection_name());
}

void BraveVpnServiceDesktop::CheckPurchasedStatus() {
  brave_vpn::BraveVPNConnectionInfo info;
  if (GetVPNCredentialsFromSwitch(&info)) {
    SetPurchasedState(PurchasedState::PURCHASED);
    CreateVPNConnection();
    return;
  }

  NOTIMPLEMENTED();
}

void BraveVpnServiceDesktop::ToggleConnection() {
  const bool can_disconnect =
      (connection_state_ == ConnectionState::CONNECTED ||
       connection_state_ == ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

void BraveVpnServiceDesktop::AddObserver(
    mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

brave_vpn::BraveVPNConnectionInfo BraveVpnServiceDesktop::GetConnectionInfo() {
  brave_vpn::BraveVPNConnectionInfo info;
  if (GetVPNCredentialsFromSwitch(&info))
    return info;

  // TODO(simonhong): Get real credentials from payment service.
  NOTIMPLEMENTED();
  return info;
}

void BraveVpnServiceDesktop::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveVpnServiceDesktop::GetConnectionState(
    GetConnectionStateCallback callback) {
  std::move(callback).Run(connection_state_);
}

void BraveVpnServiceDesktop::GetPurchasedState(
    GetPurchasedStateCallback callback) {
  std::move(callback).Run(purchased_state_);
}

void BraveVpnServiceDesktop::FetchRegionData() {
  // Unretained is safe here becasue this class owns request helper.
  GetAllServerRegions(base::BindOnce(&BraveVpnServiceDesktop::OnFetchRegionList,
                                     base::Unretained(this)));
}

void BraveVpnServiceDesktop::LoadCachedRegionData() {
  auto* preference =
      prefs_->FindPreference(brave_vpn::prefs::kBraveVPNRegionList);
  if (preference && !preference->IsDefaultValue())
    ParseAndCacheRegionList(preference->GetValue()->Clone());

  preference = prefs_->FindPreference(brave_vpn::prefs::kBraveVPNDeviceRegion);
  if (preference && !preference->IsDefaultValue()) {
    auto* region_value = preference->GetValue();
    const std::string* continent =
        region_value->FindStringKey(kRegionContinentKey);
    const std::string* name = region_value->FindStringKey(kRegionNameKey);
    const std::string* name_pretty =
        region_value->FindStringKey(kRegionNamePrettyKey);
    if (continent && name && name_pretty) {
      device_region_.continent = *continent;
      device_region_.name = *name;
      device_region_.name_pretty = *name_pretty;
    }
  }
}

void BraveVpnServiceDesktop::OnFetchRegionList(const std::string& region_list,
                                               bool success) {
  if (!success) {
    // TODO(simonhong): Re-try?
    VLOG(2) << "Failed to get region list";
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(region_list);
  if (value && value->is_list()) {
    prefs_->Set(brave_vpn::prefs::kBraveVPNRegionList, *value);

    if (ParseAndCacheRegionList(std::move(*value))) {
      // Fetch timezones list to determine default region of this device.
      GetTimezonesForRegions(base::BindOnce(
          &BraveVpnServiceDesktop::OnFetchTimezones, base::Unretained(this)));
      return;
    }
  }

  // TODO(simonhong): Re-try?
}

bool BraveVpnServiceDesktop::ParseAndCacheRegionList(base::Value region_value) {
  DCHECK(region_value.is_list());
  if (!region_value.is_list())
    return false;

  regions_.clear();
  for (const auto& value : region_value.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;

    brave_vpn::mojom::Region region;
    if (auto* continent = value.FindStringKey(kRegionContinentKey))
      region.continent = *continent;
    if (auto* name = value.FindStringKey(kRegionNameKey))
      region.name = *name;
    if (auto* name_pretty = value.FindStringKey(kRegionNamePrettyKey))
      region.name_pretty = *name_pretty;

    regions_.push_back(region);
  }

  if (regions_.empty())
    return false;

  return true;
}

void BraveVpnServiceDesktop::OnFetchTimezones(const std::string& timezones_list,
                                              bool success) {
  if (!success) {
    // TODO(simonhong): Re-try?
    VLOG(2) << "Failed to get timezones list";
    SetFallbackDeviceRegion();
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(timezones_list);
  if (value && value->is_list()) {
    ParseAndCacheDeviceRegionName(std::move(*value));
    return;
  }

  // TODO(simonhong): Re-try?
  SetFallbackDeviceRegion();
}

void BraveVpnServiceDesktop::ParseAndCacheDeviceRegionName(
    base::Value timezones_value) {
  DCHECK(timezones_value.is_list());

  if (!timezones_value.is_list()) {
    SetFallbackDeviceRegion();
    return;
  }

  const std::string current_time_zone = GetCurrentTimeZone();
  if (current_time_zone.empty()) {
    SetFallbackDeviceRegion();
    return;
  }

  for (const auto& timezones : timezones_value.GetList()) {
    DCHECK(timezones.is_dict());
    if (!timezones.is_dict())
      continue;

    const std::string* region_name = timezones.FindStringKey("name");
    if (!region_name)
      continue;
    std::string default_region_name_candidate = *region_name;
    const base::Value* timezone_list_value = timezones.FindKey("timezones");
    if (!timezone_list_value || !timezone_list_value->is_list())
      continue;

    for (const auto& timezone : timezone_list_value->GetList()) {
      DCHECK(timezone.is_string());
      if (!timezone.is_string())
        continue;
      if (current_time_zone == timezone.GetString()) {
        SetDeviceRegion(*region_name);
        VLOG(2) << "Found default region: " << device_region_.name;
        return;
      }
    }
  }

  SetFallbackDeviceRegion();
}

void BraveVpnServiceDesktop::SetDeviceRegion(const std::string& name) {
  auto it =
      std::find_if(regions_.begin(), regions_.end(),
                   [&name](const auto& region) { return region.name == name; });
  if (it != regions_.end()) {
    SetDeviceRegion(*it);
  }
}

void BraveVpnServiceDesktop::SetFallbackDeviceRegion() {
  // Set first item in the region list as a |device_region_| as a fallback.
  DCHECK(!regions_.empty());
  if (regions_.empty())
    return;

  SetDeviceRegion(regions_[0]);
}

void BraveVpnServiceDesktop::SetDeviceRegion(
    const brave_vpn::mojom::Region& region) {
  device_region_ = region;

  DictionaryPrefUpdate update(prefs_, brave_vpn::prefs::kBraveVPNDeviceRegion);
  base::Value* dict = update.Get();
  dict->SetStringKey(kRegionContinentKey, device_region_.continent);
  dict->SetStringKey(kRegionNameKey, device_region_.name);
  dict->SetStringKey(kRegionNamePrettyKey, device_region_.name_pretty);
}

std::string BraveVpnServiceDesktop::GetCurrentTimeZone() {
  if (!test_timezone_.empty())
    return test_timezone_;

  std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());
  icu::UnicodeString id;
  zone->getID(id);
  std::string current_time_zone;
  id.toUTF8String<std::string>(current_time_zone);
  return current_time_zone;
}

void BraveVpnServiceDesktop::GetAllRegions(GetAllRegionsCallback callback) {
  std::vector<brave_vpn::mojom::RegionPtr> regions;
  for (const auto& region : regions_) {
    regions.push_back(region.Clone());
  }
  std::move(callback).Run(std::move(regions));
}

void BraveVpnServiceDesktop::GetDeviceRegion(GetDeviceRegionCallback callback) {
  std::move(callback).Run(device_region_.Clone());
}

void BraveVpnServiceDesktop::GetSelectedRegion(
    GetSelectedRegionCallback callback) {
  auto* preference =
      prefs_->FindPreference(brave_vpn::prefs::kBraveVPNSelectedRegion);
  if (preference->IsDefaultValue()) {
    // Gives device region if there is no cached selected region.
    std::move(callback).Run(device_region_.Clone());
    return;
  }

  auto* region_value = preference->GetValue();
  const std::string* continent =
      region_value->FindStringKey(kRegionContinentKey);
  const std::string* name = region_value->FindStringKey(kRegionNameKey);
  const std::string* name_pretty =
      region_value->FindStringKey(kRegionNamePrettyKey);
  if (!continent || !name || !name_pretty) {
    // Gives device region if invalid data is cached.
    std::move(callback).Run(device_region_.Clone());
    return;
  }

  brave_vpn::mojom::Region region(*continent, *name, *name_pretty);
  std::move(callback).Run(region.Clone());
}

void BraveVpnServiceDesktop::SetSelectedRegion(
    brave_vpn::mojom::RegionPtr region_ptr) {
  DictionaryPrefUpdate update(prefs_,
                              brave_vpn::prefs::kBraveVPNSelectedRegion);
  base::Value* dict = update.Get();
  dict->SetStringKey(kRegionContinentKey, region_ptr->continent);
  dict->SetStringKey(kRegionNameKey, region_ptr->name);
  dict->SetStringKey(kRegionNamePrettyKey, region_ptr->name_pretty);

  // Start hostname fetching for selected region.
  FetchHostnamesForRegion(region_ptr->name);
}

void BraveVpnServiceDesktop::GetProductUrls(GetProductUrlsCallback callback) {
  brave_vpn::mojom::ProductUrls urls;
  urls.feedback = brave_vpn::kFeedbackUrl;
  urls.about = brave_vpn::kAboutUrl;
  urls.manage = GetManageUrl();
  std::move(callback).Run(urls.Clone());
}

void BraveVpnServiceDesktop::FetchHostnamesForRegion(const std::string& name) {
  // Unretained is safe here becasue this class owns request helper.
  GetHostnamesForRegion(
      base::BindOnce(&BraveVpnServiceDesktop::OnFetchHostnames,
                     base::Unretained(this), name),
      name);
}

void BraveVpnServiceDesktop::OnFetchHostnames(const std::string& region,
                                              const std::string& hostnames,
                                              bool success) {
  if (!success) {
    // TODO(simonhong): Retry?
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, std::move(*value));
    return;
  }

  // TODO(simonhong): Retry?
}

void BraveVpnServiceDesktop::ParseAndCacheHostnames(
    const std::string& region,
    base::Value hostnames_value) {
  DCHECK(hostnames_value.is_list());
  if (!hostnames_value.is_list())
    return;

  constexpr char kHostnameKey[] = "hostname";
  constexpr char kDisplayNameKey[] = "display-name";
  constexpr char kOfflineKey[] = "offline";
  constexpr char kCapacityScoreKey[] = "capacity-score";
  std::vector<brave_vpn::Hostname> hostnames;
  for (const auto& value : hostnames_value.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;

    const std::string* hostname_str = value.FindStringKey(kHostnameKey);
    const std::string* display_name_str = value.FindStringKey(kDisplayNameKey);
    absl::optional<bool> offline = value.FindBoolKey(kOfflineKey);
    absl::optional<int> capacity_score = value.FindIntKey(kCapacityScoreKey);

    if (!hostname_str || !display_name_str || !offline || !capacity_score)
      continue;

    hostnames.push_back(brave_vpn::Hostname{*hostname_str, *display_name_str,
                                            *offline, *capacity_score});
  }

  hostnames_[region] = std::move(hostnames);
}

void BraveVpnServiceDesktop::SetPurchasedState(PurchasedState state) {
  if (purchased_state_ == state)
    return;

  purchased_state_ = state;

  for (const auto& obs : observers_)
    obs->OnPurchasedStateChanged(purchased_state_);
}
