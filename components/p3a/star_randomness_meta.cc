/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/star_randomness_meta.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/network_annotations.h"
#include "brave/components/p3a/nitro_utils/attestation.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace p3a {

namespace {

constexpr char kCurrentPKPrefName[] = "brave.p3a.current_pk";
constexpr char kCurrentEpochPrefName[] = "brave.p3a.current_epoch";
constexpr char kNextEpochTimePrefName[] = "brave.p3a.next_epoch_time";
constexpr char kApprovedCertFPPrefName[] = "brave.p3a.approved_cert_fp";
// A generous arbitrary limit, 128KB
constexpr std::size_t kMaxInfoResponseSize = 128 * 1024;
const int kRndInfoRetryInitialBackoffSeconds = 5;
const int kRndInfoRetryMaxBackoffMinutes = 60;

::rust::Box<constellation::PPOPRFPublicKeyWrapper> DecodeServerPublicKey(
    const std::string* pk_base64) {
  if (pk_base64 == nullptr || pk_base64->empty()) {
    VLOG(2) << "StarRandomnessMeta: no pk available, will not validate "
               "randomness";
    return constellation::get_ppoprf_null_public_key();
  }
  absl::optional<std::vector<uint8_t>> dec_pk = base::Base64Decode(*pk_base64);
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
RandomnessServerInfo::~RandomnessServerInfo() {}

StarRandomnessMeta::StarRandomnessMeta(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    RandomnessServerInfoCallback info_callback,
    const P3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      local_state_(local_state),
      info_callback_(info_callback),
      config_(config) {
  std::string approved_cert_fp_str =
      local_state->GetString(kApprovedCertFPPrefName);
  net::HashValue approved_cert_fp;
  if (!approved_cert_fp_str.empty() &&
      approved_cert_fp.FromString(approved_cert_fp_str)) {
    VLOG(2) << "StarRandomnessMeta: loaded cached approved cert";
    approved_cert_fp_ = absl::make_optional(approved_cert_fp);
  }
}

StarRandomnessMeta::~StarRandomnessMeta() {}

void StarRandomnessMeta::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kCurrentPKPrefName, std::string());
  registry->RegisterIntegerPref(kCurrentEpochPrefName, -1);
  registry->RegisterTimePref(kNextEpochTimePrefName, base::Time());
  registry->RegisterStringPref(kApprovedCertFPPrefName, "");
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

void StarRandomnessMeta::RequestServerInfo() {
  rnd_server_info_ = nullptr;

  if (ShouldAttestEnclave() && !approved_cert_fp_.has_value()) {
    AttestServer(true);
    return;
  }

  if (!has_used_cached_info_) {
    // Using cached server info, if available.
    last_cached_epoch_ =
        absl::make_optional(local_state_->GetInteger(kCurrentEpochPrefName));
    base::Time saved_next_epoch_time =
        local_state_->GetTime(kNextEpochTimePrefName);
    // only return cached info if the epoch is not expired,
    // and if the "fake" star epoch matches the last saved epoch (if specified).
    // if "fake" star epoch does not match the saved epoch, then fresh server
    // info should be requested to update the local state with the new epoch
    // info
    if (saved_next_epoch_time > base::Time::Now() &&
        (!config_->fake_star_epoch.has_value() ||
         config_->fake_star_epoch == last_cached_epoch_)) {
      std::string saved_pk = local_state_->GetString(kCurrentPKPrefName);

      rnd_server_info_ = std::make_unique<RandomnessServerInfo>(
          *last_cached_epoch_, saved_next_epoch_time, false,
          DecodeServerPublicKey(&saved_pk));
      VLOG(2) << "StarRandomnessMeta: using cached server info";
      info_callback_.Run(rnd_server_info_.get());
      has_used_cached_info_ = true;
      return;
    }
  }

  rnd_server_info_ = nullptr;
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url =
      GURL(base::StrCat({config_->star_randomness_host, "/info"}));
  if (!resource_request->url.is_valid() ||
      !resource_request->url.SchemeIsHTTPOrHTTPS()) {
    VLOG(2) << "StarRandomnessMeta: star randomness host invalid, skipping "
               "server info request";
    return;
  }

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessRequestAnnotation());
  url_loader_->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse);
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&StarRandomnessMeta::HandleServerInfoResponse,
                     base::Unretained(this)),
      kMaxInfoResponseSize);
}

RandomnessServerInfo* StarRandomnessMeta::GetCachedRandomnessServerInfo() {
  return rnd_server_info_.get();
}

void StarRandomnessMeta::AttestServer(bool make_info_request_after) {
  if (attestation_pending_) {
    return;
  }
  attestation_pending_ = true;
  approved_cert_fp_ = absl::nullopt;

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
      ScheduleServerInfoRetry();
    }
    return;
  }
  approved_cert_fp_ = absl::make_optional(
      net::HashValue(approved_cert->CalculateChainFingerprint256()));
  std::string approved_cert_fp_str = approved_cert_fp_->ToString();
  local_state_->SetString(kApprovedCertFPPrefName, approved_cert_fp_str);
  attestation_pending_ = false;
  VLOG(2) << "StarRandomnessMeta: attestation succeeded; fp = "
          << approved_cert_fp_str;
  if (make_info_request_after) {
    RequestServerInfo();
  }
}

void StarRandomnessMeta::ScheduleServerInfoRetry() {
  url_loader_ = nullptr;
  if (current_backoff_time_.is_zero()) {
    current_backoff_time_ = base::Seconds(kRndInfoRetryInitialBackoffSeconds);
  } else {
    current_backoff_time_ *= 2;
    base::TimeDelta max_backoff_time =
        base::Minutes(kRndInfoRetryMaxBackoffMinutes);
    if (current_backoff_time_ > max_backoff_time) {
      current_backoff_time_ = max_backoff_time;
    }
  }
  VLOG(2) << "StarRandomnessMeta: scheduling server info req retry in "
          << current_backoff_time_;
  rnd_info_retry_timer_.Start(FROM_HERE, current_backoff_time_, this,
                              &StarRandomnessMeta::RequestServerInfo);
}

void StarRandomnessMeta::HandleServerInfoResponse(
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str = net::ErrorToShortString(url_loader_->NetError());
    LOG(ERROR) << "StarRandomnessMeta: no response body for randomness server "
               << "info request, net error: " << error_str;
    ScheduleServerInfoRetry();
    return;
  }
  if (!VerifyRandomnessCert(url_loader_.get())) {
    ScheduleServerInfoRetry();
    return;
  }
  url_loader_ = nullptr;
  base::JSONReader::Result parsed_value =
      base::JSONReader::ReadAndReturnValueWithError(
          *response_body, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed_value.has_value() || !parsed_value.value().is_dict()) {
    LOG(ERROR) << "StarRandomnessMeta: failed to parse server info json: "
               << parsed_value.error().message;
    ScheduleServerInfoRetry();
    return;
  }
  base::Value::Dict& root = parsed_value->GetDict();
  absl::optional<int> epoch = root.FindInt("currentEpoch");
  std::string* next_epoch_time_str = root.FindString("nextEpochTime");
  if (!epoch || !next_epoch_time_str) {
    LOG(ERROR) << "StarRandomnessMeta: failed to parse server info json: "
                  "missing fields";
    ScheduleServerInfoRetry();
    return;
  }

  if (config_->fake_star_epoch.has_value()) {
    epoch = config_->fake_star_epoch;
  }

  base::Time next_epoch_time;
  if (!base::Time::FromString(next_epoch_time_str->c_str(), &next_epoch_time) ||
      next_epoch_time <= base::Time::Now()) {
    LOG(ERROR) << "StarRandomnessMeta: failed to parse server info next "
                  "epoch time";
    ScheduleServerInfoRetry();
    return;
  }
  const std::string* pk_value = root.FindString("publicKey");
  ::rust::Box<constellation::PPOPRFPublicKeyWrapper> pk =
      DecodeServerPublicKey(pk_value);
  if (pk_value != nullptr) {
    local_state_->SetString(kCurrentPKPrefName, *pk_value);
  }
  local_state_->SetInteger(kCurrentEpochPrefName, *epoch);
  local_state_->SetTime(kNextEpochTimePrefName, next_epoch_time);

  bool epoch_change_detected = false;
  if (last_cached_epoch_ != epoch) {
    last_cached_epoch_ = epoch;
    epoch_change_detected = true;
  }

  rnd_server_info_ = std::make_unique<RandomnessServerInfo>(
      static_cast<uint8_t>(*epoch), next_epoch_time, epoch_change_detected,
      std::move(pk));
  current_backoff_time_ = base::TimeDelta();
  VLOG(2) << "StarRandomnessMeta: server info retrieved";
  info_callback_.Run(rnd_server_info_.get());
}

}  // namespace p3a
