/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/star_randomness_meta.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/network_annotations.h"
#include "brave/components/p3a/nitro_utils/attestation.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace p3a {

namespace {

constexpr char kCurrentPKPrefName[] = "brave.p3a.current_pk";  // DEPRECATED
constexpr char kCurrentEpochPrefName[] =
    "brave.p3a.current_epoch";  // DEPRECATED
constexpr char kNextEpochTimePrefName[] =
    "brave.p3a.next_epoch_time";  // DEPRECATED
constexpr char kApprovedCertFPPrefName[] =
    "brave.p3a.approved_cert_fp";  // DEPRECATED

constexpr char kRandomnessMetaDictPrefName[] =
    "brave.p3a.randomness_meta";  // DEPRECATED
constexpr char kCurrentPKPrefKey[] = "current_pk";
constexpr char kCurrentEpochPrefKey[] = "current_epoch";
constexpr char kNextEpochTimePrefKey[] = "next_epoch_time";

// A generous arbitrary limit, 128KB
constexpr std::size_t kMaxInfoResponseSize = 128 * 1024;
constexpr int kRndInfoRetryInitialBackoffSeconds = 5;
constexpr int kRndInfoRetryMaxBackoffMinutes = 60;

::rust::Box<constellation::PPOPRFPublicKeyWrapper> DecodeServerPublicKey(
    const std::string* pk_base64) {
  if (pk_base64 == nullptr || pk_base64->empty()) {
    VLOG(2) << "StarRandomnessMeta: no pk available, will not validate "
               "randomness";
    return constellation::get_ppoprf_null_public_key();
  }
  std::optional<std::vector<uint8_t>> dec_pk = base::Base64Decode(*pk_base64);
  if (!dec_pk.has_value()) {
    LOG(ERROR) << "StarRandomnessMeta: bad pk base64, will not validate "
                  "randomness";
    return constellation::get_ppoprf_null_public_key();
  }
  ::rust::Slice<const uint8_t> dec_pk_slice{dec_pk.value().data(),
                                            dec_pk.value().size()};
  constellation::PPOPRFPublicKeyResult pk_res =
      constellation::load_ppoprf_public_key(dec_pk_slice);
  if (!pk_res.error.empty()) {
    LOG(ERROR) << "StarRandomnessMeta: failed to load pk: "
               << std::string(pk_res.error);
    return constellation::get_ppoprf_null_public_key();
  }
  return std::move(pk_res.key);
}

}  // namespace

RandomnessServerInfo::RandomnessServerInfo(
    uint8_t current_epoch,
    base::Time next_epoch_time,
    bool epoch_change_detected,
    ::rust::Box<constellation::PPOPRFPublicKeyWrapper> public_key)
    : current_epoch(current_epoch),
      next_epoch_time(next_epoch_time),
      epoch_change_detected(epoch_change_detected),
      public_key(std::move(public_key)) {}
RandomnessServerInfo::~RandomnessServerInfo() = default;

RandomnessServerUpdateState::RandomnessServerUpdateState() = default;
RandomnessServerUpdateState::~RandomnessServerUpdateState() = default;

StarRandomnessMeta::StarRandomnessMeta(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    RandomnessServerInfoCallback info_callback,
    const P3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      local_state_(local_state),
      info_callback_(info_callback),
      config_(config) {
  for (MetricLogType log_type : kAllMetricLogTypes) {
    update_states_[log_type] = std::make_unique<RandomnessServerUpdateState>();
  }
  std::string approved_cert_fp_str =
      local_state->GetString(kApprovedCertFPPrefName);
  net::HashValue approved_cert_fp;
  if (!approved_cert_fp_str.empty() &&
      approved_cert_fp.FromString(approved_cert_fp_str)) {
    VLOG(2) << "StarRandomnessMeta: loaded cached approved cert";
    approved_cert_fp_ = std::make_optional(approved_cert_fp);
  }
}

StarRandomnessMeta::~StarRandomnessMeta() = default;

void StarRandomnessMeta::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kRandomnessMetaDictPrefName);
  registry->RegisterStringPref(kApprovedCertFPPrefName, "");
}

void StarRandomnessMeta::RegisterPrefsForMigration(
    PrefRegistrySimple* registry) {
  // Added 09/2023
  registry->RegisterStringPref(kCurrentPKPrefName, std::string());
  registry->RegisterIntegerPref(kCurrentEpochPrefName, -1);
  registry->RegisterTimePref(kNextEpochTimePrefName, base::Time());
}

void StarRandomnessMeta::MigrateObsoleteLocalStatePrefs(
    PrefService* local_state) {
  // Added 09/2023
  ScopedDictPrefUpdate update(local_state, kRandomnessMetaDictPrefName);
  base::Value::Dict* typical_dict =
      update->EnsureDict(MetricLogTypeToString(MetricLogType::kTypical));

  std::string current_pk = local_state->GetString(kCurrentPKPrefName);
  int current_epoch = local_state->GetInteger(kCurrentEpochPrefName);
  base::Time next_epoch_time = local_state->GetTime(kNextEpochTimePrefName);
  if (!current_pk.empty() && current_epoch != -1 &&
      !next_epoch_time.is_null()) {
    typical_dict->Set(kCurrentPKPrefKey, current_pk);
    typical_dict->Set(kCurrentEpochPrefKey, current_epoch);
    typical_dict->Set(kNextEpochTimePrefKey,
                      base::TimeToValue(next_epoch_time));
    local_state->ClearPref(kCurrentPKPrefName);
    local_state->ClearPref(kCurrentEpochPrefName);
    local_state->ClearPref(kNextEpochTimePrefName);
  }
}

bool StarRandomnessMeta::ShouldAttestEnclave() {
  return !config_->disable_star_attestation &&
         features::IsConstellationEnclaveAttestationEnabled();
}

bool StarRandomnessMeta::VerifyRandomnessCert(
    network::SimpleURLLoader* url_loader) {
  if (!ShouldAttestEnclave()) {
    VLOG(2) << "StarRandomnessMeta: skipping approved cert check";
    return true;
  }
  const network::mojom::URLResponseHead* response_info =
      url_loader->ResponseInfo();
  if (!approved_cert_fp_.has_value()) {
    LOG(ERROR) << "StarRandomnessMeta: approved cert is missing";
    AttestServer(false);
    return false;
  }
  if (!response_info->ssl_info.has_value() ||
      response_info->ssl_info->cert == nullptr) {
    LOG(ERROR) << "StarRandomnessMeta: ssl info is missing from response info";
    return false;
  }
  net::HashValue cert_fp_hash = net::HashValue(
      response_info->ssl_info->cert->CalculateChainFingerprint256());
  if (cert_fp_hash != *approved_cert_fp_) {
    LOG(ERROR) << "StarRandomnessMeta: approved cert mismatch, will retry "
                  "attestation; fp = "
               << cert_fp_hash.ToString();
    AttestServer(false);
    return false;
  }
  return true;
}

void StarRandomnessMeta::RequestServerInfo(MetricLogType log_type) {
  RandomnessServerUpdateState* update_state = update_states_[log_type].get();
  update_state->rnd_server_info = nullptr;

  if (ShouldAttestEnclave() && !approved_cert_fp_.has_value()) {
    AttestServer(true);
    return;
  }

  if (!update_state->has_used_cached_info) {
    // Using cached server info, if available.
    const base::Value::Dict& meta_dict =
        local_state_->GetDict(kRandomnessMetaDictPrefName);
    const base::Value::Dict* meta_type_dict =
        meta_dict.FindDict(MetricLogTypeToString(log_type));
    if (meta_type_dict != nullptr) {
      update_state->last_cached_epoch =
          meta_type_dict->FindInt(kCurrentEpochPrefKey);
      base::Time saved_next_epoch_time =
          base::ValueToTime(meta_type_dict->Find(kNextEpochTimePrefKey))
              .value_or(base::Time());
      // only return cached info if the epoch is not expired,
      // and if the "fake" star epoch matches the last saved epoch (if
      // specified). if "fake" star epoch does not match the saved epoch, then
      // fresh server info should be requested to update the local state with
      // the new epoch info
      if (saved_next_epoch_time > base::Time::Now() &&
          (!config_->fake_star_epochs.at(log_type).has_value() ||
           config_->fake_star_epochs.at(log_type) ==
               update_state->last_cached_epoch)) {
        const std::string* saved_pk =
            meta_type_dict->FindString(kCurrentPKPrefKey);

        update_state->rnd_server_info = std::make_unique<RandomnessServerInfo>(
            *update_state->last_cached_epoch, saved_next_epoch_time, false,
            DecodeServerPublicKey(saved_pk));
        VLOG(2) << "StarRandomnessMeta: using cached server info";
        info_callback_.Run(log_type, update_state->rnd_server_info.get());
        update_state->has_used_cached_info = true;
        return;
      }
    }
  }

  update_state->rnd_server_info = nullptr;
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url =
      GURL(base::StrCat({config_->star_randomness_host, "/instances/",
                         MetricLogTypeToString(log_type), "/info"}));
  if (!resource_request->url.is_valid() ||
      !resource_request->url.SchemeIsHTTPOrHTTPS()) {
    VLOG(2) << "StarRandomnessMeta: star randomness host invalid, skipping "
               "server info request";
    return;
  }

  update_state->url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessRequestAnnotation());
  update_state->url_loader->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse);
  update_state->url_loader->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&StarRandomnessMeta::HandleServerInfoResponse,
                     base::Unretained(this), log_type),
      kMaxInfoResponseSize);
}

RandomnessServerInfo* StarRandomnessMeta::GetCachedRandomnessServerInfo(
    MetricLogType log_type) {
  return update_states_[log_type]->rnd_server_info.get();
}

void StarRandomnessMeta::AttestServer(bool make_info_request_after) {
  if (attestation_pending_) {
    return;
  }
  attestation_pending_ = true;
  approved_cert_fp_ = std::nullopt;

  VLOG(2) << "StarRandomnessMeta: starting attestation";
  GURL attestation_url = GURL(
      base::StrCat({config_->star_randomness_host, "/enclave/attestation"}));
  if (!attestation_url.is_valid() || !attestation_url.SchemeIsHTTPOrHTTPS()) {
    VLOG(2) << "StarRandomnessMeta: star randomness host invalid, skipping "
               "server attestation";
    return;
  }
  nitro_utils::RequestAndVerifyAttestationDocument(
      attestation_url, url_loader_factory_.get(),
      base::BindOnce(&StarRandomnessMeta::HandleAttestationResult,
                     weak_ptr_factory_.GetWeakPtr(), make_info_request_after));
}

void StarRandomnessMeta::HandleAttestationResult(
    bool make_info_request_after,
    scoped_refptr<net::X509Certificate> approved_cert) {
  if (approved_cert == nullptr) {
    LOG(ERROR) << "StarRandomnessMeta: attestation failed";
    attestation_pending_ = false;
    if (make_info_request_after) {
      for (MetricLogType log_type : kAllMetricLogTypes) {
        ScheduleServerInfoRetry(log_type);
      }
    }
    return;
  }
  approved_cert_fp_ = std::make_optional(
      net::HashValue(approved_cert->CalculateChainFingerprint256()));
  std::string approved_cert_fp_str = approved_cert_fp_->ToString();
  local_state_->SetString(kApprovedCertFPPrefName, approved_cert_fp_str);
  attestation_pending_ = false;
  VLOG(2) << "StarRandomnessMeta: attestation succeeded; fp = "
          << approved_cert_fp_str;
  if (make_info_request_after) {
    for (MetricLogType log_type : kAllMetricLogTypes) {
      RequestServerInfo(log_type);
    }
  }
}

void StarRandomnessMeta::ScheduleServerInfoRetry(MetricLogType log_type) {
  RandomnessServerUpdateState* update_state = update_states_[log_type].get();
  update_state->url_loader = nullptr;
  if (update_state->current_backoff_time.is_zero()) {
    update_state->current_backoff_time =
        base::Seconds(kRndInfoRetryInitialBackoffSeconds);
  } else {
    update_state->current_backoff_time *= 2;
    base::TimeDelta max_backoff_time =
        base::Minutes(kRndInfoRetryMaxBackoffMinutes);
    if (update_state->current_backoff_time > max_backoff_time) {
      update_state->current_backoff_time = max_backoff_time;
    }
  }
  VLOG(2) << "StarRandomnessMeta: scheduling server info req retry in "
          << update_state->current_backoff_time;
  update_state->rnd_info_retry_timer.Start(
      FROM_HERE, update_state->current_backoff_time,
      base::BindOnce(&StarRandomnessMeta::RequestServerInfo,
                     base::Unretained(this), log_type));
}

void StarRandomnessMeta::HandleServerInfoResponse(
    MetricLogType log_type,
    std::unique_ptr<std::string> response_body) {
  RandomnessServerUpdateState* update_state = update_states_[log_type].get();
  if (!response_body || response_body->empty()) {
    std::string error_str =
        net::ErrorToShortString(update_state->url_loader->NetError());
    VLOG(2) << "StarRandomnessMeta: no response body for randomness server "
            << "info request, net error: " << error_str;
    ScheduleServerInfoRetry(log_type);
    return;
  }
  if (!VerifyRandomnessCert(update_state->url_loader.get())) {
    ScheduleServerInfoRetry(log_type);
    return;
  }
  update_state->url_loader = nullptr;
  base::JSONReader::Result parsed_value =
      base::JSONReader::ReadAndReturnValueWithError(
          *response_body, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed_value.has_value() || !parsed_value.value().is_dict()) {
    LOG(ERROR) << "StarRandomnessMeta: failed to parse server info json: "
               << parsed_value.error().message;
    ScheduleServerInfoRetry(log_type);
    return;
  }
  base::Value::Dict& root = parsed_value->GetDict();
  std::optional<int> epoch = root.FindInt("currentEpoch");
  std::string* next_epoch_time_str = root.FindString("nextEpochTime");
  if (!epoch || !next_epoch_time_str) {
    LOG(ERROR) << "StarRandomnessMeta: failed to parse server info json: "
                  "missing fields";
    ScheduleServerInfoRetry(log_type);
    return;
  }

  if (config_->fake_star_epochs.at(log_type).has_value()) {
    epoch = config_->fake_star_epochs.at(log_type);
  }

  base::Time next_epoch_time;
  if (!base::Time::FromString(next_epoch_time_str->c_str(), &next_epoch_time) ||
      next_epoch_time <= base::Time::Now()) {
    LOG(ERROR) << "StarRandomnessMeta: failed to parse server info next "
                  "epoch time";
    ScheduleServerInfoRetry(log_type);
    return;
  }
  const std::string* pk_value = root.FindString("publicKey");
  ::rust::Box<constellation::PPOPRFPublicKeyWrapper> pk =
      DecodeServerPublicKey(pk_value);

  ScopedDictPrefUpdate update(local_state_, kRandomnessMetaDictPrefName);
  base::Value::Dict* meta_type_dict =
      update->EnsureDict(MetricLogTypeToString(log_type));
  if (pk_value != nullptr) {
    meta_type_dict->Set(kCurrentPKPrefKey, *pk_value);
  }
  meta_type_dict->Set(kCurrentEpochPrefKey, *epoch);
  meta_type_dict->Set(kNextEpochTimePrefKey,
                      base::TimeToValue(next_epoch_time));

  bool epoch_change_detected = false;
  if (update_state->last_cached_epoch != epoch) {
    update_state->last_cached_epoch = epoch;
    epoch_change_detected = true;
  }

  update_state->rnd_server_info = std::make_unique<RandomnessServerInfo>(
      static_cast<uint8_t>(*epoch), next_epoch_time, epoch_change_detected,
      std::move(pk));
  update_state->current_backoff_time = base::TimeDelta();
  VLOG(2) << "StarRandomnessMeta: server info retrieved";
  info_callback_.Run(log_type, update_state->rnd_server_info.get());
}

}  // namespace p3a
