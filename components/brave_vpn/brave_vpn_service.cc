/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "net/base/network_change_notifier.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/parsed_cookie.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/url_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/power_monitor/power_monitor.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/switches.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace {

constexpr char kVpnHost[] = "connect-api.guardianapp.com";

constexpr char kAllServerRegions[] = "api/v1/servers/all-server-regions";
constexpr char kTimezonesForRegions[] =
    "api/v1.1/servers/timezones-for-regions";
constexpr char kHostnameForRegion[] = "api/v1.2/servers/hostnames-for-region";
constexpr char kProfileCredential[] = "api/v1.1/register-and-create";
constexpr char kCredential[] = "api/v1.3/device/";
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

std::string CreateJSONRequestBody(base::ValueView node) {
  std::string json;
  base::JSONWriter::Write(node, &json);
  return json;
}

std::string GetSubscriberCredentialFromJson(const std::string& json) {
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                    base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v || !records_v->is_dict()) {
    VLOG(1) << __func__ << "Invalid response, could not parse JSON.";
    return "";
  }

  const auto& dict = records_v->GetDict();
  const auto* subscriber_credential = dict.FindString("subscriber-credential");
  return subscriber_credential == nullptr ? "" : *subscriber_credential;
}
#if !BUILDFLAG(IS_ANDROID)
bool IsNetworkAvailable() {
  return net::NetworkChangeNotifier::GetConnectionType() !=
         net::NetworkChangeNotifier::CONNECTION_NONE;
}
#endif  // !BUILDFLAG(IS_ANDROID)
}  // namespace

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;
using PurchasedState = mojom::PurchasedState;

BraveVpnService::BraveVpnService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter)
    : prefs_(prefs),
      skus_service_getter_(skus_service_getter),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {
  DCHECK(IsBraveVPNEnabled());
#if !BUILDFLAG(IS_ANDROID)
  auto* cmd = base::CommandLine::ForCurrentProcess();
  is_simulation_ = cmd->HasSwitch(switches::kBraveVPNSimulation);
  observed_.Observe(GetBraveVPNConnectionAPI());

  GetBraveVPNConnectionAPI()->set_target_vpn_entry_name(kBraveVPNEntryName);

  // To get proper connection state, we need to load purchased state.
  // Connection state will be checked after we confirm that this profile
  // is purchased user.
  // However, purchased state loading makes additional network request.
  // We should not make this network request for fresh user.
  // To prevent this, we load purchased state at startup only
  // when profile has cached region list because region list is fetched
  // and cached only when user purchased at least once.
  auto* preference = prefs_->FindPreference(prefs::kBraveVPNRegionList);
  if (preference && !preference->IsDefaultValue()) {
    ReloadPurchasedState();
  }
  base::PowerMonitor::AddPowerSuspendObserver(this);
#endif  // !BUILDFLAG(IS_ANDROID)
}

BraveVpnService::~BraveVpnService() {
#if !BUILDFLAG(IS_ANDROID)
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::PowerMonitor::RemovePowerSuspendObserver(this);
#endif  // !BUILDFLAG(IS_ANDROID)
}
std::string BraveVpnService::GetCurrentEnvironment() const {
  return prefs_->GetString(prefs::kBraveVPNEEnvironment);
}

void BraveVpnService::ReloadPurchasedState() {
  LoadPurchasedState(skus::GetDomain("vpn", GetCurrentEnvironment()));
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
  // Ignore connection state change request for non purchased user.
  // This can be happened when user controls vpn via os settings.
  if (!is_purchased_user())
    return;

  if (connection_state_ == state)
    return;
#if BUILDFLAG(IS_WIN)
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
#endif  // BUILDFLAG(IS_WIN)
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
  UpdateAndNotifyConnectionStateChange(!IsNetworkAvailable()
                                           ? ConnectionState::CONNECT_FAILED
                                           : ConnectionState::DISCONNECTED);

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

  if (!IsNetworkAvailable()) {
    VLOG(2) << __func__ << ": Network is not available, failed to connect";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

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

const BraveVPNConnectionInfo& BraveVpnService::GetConnectionInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return connection_info_;
}

void BraveVpnService::BindInterface(
    mojo::PendingReceiver<mojom::ServiceHandler> receiver) {
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

  // Already loaded from cache.
  if (!regions_.empty())
    return;

  // Empty device region means it's initial state.
  if (GetDeviceRegion().empty())
    return;

  auto* preference = prefs_->FindPreference(prefs::kBraveVPNRegionList);
  DCHECK(preference);
  // Early return when we don't have any cached region data.
  if (preference->IsDefaultValue())
    return;

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

void BraveVpnService::SetRegionListToPrefs() {
  DCHECK(!regions_.empty());

  base::Value::List regions_list;
  for (const auto& region : regions_) {
    regions_list.Append(GetValueFromRegion(region));
  }
  prefs_->Set(prefs::kBraveVPNRegionList, base::Value(std::move(regions_list)));
}

void BraveVpnService::OnFetchRegionList(bool background_fetch,
                                        const std::string& region_list,
                                        bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Don't update purchased state during the background fetching.

  if (!background_fetch && !success) {
    VLOG(2) << "Failed to get region list";
    SetPurchasedState(GetCurrentEnvironment(), PurchasedState::FAILED);
    return;
  }

  absl::optional<base::Value> value = base::JSONReader::Read(region_list);
  if (value && value->is_list()) {
    if (background_fetch) {
      ParseAndCacheRegionList(value->GetList(), true);
      return;
    }

    if (ParseAndCacheRegionList(value->GetList(), true)) {
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
    SetPurchasedState(GetCurrentEnvironment(), PurchasedState::FAILED);
  }
}

bool BraveVpnService::ParseAndCacheRegionList(
    const base::Value::List& region_value,
    bool save_to_prefs) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  regions_ = ParseRegionList(region_value);
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
    SetDeviceRegionWithTimezone(value->GetList());
  } else {
    VLOG(2) << "Failed to get invalid timezones list";
  }

  // Can set as purchased state now regardless of timezone fetching result.
  // We use default one picked from region list as a device region on failure.
  SetPurchasedState(GetCurrentEnvironment(), PurchasedState::PURCHASED);
}

void BraveVpnService::SetDeviceRegionWithTimezone(
    const base::Value::List& timezones_value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const std::string current_time_zone = GetCurrentTimeZone();
  if (current_time_zone.empty())
    return;

  for (const auto& timezones : timezones_value) {
    DCHECK(timezones.is_dict());
    if (!timezones.is_dict())
      continue;

    const std::string* region_name = timezones.GetDict().FindString("name");
    if (!region_name)
      continue;
    const auto* timezone_list_value = timezones.GetDict().FindList("timezones");
    if (!timezone_list_value)
      continue;

    for (const auto& timezone : *timezone_list_value) {
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
  prefs_->SetString(prefs::kBraveVPNDeviceRegion, name);
}

void BraveVpnService::SetSelectedRegion(const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  prefs_->SetString(prefs::kBraveVPNSelectedRegion, name);
}

std::string BraveVpnService::GetDeviceRegion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return prefs_->GetString(prefs::kBraveVPNDeviceRegion);
}

std::string BraveVpnService::GetSelectedRegion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return prefs_->GetString(prefs::kBraveVPNSelectedRegion);
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
  std::vector<mojom::RegionPtr> regions;
  for (const auto& region : regions_) {
    regions.push_back(region.Clone());
  }
  std::move(callback).Run(std::move(regions));
}

void BraveVpnService::GetDeviceRegion(GetDeviceRegionCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  auto region_name = GetDeviceRegion();
  DCHECK(!region_name.empty());
  std::move(callback).Run(
      GetRegionPtrWithNameFromRegionList(region_name, regions_));
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
  std::move(callback).Run(
      GetRegionPtrWithNameFromRegionList(region_name, regions_));
}

void BraveVpnService::SetSelectedRegion(mojom::RegionPtr region_ptr) {
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
  std::move(callback).Run(mojom::ProductUrls::New(
      kFeedbackUrl, kAboutUrl, GetManageUrl(GetCurrentEnvironment())));
}

void BraveVpnService::CreateSupportTicket(
    const std::string& email,
    const std::string& subject,
    const std::string& body,
    CreateSupportTicketCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnCreateSupportTicket,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  OAuthRequest(
      GetURLWithPath(kVpnHost, kCreateSupportTicket), "POST",
      CreateJSONRequestBody(GetValueWithTicketInfos(email, subject, body)),
      std::move(internal_callback));
}

void BraveVpnService::GetSupportData(GetSupportDataCallback callback) {
  std::string brave_version =
      version_info::GetBraveVersionWithoutChromiumMajorVersion();
  std::string os_version = version_info::GetOSType();
  std::string vpn_hostname = hostname_ ? hostname_->hostname : "";
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
    ParseAndCacheHostnames(region, value->GetList());
    return;
  }

  VLOG(2) << __func__ << " : failed to fetch hostnames for " << region;
  UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
}

void BraveVpnService::ParseAndCacheHostnames(
    const std::string& region,
    const base::Value::List& hostnames_value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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

  if (skus_credential_.empty()) {
    VLOG(2) << __func__ << " : skus_credential is empty";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }

  // Get subscriber credentials and then get EAP credentials with it to create
  // OS VPN entry.
  VLOG(2) << __func__ << " : request subscriber credential:"
          << GetBraveVPNPaymentsEnv(GetCurrentEnvironment());
  GetSubscriberCredentialV12(
      base::BindOnce(&BraveVpnService::OnGetSubscriberCredentialV12,
                     base::Unretained(this)),
      GetBraveVPNPaymentsEnv(GetCurrentEnvironment()), skus_credential_);
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
    const auto& dict = value->GetDict();
    const std::string* username = dict.FindString(kUsernameKey);
    const std::string* password = dict.FindString(kPasswordKey);
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

BraveVPNOSConnectionAPI* BraveVpnService::GetBraveVPNConnectionAPI() {
  if (is_simulation_)
    return BraveVPNOSConnectionAPI::GetInstanceForTest();
  return BraveVPNOSConnectionAPI::GetInstance();
}

// NOTE(bsclifton): Desktop uses API to create a ticket.
// Android and iOS directly send an email.
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
void BraveVpnService::OnSuspend() {
  // Set reconnection state in case if computer/laptop is going to sleep.
  // The disconnection event will be fired after waking up and we want to
  // restore the connection.
  needs_connect_ = is_connected();
}

void BraveVpnService::OnResume() {}
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
void BraveVpnService::GetPurchaseToken(GetPurchaseTokenCallback callback) {
  std::string purchase_token_string;
  // Get the Android purchase token (for Google Play Store).
  // The value for this is validated on the account.brave.com side
  auto* purchase_token =
      prefs_->FindPreference(prefs::kBraveVPNPurchaseTokenAndroid);
  if (purchase_token && !purchase_token->IsDefaultValue()) {
    purchase_token_string =
        prefs_->GetString(prefs::kBraveVPNPurchaseTokenAndroid);
  }

  // TODO(bsclifton): remove me. This is for testing only
  if (purchase_token_string.length() == 0) {
    purchase_token_string =
        "oohdhbmbebmciddpbcicgnko.AO-J1OxJGS6-"
        "tNYvzofx7RO2hJSEgQmi6tOrLHEB4zJ2OhsyhX3mhEe4QKS0MVxtJCBNIAlBP5jAgDPqdX"
        "DNz15JhIXt5QYcIExIxe5H5ifbhAsHILlUXlE";
  }

  base::Value response(base::Value::Type::DICTIONARY);
  response.SetStringKey("type", "android");
  response.SetStringKey("raw_receipt", purchase_token_string.c_str());
  response.SetStringKey("package", "com.brave.browser");
  response.SetStringKey("subscription_id", "brave-firewall-vpn-premium");

  std::string response_json;
  base::JSONWriter::Write(response, &response_json);

  std::string encoded_response_json;
  base::Base64Encode(response_json, &encoded_response_json);

  std::move(callback).Run(encoded_response_json);
}
#endif  // BUILDFLAG(IS_ANDROID)

void BraveVpnService::AddObserver(
    mojo::PendingRemote<mojom::ServiceObserver> observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  observers_.Add(std::move(observer));
}

mojom::PurchasedState BraveVpnService::GetPurchasedStateSync() const {
  return purchased_state_.value_or(mojom::PurchasedState::NOT_PURCHASED);
}

void BraveVpnService::GetPurchasedState(GetPurchasedStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto value = GetPurchasedStateSync();
  VLOG(2) << __func__ << " : " << value;
  std::move(callback).Run(value);
}

void BraveVpnService::LoadPurchasedState(const std::string& domain) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto requested_env = skus::GetEnvironmentForDomain(domain);
  if (GetCurrentEnvironment() == requested_env &&
      purchased_state_ == PurchasedState::LOADING)
    return;
#if !BUILDFLAG(IS_ANDROID)
  if (!IsNetworkAvailable()) {
    VLOG(2) << __func__ << ": Network is not available, failed to connect";
    UpdateAndNotifyConnectionStateChange(ConnectionState::CONNECT_FAILED);
    return;
  }
#endif
  if (!purchased_state_.has_value())
    SetPurchasedState(requested_env, PurchasedState::LOADING);

#if !BUILDFLAG(IS_ANDROID) && !defined(OFFICIAL_BUILD)
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(switches::kBraveVPNTestMonthlyPass)) {
    skus_credential_ =
        cmd->GetSwitchValueASCII(switches::kBraveVPNTestMonthlyPass);
    LoadCachedRegionData();
    SetCurrentEnvironment(requested_env);
    if (!regions_.empty()) {
      SetPurchasedState(GetCurrentEnvironment(), PurchasedState::PURCHASED);
    } else {
      FetchRegionData(false);
    }

    GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);
    return;
  }
#endif  // !BUILDFLAG(IS_ANDROID) && !defined(OFFICIAL_BUILD)

  EnsureMojoConnected();
  skus_service_->CredentialSummary(
      domain, base::BindOnce(&BraveVpnService::OnCredentialSummary,
                             base::Unretained(this), domain));
}

void BraveVpnService::OnCredentialSummary(const std::string& domain,
                                          const std::string& summary_string) {
  auto env = skus::GetEnvironmentForDomain(domain);
  std::string summary_string_trimmed;
  base::TrimWhitespaceASCII(summary_string, base::TrimPositions::TRIM_ALL,
                            &summary_string_trimmed);
  if (summary_string_trimmed.length() == 0) {
    // no credential found; person needs to login
    VLOG(1) << __func__ << " : No credential found; user needs to login!";
    SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
    return;
  }

  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          summary_string, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;

  if (records_v && records_v->is_dict()) {
    auto active = records_v->GetDict().FindBool("active").value_or(false);
    if (active) {
      VLOG(1) << __func__ << " : Active credential found!";
      // if a credential is ready, we can present it
      EnsureMojoConnected();
      skus_service_->PrepareCredentialsPresentation(
          domain, "*",
          base::BindOnce(&BraveVpnService::OnPrepareCredentialsPresentation,
                         base::Unretained(this), domain));
    } else {
      VLOG(1) << __func__ << " : Credential appears to be expired.";
      SetPurchasedState(env, PurchasedState::EXPIRED);
    }
  } else {
    VLOG(1) << __func__ << " : Got invalid credential summary!";
    SetPurchasedState(env, PurchasedState::FAILED);
  }
}

void BraveVpnService::OnPrepareCredentialsPresentation(
    const std::string& domain,
    const std::string& credential_as_cookie) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto env = skus::GetEnvironmentForDomain(domain);
  // Credential is returned in cookie format.
  net::CookieInclusionStatus status;
  net::ParsedCookie credential_cookie(credential_as_cookie, &status);
  // TODO(bsclifton): have a better check / logging.
  // should these failed states be considered NOT_PURCHASED?
  // or maybe it can be considered FAILED status?
  if (!credential_cookie.IsValid()) {
    VLOG(1) << __func__ << " : FAILED credential_cookie.IsValid";
    SetPurchasedState(env, PurchasedState::FAILED);
    return;
  }
  if (!status.IsInclude()) {
    VLOG(1) << __func__ << " : FAILED status.IsInclude";
    SetPurchasedState(env, PurchasedState::FAILED);
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
  if (credential.empty()) {
    SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
    return;
  }
  if (GetCurrentEnvironment() != env) {
    // Change environment because we have successfully authorized with new one.
    SetCurrentEnvironment(env);
  }

  skus_credential_ = credential;

#if BUILDFLAG(IS_ANDROID)
  SetPurchasedState(env, PurchasedState::PURCHASED);
#else
  LoadCachedRegionData();

  // Only fetch when we don't have cache.
  if (!regions_.empty()) {
    SetPurchasedState(env, PurchasedState::PURCHASED);
  } else {
    FetchRegionData(false);
  }

  ScheduleBackgroundRegionDataFetch();
  GetBraveVPNConnectionAPI()->CheckConnection(kBraveVPNEntryName);
#endif
}

void BraveVpnService::SetPurchasedState(const std::string& env,
                                        PurchasedState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (GetPurchasedStateSync() == state || env != GetCurrentEnvironment()) {
    return;
  }

  purchased_state_ = state;

  for (const auto& obs : observers_)
    obs->OnPurchasedStateChanged(purchased_state_.value());
}

void BraveVpnService::SetCurrentEnvironment(const std::string& env) {
  prefs_->SetString(prefs::kBraveVPNEEnvironment, env);
  purchased_state_.reset();
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
  api_request_helper_.Request(method, url, post_data, "application/json", true,
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
  base::Value::Dict dict;
  dict.Set("region", region);
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
  base::Value::Dict dict;
  dict.Set("subscriber-credential", subscriber_credential);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::GetWireguardProfileCredentials(
    ResponseCallback callback,
    const std::string& subscriber_credential,
    const std::string& public_key,
    const std::string& hostname) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(hostname, kCredential);
  base::Value::Dict dict;
  dict.Set("subscriber-credential", subscriber_credential);
  dict.Set("public-key", public_key);
  dict.Set("transport-protocol", "wireguard");
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::VerifyCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url =
      GetURLWithPath(hostname, kCredential + client_id + "/verify-credentials");
  base::Value::Dict dict;
  dict.Set("subscriber-credential", subscriber_credential);
  dict.Set("api-auth-token", api_auth_token);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback));
}

void BraveVpnService::InvalidateCredentials(
    ResponseCallback callback,
    const std::string& hostname,
    const std::string& client_id,
    const std::string& subscriber_credential,
    const std::string& api_auth_token) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetResponse,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  GURL base_url = GetURLWithPath(
      hostname, kCredential + client_id + "/invalidate-credentials");
  base::Value::Dict dict;
  dict.Set("subscriber-credential", subscriber_credential);
  dict.Set("api-auth-token", api_auth_token);
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
  base::Value::Dict dict;
  dict.Set("purchase-token", purchase_token);
  dict.Set("product-id", product_id);
  dict.Set("product-type", product_type);
  dict.Set("bundle-id", bundle_id);
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
  GURL base_url = GetURLWithPath(kVpnHost, kCreateSubscriberCredentialV12);
  base::Value::Dict dict;
  dict.Set("product-type", product_type);
  dict.Set("product-id", product_id);
  dict.Set("validation-method", validation_method);
  dict.Set("purchase-token", purchase_token);
  dict.Set("bundle-id", bundle_id);
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
  base::Value::Dict dict;
  dict.Set("validation-method", "brave-premium");
  dict.Set("brave-vpn-premium-monthly-pass", monthly_pass);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback),
               {{"Brave-Payments-Environment", payments_environment}});
}

}  // namespace brave_vpn
