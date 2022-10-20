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
#include "base/time/time.h"
#include "brave/components/brave_vpn/brave_vpn_constants.h"
#include "brave/components/brave_vpn/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/pref_names.h"
#include "brave/components/brave_vpn/vpn_response_parser.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/prefs/pref_service.h"
#include "net/cookies/cookie_inclusion_status.h"
#include "net/cookies/parsed_cookie.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/url_util.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_vpn/switches.h"
#include "brave/components/version_info/version_info.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/version_info/version_info.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace brave_vpn {

using ConnectionState = mojom::ConnectionState;
using PurchasedState = mojom::PurchasedState;

BraveVpnService::BraveVpnService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs,
    PrefService* profile_prefs,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter)
    : local_prefs_(local_prefs),
      profile_prefs_(profile_prefs),
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
  auto* preference = local_prefs_->FindPreference(prefs::kBraveVPNRegionList);
  if (preference && !preference->IsDefaultValue()) {
    ReloadPurchasedState();
  }
#endif  // !BUILDFLAG(IS_ANDROID)

  InitP3A();
}

BraveVpnService::~BraveVpnService() = default;

std::string BraveVpnService::GetCurrentEnvironment() const {
  return local_prefs_->GetString(prefs::kBraveVPNEEnvironment);
}

void BraveVpnService::ReloadPurchasedState() {
  LoadPurchasedState(skus::GetDomain("vpn", GetCurrentEnvironment()));
}

void BraveVpnService::BindInterface(
    mojo::PendingReceiver<mojom::ServiceHandler> receiver) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  receivers_.Add(this, std::move(receiver));
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

void BraveVpnService::OnGetInvalidToken() {
  SetPurchasedState(GetCurrentEnvironment(), PurchasedState::EXPIRED);
}

void BraveVpnService::OnConnectionStateChanged(mojom::ConnectionState state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;

  // Ignore connection state change request for non purchased user.
  // This can be happened when user controls vpn via os settings.
  if (!is_purchased_user())
    return;

  if (state == ConnectionState::CONNECTED) {
    RecordP3A(true);
  }

  for (const auto& obs : observers_)
    obs->OnConnectionStateChanged(state);
}

mojom::ConnectionState BraveVpnService::GetConnectionState() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return GetBraveVPNConnectionAPI()->connection_state();
}

bool BraveVpnService::IsConnected() const {
  return GetConnectionState() == ConnectionState::CONNECTED;
}

void BraveVpnService::RemoveVPNConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(2) << __func__;
  GetBraveVPNConnectionAPI()->RemoveVPNConnection();
}

void BraveVpnService::Connect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  GetBraveVPNConnectionAPI()->Connect();
}

void BraveVpnService::Disconnect() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  GetBraveVPNConnectionAPI()->Disconnect();
}

void BraveVpnService::ToggleConnection() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  GetBraveVPNConnectionAPI()->ToggleConnection();
}

void BraveVpnService::GetConnectionState(GetConnectionStateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const auto state = GetBraveVPNConnectionAPI()->connection_state();
  VLOG(2) << __func__ << " : " << state;
  std::move(callback).Run(state);
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

  auto* preference = local_prefs_->FindPreference(prefs::kBraveVPNRegionList);
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
  local_prefs_->Set(prefs::kBraveVPNRegionList,
                    base::Value(std::move(regions_list)));
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
  local_prefs_->SetString(prefs::kBraveVPNDeviceRegion, name);
}

void BraveVpnService::SetSelectedRegion(const std::string& name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  local_prefs_->SetString(prefs::kBraveVPNSelectedRegion, name);
}

std::string BraveVpnService::GetDeviceRegion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return local_prefs_->GetString(prefs::kBraveVPNDeviceRegion);
}

std::string BraveVpnService::GetSelectedRegion() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return local_prefs_->GetString(prefs::kBraveVPNSelectedRegion);
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
  auto connection_state = GetConnectionState();
  if (connection_state == ConnectionState::DISCONNECTING ||
      connection_state == ConnectionState::CONNECTING) {
    VLOG(2) << __func__ << ": Current state: " << connection_state
            << " : prevent changing selected region while previous operation "
               "is in-progress";
    return;
  }

  VLOG(2) << __func__ << " : " << region_ptr->name_pretty;
  SetSelectedRegion(region_ptr->name);

  // As new selected region is used, |connection_info_| for previous selected
  // should be cleared.
  GetBraveVPNConnectionAPI()->ResetConnectionInfo();
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

  std::move(callback).Run(brave_version, os_version,
                          GetBraveVPNConnectionAPI()->GetHostname());
}

BraveVPNOSConnectionAPI* BraveVpnService::GetBraveVPNConnectionAPI() const {
  if (is_simulation_)
    return BraveVPNOSConnectionAPI::GetInstanceForTest();
  return BraveVPNOSConnectionAPI::GetInstance();
}

// NOTE(bsclifton): Desktop uses API to create a ticket.
// Android and iOS directly send an email.
void BraveVpnService::OnCreateSupportTicket(
    CreateSupportTicketCallback callback,
    APIRequestResult api_request_result) {
  bool success = api_request_result.response_code() == 200;
  VLOG(2) << "OnCreateSupportTicket success=" << success
          << "\nresponse_code=" << api_request_result.response_code();
  std::move(callback).Run(success, api_request_result.body());
}
#endif  // !BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_ANDROID)
void BraveVpnService::GetPurchaseToken(GetPurchaseTokenCallback callback) {
  std::string purchase_token_string = "";
  std::string package_string = "com.brave.browser";
  std::string product_id_string = "brave-firewall-vpn-premium";

  // Get the Android purchase token (for Google Play Store).
  // The value for this is validated on the account.brave.com side
  auto* purchase_token =
      profile_prefs_->FindPreference(prefs::kBraveVPNPurchaseTokenAndroid);
  if (purchase_token && !purchase_token->IsDefaultValue()) {
    purchase_token_string =
        profile_prefs_->GetString(prefs::kBraveVPNPurchaseTokenAndroid);
  }

  // Package name is important; for real users, it'll be the Release package.
  // For testing we do have the ability to use the Nightly package.
  auto* package =
      profile_prefs_->FindPreference(prefs::kBraveVPNPackageAndroid);
  if (package && !package->IsDefaultValue()) {
    package_string = profile_prefs_->GetString(prefs::kBraveVPNPackageAndroid);
  }

  auto* product_id =
      profile_prefs_->FindPreference(prefs::kBraveVPNProductIdAndroid);
  if (product_id && !product_id->IsDefaultValue()) {
    product_id_string =
        profile_prefs_->GetString(prefs::kBraveVPNProductIdAndroid);
  }

  base::Value::Dict response;
  response.Set("type", "android");
  response.Set("raw_receipt", purchase_token_string);
  response.Set("package", package_string);
  response.Set("subscription_id", product_id_string);

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
    GetBraveVPNConnectionAPI()->SetConnectionState(
        ConnectionState::CONNECT_FAILED);
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

    GetBraveVPNConnectionAPI()->CheckConnection();
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

  absl::optional<base::Value> records_v = base::JSONReader::Read(
      summary_string, base::JSONParserOptions::JSON_PARSE_RFC);

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
      VLOG(1) << __func__ << " : Credential is not active.";
      SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
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
  GetBraveVPNConnectionAPI()->SetSkusCredential(skus_credential_);

  LoadCachedRegionData();

  // Only fetch when we don't have cache.
  if (!regions_.empty()) {
    SetPurchasedState(env, PurchasedState::PURCHASED);
  } else {
    FetchRegionData(false);
  }

  ScheduleBackgroundRegionDataFetch();
  GetBraveVPNConnectionAPI()->CheckConnection();
#endif
}

// TODO(simonhong): Should move p3a to BraveVPNOSConnectionAPI?
void BraveVpnService::InitP3A() {
  p3a_timer_.Start(FROM_HERE, base::Hours(kP3AIntervalHours), this,
                   &BraveVpnService::OnP3AInterval);
  RecordP3A(false);
}

void BraveVpnService::RecordP3A(bool new_usage) {
  if (new_usage) {
    p3a_utils::RecordFeatureUsage(local_prefs_, prefs::kBraveVPNFirstUseTime,
                                  prefs::kBraveVPNLastUseTime);
  }
  p3a_utils::RecordFeatureNewUserReturning(
      local_prefs_, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNUsedSecondDay, kNewUserReturningHistogramName);
  p3a_utils::RecordFeatureDaysInMonthUsed(
      local_prefs_, new_usage, prefs::kBraveVPNLastUseTime,
      prefs::kBraveVPNDaysInMonthUsed, kDaysInMonthUsedHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_prefs_, prefs::kBraveVPNLastUseTime, kLastUsageTimeHistogramName);
}

#if BUILDFLAG(IS_ANDROID)
void BraveVpnService::RecordAndroidBackgroundP3A(int64_t session_start_time_ms,
                                                 int64_t session_end_time_ms) {
  if (session_start_time_ms < 0 || session_end_time_ms < 0) {
    RecordP3A(false);
    return;
  }
  base::Time session_start_time =
      base::Time::FromJsTime(static_cast<double>(session_start_time_ms))
          .LocalMidnight();
  base::Time session_end_time =
      base::Time::FromJsTime(static_cast<double>(session_end_time_ms))
          .LocalMidnight();
  for (base::Time day = session_start_time; day <= session_end_time;
       day += base::Days(1)) {
    bool is_last_day = day == session_end_time;
    // Call functions for each day in the last session to ensure
    // p3a_util functions produce the correct result
    p3a_utils::RecordFeatureUsage(local_prefs_, prefs::kBraveVPNFirstUseTime,
                                  prefs::kBraveVPNLastUseTime, day);
    p3a_utils::RecordFeatureNewUserReturning(
        local_prefs_, prefs::kBraveVPNFirstUseTime, prefs::kBraveVPNLastUseTime,
        prefs::kBraveVPNUsedSecondDay, kNewUserReturningHistogramName,
        is_last_day);
    p3a_utils::RecordFeatureDaysInMonthUsed(
        local_prefs_, day, prefs::kBraveVPNLastUseTime,
        prefs::kBraveVPNDaysInMonthUsed, kDaysInMonthUsedHistogramName,
        is_last_day);
  }
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_prefs_, prefs::kBraveVPNLastUseTime, kLastUsageTimeHistogramName);
}
#endif

void BraveVpnService::OnP3AInterval() {
  RecordP3A(false);
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
  local_prefs_->SetString(prefs::kBraveVPNEEnvironment, env);
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
    api_request_helper::APIRequestResult result) {
  // NOTE: |api_request_helper_| uses JsonSanitizer to sanitize input made with
  // requests. |body| will be empty when the response from service is invalid
  // json.
  const bool success = result.response_code() == 200;
  std::move(callback).Run(result.body(), success);
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
    APIRequestResult api_request_result) {
  bool success = api_request_result.response_code() == 200;
  std::string error;
  std::string subscriber_credential =
      ParseSubscriberCredentialFromJson(api_request_result.body(), &error);
  if (!success) {
    subscriber_credential = error;
    VLOG(1) << __func__ << " Response from API was not HTTP 200 (Received "
            << api_request_result.response_code() << ")";
  }
  std::move(callback).Run(subscriber_credential, success);
}

void BraveVpnService::GetSubscriberCredentialV12(ResponseCallback callback) {
  auto internal_callback =
      base::BindOnce(&BraveVpnService::OnGetSubscriberCredential,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  const GURL base_url =
      GetURLWithPath(kVpnHost, kCreateSubscriberCredentialV12);
  base::Value::Dict dict;
  dict.Set("validation-method", "brave-premium");
  dict.Set("brave-vpn-premium-monthly-pass", skus_credential_);
  std::string request_body = CreateJSONRequestBody(dict);
  OAuthRequest(base_url, "POST", request_body, std::move(internal_callback),
               {{"Brave-Payments-Environment",
                 GetBraveVPNPaymentsEnv(GetCurrentEnvironment())}});
}

}  // namespace brave_vpn
