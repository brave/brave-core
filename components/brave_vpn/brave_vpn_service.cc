/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service.h"

#include <algorithm>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/parsed_cookie.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/url_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/brave_vpn/switches.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#endif  // !BUILDFLAG(IS_ANDROID)

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

namespace {

#if !BUILDFLAG(IS_ANDROID)
constexpr char kBraveVPNEntryName[] = "BraveVPN";

constexpr char kRegionContinentKey[] = "continent";
constexpr char kRegionNameKey[] = "name";
constexpr char kRegionNamePrettyKey[] = "name-pretty";

constexpr char kCreateSupportTicket[] = "api/v1.2/partners/support-ticket";

bool IsValidRegionValue(const base::Value& value) {
  if (!value.FindStringKey(kRegionContinentKey) ||
      !value.FindStringKey(kRegionNameKey) ||
      !value.FindStringKey(kRegionNamePrettyKey)) {
    return false;
  }

  return true;
}

// On desktop, the environment is tied to SKUs because you would purchase it
// from `account.brave.com` (or similar, based on env). The credentials for VPN
// will always be in the same environment as the SKU environment.
//
// When the vendor receives a credential from us during auth, it also includes
// the environment. The vendor then can do a lookup using Payment Service.
std::string GetBraveVPNPaymentsEnv() {
  const std::string env = skus::GetEnvironment();
  if (env == skus::kEnvProduction)
    return "";
  // Use same value.
  if (env == skus::kEnvStaging || env == skus::kEnvDevelopment)
    return env;

  NOTREACHED();

#if defined(OFFICIAL_BUILD)
  return "";
#else
  return "development";
#endif
}

#endif  // !BUILDFLAG(IS_ANDROID)

constexpr char kVpnHost[] = "connect-api.guardianapp.com";

constexpr char kAllServerRegions[] = "api/v1/servers/all-server-regions";
constexpr char kTimezonesForRegions[] =
    "api/v1.1/servers/timezones-for-regions";
constexpr char kHostnameForRegion[] = "api/v1/servers/hostnames-for-region";
constexpr char kCreateSubscriberCredential[] =
    "api/v1/subscriber-credential/create";
constexpr char kProfileCredential[] = "api/v1.1/register-and-create";
constexpr char kVerifyPurchaseToken[] = "api/v1.1/verify-purchase-token";
constexpr char kCreateSubscriberCredentialV12[] =
    "api/v1.2/subscriber-credential/create";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_vpn_service", R"(
      semantics {
        sender: "Brave VPN Service"
        description:
          "This service is used to communicate with Guardian VPN apis"
          "on behalf of the user interacting with the Brave VPN."
        trigger:
          "Triggered by user connecting the Brave VPN."
        data:
          "Servers, hosts and credentials for Brave VPN"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

GURL GetURLWithPath(const std::string& host, const std::string& path) {
  return GURL(std::string(url::kHttpsScheme) + "://" + host).Resolve(path);
}

std::string CreateJSONRequestBody(const base::Value& dict) {
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

std::string GetSubscriberCredentialFromJson(const std::string& json) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    VLOG(1) << __func__ << "Invalid response, could not parse JSON.";
    return "";
  }

  const base::DictionaryValue* response_dict;
  if (!records_v->GetAsDictionary(&response_dict)) {
    return "";
  }

  const base::Value* subscriber_credential =
      records_v->FindKey("subscriber-credential");
  return subscriber_credential == nullptr ? ""
                                          : subscriber_credential->GetString();
}

}  // namespace

#if BUILDFLAG(IS_ANDROID)
BraveVpnService::BraveVpnService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter)
    : skus_service_getter_(skus_service_getter),
      api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      weak_ptr_factory_(this) {}
#else
BraveVpnService::BraveVpnService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter)
    : prefs_(prefs),
      skus_service_getter_(skus_service_getter),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {
  DCHECK(brave_vpn::IsBraveVPNEnabled());

  auto* cmd = base::CommandLine::ForCurrentProcess();
  is_simulation_ = cmd->HasSwitch(brave_vpn::switches::kBraveVPNSimulation);
  observed_.Observe(GetBraveVPNConnectionAPI());

  GetBraveVPNConnectionAPI()->set_target_vpn_entry_name(kBraveVPNEntryName);
}
#endif

BraveVpnService::~BraveVpnService() {
#if !BUILDFLAG(IS_ANDROID)
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#endif  // !BUILDFLAG(IS_ANDROID)
}

#if !BUILDFLAG(IS_ANDROID)
void BraveVpnService::ScheduleBackgroundRegionDataFetch() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (region_data_update_timer_.IsRunning())
    return;

  // Try to update region list every 5h.
  constexpr int kRegionDataUpdateIntervalInHours = 5;
  region_data_update_timer_.Start(
      FROM_HERE, base::Hours(kRegionDataUpdateIntervalInHours),
      base::BindRepeating(&BraveVpnService::FetchRegionData,
                          base::Unretained(this), true));
}

void BraveVpnService::OnCreated() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  for (const auto& obs : observers_)
    obs->OnConnectionCreated();

  // It's time to ask connecting to os after vpn entry is created.
  GetBraveVPNConnectionAPI()->Connect(GetConnectionInfo().connection_name());
}

void BraveVpnService::OnCreateFailed() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  // Clear connecting cancel request.
  if (cancel_connecting_)
    cancel_connecting_ = false;

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_NOT_ALLOWED);
}

void BraveVpnService::OnRemoved() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  for (const auto& obs : observers_)
    obs->OnConnectionRemoved();
}

void BraveVpnService::UpdateAndNotifyConnectionStateChange(
    ConnectionState state,
    bool force) {
  // this is a simple state machine for handling connection state
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == state)
    return;

  // On Windows, we get disconnected status update twice.
  // When user connects to different region while connected,
  // we disconnect current connection and connect to newly selected
  // region. To do that we monitor |DISCONNECTED| state and start
  // connect when we get that state. But, Windows sends disconnected state
  // noti again. So, ignore second one.
  // On exception - we allow from connecting to disconnected in canceling
  // scenario.
  if (connection_state_ == ConnectionState::CONNECTING &&
      state == ConnectionState::DISCONNECTED && !cancel_connecting_) {
    VLOG(2) << __func__ << ": Ignore disconnected state while connecting";
    return;
  }

  // On Windows, we could get disconnected state after connect failed.
  // To make connect failed state as a last state, ignore disconnected state.
  if (!force && connection_state_ == ConnectionState::CONNECT_FAILED &&
      state == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << ": Ignore disconnected state after connect failed";
    return;
  }

  VLOG(2) << __func__ << " : changing from " << connection_state_ << " to "
          << state;

  connection_state_ = state;
  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(connection_state_);
}

void BraveVpnService::OnConnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  if (cancel_connecting_) {
    // As connect is done, we don't need more for cancelling.
    // Just start normal Disconenct() process.
    cancel_connecting_ = false;
    GetBraveVPNConnectionAPI()->Disconnect(kBraveVPNEntryName);
    return;
  }

  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTED);
}

void BraveVpnService::OnIsConnecting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  if (!cancel_connecting_)
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);
}

void BraveVpnService::OnConnectFailed() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  cancel_connecting_ = false;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVpnService::OnDisconnected() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);

  if (needs_connect_) {
    needs_connect_ = false;
    Connect();
  }
}

void BraveVpnService::OnIsDisconnecting() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVpnService::CreateVPNConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  VLOG(2) << __func__;
  GetBraveVPNConnectionAPI()->CreateVPNConnection(GetConnectionInfo());
}

void BraveVpnService::RemoveVPNConnnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  GetBraveVPNConnectionAPI()->RemoveVPNConnection(kBraveVPNEntryName);
}

void BraveVpnService::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == ConnectionState::DISCONNECTING ||
      connection_state_ == ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << ": Current state: " << connection_state_
            << " : prevent connecting while previous operation is in-progress";
    return;
  }

  DCHECK(!cancel_connecting_);

  // User can ask connect again when user want to change region.
  if (connection_state_ == ConnectionState::CONNECTED) {
    // Disconnect first and then create again to setup for new region.
    needs_connect_ = true;
    Disconnect();
    return;
  }

  VLOG(2) << __func__ << " : start connecting!";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECTING);

  if (is_simulation_ || connection_info_.IsValid()) {
    VLOG(2) << __func__
            << " : direct connect as we already have valid connection info.";
    GetBraveVPNConnectionAPI()->Connect(GetConnectionInfo().connection_name());
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

void BraveVpnService::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == ConnectionState::DISCONNECTED) {
    VLOG(2) << __func__ << " : already disconnected";
    return;
  }

  if (connection_state_ == ConnectionState::DISCONNECTING) {
    VLOG(2) << __func__ << " : disconnecting in progress";
    return;
  }

  if (is_simulation_ || connection_state_ != ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << " : start disconnecting!";
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
    GetBraveVPNConnectionAPI()->Disconnect(kBraveVPNEntryName);
    return;
  }

  cancel_connecting_ = true;
  VLOG(2) << __func__ << " : Start cancelling connect request";
  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTING);
}

void BraveVpnService::ToggleConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const bool can_disconnect =
      (connection_state_ == ConnectionState::CONNECTED ||
       connection_state_ == ConnectionState::CONNECTING);
  can_disconnect ? Disconnect() : Connect();
}

const brave_vpn::BraveVPNConnectionInfo& BraveVpnService::GetConnectionInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return connection_info_;
}

void BraveVpnService::BindInterface(
    mojo::PendingReceiver<brave_vpn::mojom::ServiceHandler> receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receivers_.Add(this, std::move(receiver));
}

void BraveVpnService::GetConnectionState(GetConnectionStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__ << " : " << connection_state_;
  std::move(callback).Run(connection_state_);
}

void BraveVpnService::ResetConnectionState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Reset is allowed only when it has failed state.
  if (connection_state_ != ConnectionState::CONNECT_NOT_ALLOWED &&
      connection_state_ != ConnectionState::CONNECT_FAILED)
    return;

  UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED, true);
}

void BraveVpnService::FetchRegionData(bool background_fetch) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Only do background fetching for purchased user.
  if (background_fetch && !is_purchased_user())
    return;

  VLOG(2) << __func__
          << (background_fetch ? " : Start fetching region data in background"
                               : " : Start fetching region data");

  // Unretained is safe here becasue this class owns request helper.
  GetAllServerRegions(base::BindOnce(&BraveVpnService::OnFetchRegionList,
                                     base::Unretained(this), background_fetch));
}

void BraveVpnService::LoadCachedRegionData() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(PurchasedState::LOADING, purchased_state_);

  // Already loaded from cache.
  if (!regions_.empty())
    return;

  // Empty device region means it's initial state.
  if (GetDeviceRegion().empty())
    return;

  auto* preference =
      prefs_->FindPreference(brave_vpn::prefs::kBraveVPNRegionList);
  DCHECK(preference);
  // Early return when we don't have any cached region data.
  if (preference->IsDefaultValue())
    return;

  // If cached one is outdated, don't use it.
  if (!ValidateCachedRegionData(*preference->GetValue())) {
    VLOG(2) << __func__ << " : Cached data is outdate. Will get fetch latest.";
    return;
  }

  if (ParseAndCacheRegionList(*preference->GetValue())) {
    VLOG(2) << __func__ << " : Loaded cached region list";
    return;
  }

  VLOG(2) << __func__ << " : Failed to load cached region list";
}

void BraveVpnService::SetRegionListToPrefs() {
  DCHECK(!regions_.empty());

  base::Value regions_list(base::Value::Type::LIST);
  for (const auto& region : regions_) {
    regions_list.Append(GetValueFromRegion(region));
  }
  prefs_->Set(brave_vpn::prefs::kBraveVPNRegionList, std::move(regions_list));
}

void BraveVpnService::OnFetchRegionList(bool background_fetch,
                                        const std::string& region_list,
                                        bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Don't update purchased state during the background fetching.

  if (!background_fetch && !success) {
    VLOG(2) << "Failed to get region list";
    SetPurchasedState(PurchasedState::FAILED);
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(region_list);
  if (value && value->is_list()) {
    if (background_fetch) {
      ParseAndCacheRegionList(*value, true);
      return;
    }

    if (ParseAndCacheRegionList(*value, true)) {
      VLOG(2) << "Got valid region list";
      // Set default device region and it'll be updated when received valid
      // timezone info.
      SetFallbackDeviceRegion();
      // Fetch timezones list to determine default region of this device.
      GetTimezonesForRegions(base::BindOnce(&BraveVpnService::OnFetchTimezones,
                                            base::Unretained(this)));
      return;
    }
  }

  // Don't update purchased state during the background fetching.
  if (!background_fetch) {
    VLOG(2) << "Got invalid region list";
    SetPurchasedState(PurchasedState::FAILED);
  }
}

bool BraveVpnService::ValidateCachedRegionData(
    const base::Value& region_value) const {
  for (const auto& value : region_value.GetList()) {
    // Make sure cached one has all latest properties.
    if (!IsValidRegionValue(value))
      return false;
  }

  return true;
}

base::Value BraveVpnService::GetValueFromRegion(
    const brave_vpn::mojom::Region& region) const {
  base::Value region_dict(base::Value::Type::DICTIONARY);
  region_dict.SetStringKey(kRegionContinentKey, region.continent);
  region_dict.SetStringKey(kRegionNameKey, region.name);
  region_dict.SetStringKey(kRegionNamePrettyKey, region.name_pretty);
  return region_dict;
}

brave_vpn::mojom::Region BraveVpnService::GetRegionFromValue(
    const base::Value& value) const {
  brave_vpn::mojom::Region region;
  if (auto* continent = value.FindStringKey(kRegionContinentKey))
    region.continent = *continent;
  if (auto* name = value.FindStringKey(kRegionNameKey))
    region.name = *name;
  if (auto* name_pretty = value.FindStringKey(kRegionNamePrettyKey))
    region.name_pretty = *name_pretty;
  return region;
}

bool BraveVpnService::ParseAndCacheRegionList(const base::Value& region_value,
                                              bool save_to_prefs) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(region_value.is_list());
  if (!region_value.is_list())
    return false;

  regions_.clear();
  for (const auto& value : region_value.GetList()) {
    DCHECK(value.is_dict());
    if (!value.is_dict())
      continue;
    regions_.push_back(GetRegionFromValue(value));
  }

  // Sort region list alphabetically
  std::sort(regions_.begin(), regions_.end(),
            [](brave_vpn::mojom::Region& a, brave_vpn::mojom::Region& b) {
              return (a.name_pretty < b.name_pretty);
            });

  VLOG(2) << __func__ << " : has regionlist: " << !regions_.empty();

  // If we can't get region list, we can't determine device region.
  if (regions_.empty())
    return false;

  if (save_to_prefs)
    SetRegionListToPrefs();
  return true;
}

void BraveVpnService::OnFetchTimezones(const std::string& timezones_list,
                                       bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  absl::optional<base::Value> value = base::JSONReader::Read(timezones_list);
  if (success && value && value->is_list()) {
    VLOG(2) << "Got valid timezones list";
    SetDeviceRegionWithTimezone(*value);
  } else {
    VLOG(2) << "Failed to get invalid timezones list";
  }

  // Can set as purchased state now regardless of timezone fetching result.
  // We use default one picked from region list as a device region on failure.
  SetPurchasedState(PurchasedState::PURCHASED);
}

void BraveVpnService::SetDeviceRegionWithTimezone(
    const base::Value& timezones_value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(timezones_value.is_list());

  const std::string current_time_zone = GetCurrentTimeZone();
  if (current_time_zone.empty())
    return;

  for (const auto& timezones : timezones_value.GetList()) {
    DCHECK(timezones.is_dict());
    if (!timezones.is_dict())
      continue;

    const std::string* region_name = timezones.FindStringKey("name");
    if (!region_name)
      continue;
    const base::Value* timezone_list_value = timezones.FindKey("timezones");
    if (!timezone_list_value || !timezone_list_value->is_list())
      continue;

    for (const auto& timezone : timezone_list_value->GetList()) {
      DCHECK(timezone.is_string());
      if (!timezone.is_string())
        continue;
      if (current_time_zone == timezone.GetString()) {
        VLOG(2) << "Found default region: " << *region_name;
        SetDeviceRegion(*region_name);
        return;
      }
    }
  }
}

void BraveVpnService::SetDeviceRegion(const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  prefs_->SetString(brave_vpn::prefs::kBraveVPNDeviceRegion, name);
}

void BraveVpnService::SetSelectedRegion(const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  prefs_->SetString(brave_vpn::prefs::kBraveVPNSelectedRegion, name);
}

std::string BraveVpnService::GetDeviceRegion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return prefs_->GetString(brave_vpn::prefs::kBraveVPNDeviceRegion);
}

std::string BraveVpnService::GetSelectedRegion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return prefs_->GetString(brave_vpn::prefs::kBraveVPNSelectedRegion);
}

void BraveVpnService::SetFallbackDeviceRegion() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Set first item in the region list as a |device_region_| as a fallback.
  DCHECK(!regions_.empty());
  SetDeviceRegion(regions_[0].name);
}

std::string BraveVpnService::GetCurrentTimeZone() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!test_timezone_.empty())
    return test_timezone_;

  std::unique_ptr<icu::TimeZone> zone(icu::TimeZone::createDefault());
  icu::UnicodeString id;
  zone->getID(id);
  std::string current_time_zone;
  id.toUTF8String<std::string>(current_time_zone);
  return current_time_zone;
}

void BraveVpnService::GetAllRegions(GetAllRegionsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<brave_vpn::mojom::RegionPtr> regions;
  for (const auto& region : regions_) {
    regions.push_back(region.Clone());
  }
  std::move(callback).Run(std::move(regions));
}

brave_vpn::mojom::Region BraveVpnService::GetRegionWithName(
    const std::string& name) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  brave_vpn::mojom::Region region;
  auto it =
      std::find_if(regions_.begin(), regions_.end(),
                   [&name](const auto& region) { return region.name == name; });
  if (it != regions_.end())
    region = *it;
  return region;
}

void BraveVpnService::GetDeviceRegion(GetDeviceRegionCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  auto region_name = GetDeviceRegion();
  DCHECK(!region_name.empty());
  auto region = GetRegionWithName(region_name);
  std::move(callback).Run(region.Clone());
}

void BraveVpnService::GetSelectedRegion(GetSelectedRegionCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  auto region_name = GetSelectedRegion();
  if (region_name.empty()) {
    // Gives device region if there is no cached selected region.
    VLOG(2) << __func__ << " : give device region instead.";
    region_name = GetDeviceRegion();
  }
  DCHECK(!region_name.empty());
  auto region = GetRegionWithName(region_name);
  VLOG(2) << __func__ << " : Give " << region.name_pretty;
  std::move(callback).Run(region.Clone());
}

void BraveVpnService::SetSelectedRegion(
    brave_vpn::mojom::RegionPtr region_ptr) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (connection_state_ == ConnectionState::DISCONNECTING ||
      connection_state_ == ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << ": Current state: " << connection_state_
            << " : prevent changing selected region while previous operation "
               "is in-progress";
    return;
  }

  VLOG(2) << __func__ << " : " << region_ptr->name_pretty;
  SetSelectedRegion(region_ptr->name);

  // As new selected region is used, |connection_info_| for previous selected
  // should be cleared.
  connection_info_.Reset();
}

void BraveVpnService::GetProductUrls(GetProductUrlsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(brave_vpn::mojom::ProductUrls::New(
      brave_vpn::kFeedbackUrl, brave_vpn::kAboutUrl,
      brave_vpn::GetManageUrl()));
}

void BraveVpnService::CreateSupportTicket(
    const std::string& email,
    const std::string& subject,
    const std::string& body,
    CreateSupportTicketCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnCreateSupportTicket,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  const GURL base_url = GetURLWithPath(kVpnHost, kCreateSupportTicket);
  base::Value dict(base::Value::Type::DICTIONARY);

  std::string email_trimmed, subject_trimmed, body_trimmed, body_encoded;
  base::TrimWhitespaceASCII(email, base::TRIM_ALL, &email_trimmed);
  base::TrimWhitespaceASCII(subject, base::TRIM_ALL, &subject_trimmed);
  base::TrimWhitespaceASCII(body, base::TRIM_ALL, &body_trimmed);
  base::Base64Encode(body_trimmed, &body_encoded);

  // required fields
  dict.SetStringKey("email", email_trimmed);
  dict.SetStringKey("subject", subject_trimmed);
  dict.SetStringKey("support-ticket", body_encoded);
  dict.SetStringKey("partner-client-id", "com.brave.browser");

  // optional (but encouraged) fields
  dict.SetStringKey("subscriber-credential", "");
  dict.SetStringKey("payment-validation-method", "brave-premium");
  dict.SetStringKey("payment-validation-data", "");

  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::GetSupportData(GetSupportDataCallback callback) {
  std::string brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  std::string os_version = version_info::GetOSType();
  std::string vpn_hostname = hostname_ ? hostname_->display_name : "";
  std::move(callback).Run(brave_version, os_version, vpn_hostname);
}

void BraveVpnService::FetchHostnamesForRegion(const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  // Hostname will be replaced with latest one.
  hostname_.reset();

  // Unretained is safe here becasue this class owns request helper.
  GetHostnamesForRegion(base::BindOnce(&BraveVpnService::OnFetchHostnames,
                                       base::Unretained(this), name),
                        name);
}

void BraveVpnService::OnFetchHostnames(const std::string& region,
                                       const std::string& hostnames,
                                       bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(hostnames);
  if (value && value->is_list()) {
    ParseAndCacheHostnames(region, *value);
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVpnService::ParseAndCacheHostnames(
    const std::string& region,
    const base::Value& hostnames_value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(hostnames_value.is_list());
  if (!hostnames_value.is_list()) {
    VLOG(2) << __func__ << " : failed to parse hostnames for " << region;
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

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

  VLOG(2) << __func__ << " : has hostname: " << !hostnames.empty();

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

  if (skus_credential_.empty()) {
    VLOG(2) << __func__ << " : skus_credential is empty";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  // Get subscriber credentials and then get EAP credentials with it to create
  // OS VPN entry.
  VLOG(2) << __func__ << " : request subscriber credential";
  GetSubscriberCredentialV12(
      base::BindOnce(&BraveVpnService::OnGetSubscriberCredentialV12,
                     base::Unretained(this)),
      GetBraveVPNPaymentsEnv(), skus_credential_);
}

void BraveVpnService::OnGetSubscriberCredentialV12(
    const std::string& subscriber_credential,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to get subscriber credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : received subscriber credential";
  // TODO(bsclifton): consider storing `subscriber_credential` for
  // support ticket use-case (see `CreateSupportTicket`).
  GetProfileCredentials(
      base::BindOnce(&BraveVpnService::OnGetProfileCredentials,
                     base::Unretained(this)),
      subscriber_credential, hostname_->hostname);
}

void BraveVpnService::OnGetProfileCredentials(
    const std::string& profile_credential,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (cancel_connecting_) {
    UpdateAndNotifyConnectionStateChange(ConnectionState::DISCONNECTED);
    cancel_connecting_ = false;
    return;
  }

  if (!success) {
    VLOG(2) << __func__ << " : failed to get profile credential";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  VLOG(2) << __func__ << " : received profile credential";

  absl::optional<base::Value> value =
      base::JSONReader::Read(profile_credential);
  if (value && value->is_dict()) {
    constexpr char kUsernameKey[] = "eap-username";
    constexpr char kPasswordKey[] = "eap-password";
    const std::string* username = value->FindStringKey(kUsernameKey);
    const std::string* password = value->FindStringKey(kPasswordKey);
    if (!username || !password) {
      VLOG(2) << __func__ << " : it's invalid profile credential";
      UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
      return;
    }

    connection_info_.SetConnectionInfo(kBraveVPNEntryName, hostname_->hostname,
                                       *username, *password);
    // Let's create os vpn entry with |connection_info_|.
    CreateVPNConnection();
    return;
  }

  VLOG(2) << __func__ << " : it's invalid profile credential";
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

std::unique_ptr<brave_vpn::Hostname> BraveVpnService::PickBestHostname(
    const std::vector<brave_vpn::Hostname>& hostnames) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<brave_vpn::Hostname> filtered_hostnames;
  std::copy_if(
      hostnames.begin(), hostnames.end(),
      std::back_inserter(filtered_hostnames),
      [](const brave_vpn::Hostname& hostname) { return !hostname.is_offline; });

  std::sort(filtered_hostnames.begin(), filtered_hostnames.end(),
            [](const brave_vpn::Hostname& a, const brave_vpn::Hostname& b) {
              return a.capacity_score > b.capacity_score;
            });

  if (filtered_hostnames.empty())
    return std::make_unique<brave_vpn::Hostname>();

  // Pick highest capacity score.
  return std::make_unique<brave_vpn::Hostname>(filtered_hostnames[0]);
}

brave_vpn::BraveVPNOSConnectionAPI*
BraveVpnService::GetBraveVPNConnectionAPI() {
  if (is_simulation_)
    return brave_vpn::BraveVPNOSConnectionAPI::GetInstanceForTest();
  return brave_vpn::BraveVPNOSConnectionAPI::GetInstance();
}

void BraveVpnService::OnCreateSupportTicket(
    CreateSupportTicketCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  bool success = status == 200;
  VLOG(2) << "OnCreateSupportTicket success=" << success
          << "\nresponse_code=" << status;
  std::move(callback).Run(success, body);
}
#endif  // !BUILDFLAG(IS_ANDROID)

void BraveVpnService::AddObserver(
    mojo::PendingRemote<brave_vpn::mojom::ServiceObserver> observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.Add(std::move(observer));
}

void BraveVpnService::GetPurchasedState(GetPurchasedStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__ << " : " << static_cast<int>(purchased_state_);
  std::move(callback).Run(purchased_state_);
}

void BraveVpnService::LoadPurchasedState() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (purchased_state_ == PurchasedState::LOADING)
    return;

  SetPurchasedState(PurchasedState::LOADING);

#if !BUILDFLAG(IS_ANDROID) && !defined(OFFICIAL_BUILD)
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(brave_vpn::switches::kBraveVPNTestMonthlyPass)) {
    skus_credential_ =
        cmd->GetSwitchValueASCII(brave_vpn::switches::kBraveVPNTestMonthlyPass);
    LoadCachedRegionData();
    if (!regions_.empty()) {
      SetPurchasedState(PurchasedState::PURCHASED);
    } else {
      FetchRegionData(false);
    }

    GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);
    return;
  }
#endif  // !BUILDFLAG(IS_ANDROID) && !defined(OFFICIAL_BUILD)

  EnsureMojoConnected();
  skus_service_->CredentialSummary(
      skus::GetDomain("vpn"),
      base::BindOnce(&BraveVpnService::OnCredentialSummary,
                     base::Unretained(this)));
}

void BraveVpnService::OnCredentialSummary(const std::string& summary_string) {
  std::string summary_string_trimmed;
  base::TrimWhitespaceASCII(summary_string, base::TrimPositions::TRIM_ALL,
                            &summary_string_trimmed);
  if (summary_string_trimmed.length() == 0) {
    // no credential found; person needs to login
    VLOG(1) << __func__ << " : No credential found; user needs to login!";
    SetPurchasedState(PurchasedState::NOT_PURCHASED);
    return;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          summary_string, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (records_v && records_v->is_dict()) {
    const base::Value* active = records_v->FindKey("active");
    bool has_credential = active && active->is_bool() && active->GetBool();
    if (has_credential) {
      VLOG(1) << __func__ << " : Active credential found!";
      // if a credential is ready, we can present it
      EnsureMojoConnected();
      skus_service_->PrepareCredentialsPresentation(
          skus::GetDomain("vpn"), "*",
          base::BindOnce(&BraveVpnService::OnPrepareCredentialsPresentation,
                         base::Unretained(this)));
    } else {
      VLOG(1) << __func__ << " : Credential appears to be expired.";
      SetPurchasedState(PurchasedState::EXPIRED);
    }
  } else {
    VLOG(1) << __func__ << " : Got invalid credential summary!";
    SetPurchasedState(PurchasedState::FAILED);
  }
}

void BraveVpnService::OnPrepareCredentialsPresentation(
    const std::string& credential_as_cookie) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Credential is returned in cookie format.
  net::CookieInclusionStatus status;
  net::ParsedCookie credential_cookie(credential_as_cookie, &status);
  // TODO(bsclifton): have a better check / logging.
  // should these failed states be considered NOT_PURCHASED?
  // or maybe it can be considered FAILED status?
  if (!credential_cookie.IsValid()) {
    VLOG(1) << __func__ << " : FAILED credential_cookie.IsValid";
    SetPurchasedState(PurchasedState::FAILED);
    return;
  }
  if (!status.IsInclude()) {
    VLOG(1) << __func__ << " : FAILED status.IsInclude";
    SetPurchasedState(PurchasedState::FAILED);
    return;
  }

  // Credential value received needs to be URL decoded.
  // That leaves us with a Base64 encoded JSON blob which is the credential.
  std::string encoded_credential = credential_cookie.Value();
  url::RawCanonOutputT<char16_t> unescaped;
  url::DecodeURLEscapeSequences(
      encoded_credential.data(), encoded_credential.size(),
      url::DecodeURLMode::kUTF8OrIsomorphic, &unescaped);
  std::string credential;
  base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &credential);

  skus_credential_ = credential;

  if (skus_credential_.empty()) {
    VLOG(2) << __func__ << " : Got empty skus credentials";
    SetPurchasedState(PurchasedState::NOT_PURCHASED);
    return;
  }

#if BUILDFLAG(IS_ANDROID)
  SetPurchasedState(PurchasedState::PURCHASED);
#else
  LoadCachedRegionData();

  // Only fetch when we don't have cache.
  if (!regions_.empty()) {
    SetPurchasedState(PurchasedState::PURCHASED);
  } else {
    FetchRegionData(false);
  }

  ScheduleBackgroundRegionDataFetch();
  GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);
#endif
}

void BraveVpnService::SetPurchasedState(PurchasedState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (purchased_state_ == state)
    return;

  purchased_state_ = state;

  switch (state) {
    case PurchasedState::NOT_PURCHASED:
      VLOG(1) << "SetPurchasedState(NOT_PURCHASED);";
      break;
    case PurchasedState::PURCHASED:
      VLOG(1) << "SetPurchasedState(PURCHASED);";
      break;
    case PurchasedState::EXPIRED:
      VLOG(1) << "SetPurchasedState(EXPIRED);";
      break;
    case PurchasedState::LOADING:
      VLOG(1) << "SetPurchasedState(LOADING);";
      break;
    case PurchasedState::FAILED:
      VLOG(1) << "SetPurchasedState(FAILED);";
      break;
  }

  for (const auto& obs : observers_)
    obs->OnPurchasedStateChanged(purchased_state_);
}

void BraveVpnService::EnsureMojoConnected() {
  if (!skus_service_) {
    auto pending = skus_service_getter_.Run();
    skus_service_.Bind(std::move(pending));
  }
  DCHECK(skus_service_);
  skus_service_.set_disconnect_handler(base::BindOnce(
      &BraveVpnService::OnMojoConnectionError, base::Unretained(this)));
}

void BraveVpnService::OnMojoConnectionError() {
  skus_service_.reset();
  EnsureMojoConnected();
}

void BraveVpnService::Shutdown() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  skus_service_.reset();
  observers_.Clear();

#if !BUILDFLAG(IS_ANDROID)
  observed_.Reset();
  receivers_.Clear();
#endif  // !BUILDFLAG(IS_ANDROID)
}

void BraveVpnService::OAuthRequest(
    const GURL& url,
    const std::string& method,
    const std::string& post_data,
    URLRequestCallback callback,
    const base::flat_map<std::string, std::string>& headers) {
  api_request_helper_.Request(method, url, post_data, "application/json", false,
                              std::move(callback), headers);
}

void BraveVpnService::GetAllServerRegions(ResponseCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kAllServerRegions);
  OAuthRequest(base_url, "GET", "", std::move(internal_callback));
}

void BraveVpnService::GetTimezonesForRegions(ResponseCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kTimezonesForRegions);
  OAuthRequest(base_url, "GET", "", std::move(internal_callback));
}

void BraveVpnService::GetHostnamesForRegion(ResponseCallback callback,
                                            const std::string& region) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kHostnameForRegion);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("region", region);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::GetProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& hostname) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(hostname, kProfileCredential);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("subscriber-credential", subscriber_credential);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::VerifyPurchaseToken(ResponseCallback callback,
                                          const std::string& purchase_token,
                                          const std::string& product_id,
                                          const std::string& product_type,
                                          const std::string& bundle_id) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kVerifyPurchaseToken);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("purchase-token", purchase_token);
  dict.SetStringKey("product-id", product_id);
  dict.SetStringKey("product-type", product_type);
  dict.SetStringKey("bundle-id", bundle_id);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::OnGetResponse(
    ResponseCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  std::string json_response;
  bool success = status == 200;
  if (success) {
    // Give sanitized json response on success.
    json_response = body;
    data_decoder::JsonSanitizer::Sanitize(
        json_response,
        base::BindOnce(&BraveVpnService::OnGetSanitizedJsonResponse,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  } else {
    // Give empty response on failure.
    std::move(callback).Run(json_response, success);
  }
}

void BraveVpnService::OnGetSanitizedJsonResponse(
    ResponseCallback callback,
    data_decoder::JsonSanitizer::Result sanitized_json_response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  std::string json_response;
  bool success = true;
  if (sanitized_json_response.error) {
    VLOG(1) << "Response validation error: " << *sanitized_json_response.error;
    success = false;
  }

  if (success && !sanitized_json_response.value.has_value()) {
    VLOG(1) << "Empty response";
    success = false;
  }

  if (success)
    json_response = sanitized_json_response.value.value();

  std::move(callback).Run(json_response, success);
}

void BraveVpnService::GetSubscriberCredential(
    ResponseCallback callback,
    const std::string& product_type,
    const std::string& product_id,
    const std::string& validation_method,
    const std::string& purchase_token,
    const std::string& bundle_id) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetSubscriberCredential,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(kVpnHost, kCreateSubscriberCredential);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("product-type", product_type);
  dict.SetStringKey("product-id", product_id);
  dict.SetStringKey("validation-method", validation_method);
  dict.SetStringKey("purchase-token", purchase_token);
  dict.SetStringKey("bundle-id", bundle_id);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::OnGetSubscriberCredential(
    ResponseCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  std::string subscriber_credential;
  bool success = status == 200;
  if (success) {
    subscriber_credential = GetSubscriberCredentialFromJson(body);
  } else {
    VLOG(1) << __func__ << " Response from API was not HTTP 200 (Received "
            << status << ")";
  }
  std::move(callback).Run(subscriber_credential, success);
}

void BraveVpnService::GetSubscriberCredentialV12(
    ResponseCallback callback,
    const std::string& payments_environment,
    const std::string& monthly_pass) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetSubscriberCredential,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  const GURL base_url =
      GetURLWithPath(kVpnHost, kCreateSubscriberCredentialV12);
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("validation-method", "brave-premium");
  dict.SetStringKey("brave-vpn-premium-monthly-pass", monthly_pass);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback),
               {{"Brave-Payments-Environment", payments_environment}});
}
