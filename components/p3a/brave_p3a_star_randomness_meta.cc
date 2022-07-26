/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_randomness_meta.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "brave/components/nitro_utils/attestation.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/network_annotations.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave {

namespace {

constexpr char kCurrentPKPrefName[] = "brave.p3a.current_pk";
constexpr char kCurrentEpochPrefName[] = "brave.p3a.current_epoch";
constexpr char kNextEpochTimePrefName[] = "brave.p3a.next_epoch_time";
constexpr std::size_t kMaxInfoResponseSize = 131072;
const int kRndInfoRetryInitialBackoffSeconds = 5;
const int kRndInfoRetryMaxBackoffMinutes = 60;

::rust::Box<nested_star::PPOPRFPublicKeyWrapper> DecodeServerPublicKey(
    const std::string* pk_base64) {
  if (pk_base64 == nullptr || pk_base64->empty()) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: no pk available, will not validate "
           "randomness";
    return nested_star::get_ppoprf_null_public_key();
  }
  absl::optional<std::vector<uint8_t>> dec_pk = base::Base64Decode(*pk_base64);
  if (!dec_pk.has_value()) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: bad pk base64, will not validate "
           "randomness";
    return nested_star::get_ppoprf_null_public_key();
  }
  ::rust::Slice<const uint8_t> dec_pk_slice{dec_pk.value().data(),
                                            dec_pk.value().size()};
  nested_star::PPOPRFPublicKeyResult pk_res =
      nested_star::load_ppoprf_public_key(dec_pk_slice);
  if (!pk_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarRandomnessMeta: failed to load pk: "
               << std::string(pk_res.error);
    return nested_star::get_ppoprf_null_public_key();
  }
  return std::move(pk_res.key);
}

}  // namespace

RandomnessServerInfo::RandomnessServerInfo(
    uint8_t current_epoch,
    base::Time next_epoch_time,
    ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> public_key)
    : current_epoch(current_epoch),
      next_epoch_time(next_epoch_time),
      public_key(std::move(public_key)) {}
RandomnessServerInfo::~RandomnessServerInfo() {}

BraveP3AStarRandomnessMeta::BraveP3AStarRandomnessMeta(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    RandomnessServerInfoCallback info_callback,
    BraveP3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      local_state_(local_state),
      info_callback_(info_callback),
      config_(config) {}

BraveP3AStarRandomnessMeta::~BraveP3AStarRandomnessMeta() {}

void BraveP3AStarRandomnessMeta::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kCurrentPKPrefName, std::string());
  registry->RegisterIntegerPref(kCurrentEpochPrefName, -1);
  registry->RegisterTimePref(kNextEpochTimePrefName, base::Time());
}

bool BraveP3AStarRandomnessMeta::VerifyRandomnessCert(
    network::SimpleURLLoader* url_loader) {
  if (config_->disable_star_attestation) {
    VLOG(2) << "BraveP3AStarRandomnessMeta: skipping approved cert check";
    return true;
  }
  const network::mojom::URLResponseHead* response_info =
      url_loader->ResponseInfo();
  if (approved_cert_ == nullptr) {
    LOG(ERROR) << "BraveP3AStarRandomnessMeta: approved cert is missing";
    AttestServer(false);
    return false;
  }
  if (!response_info->ssl_info.has_value() ||
      response_info->ssl_info->cert == nullptr) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: ssl info is missing from response info";
    return false;
  }
  if (!response_info->ssl_info->cert->EqualsIncludingChain(
          approved_cert_.get())) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: approved cert mismatch, will retry "
           "attestation";
    AttestServer(false);
    return false;
  }
  return true;
}

void BraveP3AStarRandomnessMeta::RequestServerInfo() {
  rnd_server_info_ = nullptr;

  if (!config_->disable_star_attestation && approved_cert_ == nullptr) {
    AttestServer(true);
    return;
  }

  if (!has_used_cached_info) {
    // Using cached server info, if available.
    base::Time saved_next_epoch_time =
        local_state_->GetTime(kNextEpochTimePrefName);
    if (saved_next_epoch_time > base::Time::Now()) {
      int saved_epoch = local_state_->GetInteger(kCurrentEpochPrefName);
      std::string saved_pk = local_state_->GetString(kCurrentPKPrefName);

      rnd_server_info_.reset(new RandomnessServerInfo(
          static_cast<uint8_t>(saved_epoch), saved_next_epoch_time,
          DecodeServerPublicKey(&saved_pk)));
      VLOG(2) << "BraveP3AStarRandomnessMeta: using cached server info";
      info_callback_.Run(rnd_server_info_.get());
      has_used_cached_info = true;
      return;
    }
  }

  rnd_server_info_.reset(nullptr);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(config_->star_randomness_host + "/info");

  url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessRequestAnnotation());
  url_loader_->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse);
  url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarRandomnessMeta::HandleServerInfoResponse,
                     base::Unretained(this)),
      kMaxInfoResponseSize);
}

RandomnessServerInfo*
BraveP3AStarRandomnessMeta::GetCachedRandomnessServerInfo() {
  return rnd_server_info_.get();
}

void BraveP3AStarRandomnessMeta::AttestServer(bool make_info_request_after) {
  if (attestation_pending_) {
    return;
  }
  attestation_pending_ = true;
  approved_cert_ = nullptr;

  VLOG(2) << "BraveP3AStarRandomnessMeta: starting attestation";
  GURL attestation_url = GURL(config_->star_randomness_host + "/attestation");
  nitro_utils::RequestAndVerifyAttestationDocument(
      attestation_url, url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarRandomnessMeta::HandleAttestationResult,
                     base::Unretained(this), make_info_request_after));
}

void BraveP3AStarRandomnessMeta::HandleAttestationResult(
    bool make_info_request_after,
    scoped_refptr<net::X509Certificate> approved_cert) {
  if (approved_cert == nullptr) {
    LOG(ERROR) << "BraveP3AStarRandomnessMeta: attestation failed";
    attestation_pending_ = false;
    if (make_info_request_after) {
      ScheduleServerInfoRetry();
    }
    return;
  }
  approved_cert_ = approved_cert;
  attestation_pending_ = false;
  VLOG(2) << "BraveP3AStarRandomnessMeta: attestation succeeded";
  if (make_info_request_after) {
    RequestServerInfo();
  }
}

void BraveP3AStarRandomnessMeta::ScheduleServerInfoRetry() {
  url_loader_.reset();
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
  VLOG(2) << "BraveP3AStarRandomnessMeta: scheduling server info req retry in "
          << current_backoff_time_;
  rnd_info_retry_timer_.Start(FROM_HERE, current_backoff_time_, this,
                              &BraveP3AStarRandomnessMeta::RequestServerInfo);
}

void BraveP3AStarRandomnessMeta::HandleServerInfoResponse(
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str = net::ErrorToShortString(url_loader_->NetError());
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: no response body for randomness server "
        << "info request, net error: " << error_str;
    ScheduleServerInfoRetry();
    return;
  }
  if (!VerifyRandomnessCert(url_loader_.get())) {
    ScheduleServerInfoRetry();
    return;
  }
  url_loader_.reset();
  base::JSONReader::ValueWithError parsed_value =
      base::JSONReader::ReadAndReturnValueWithError(
          *response_body, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed_value.value || !parsed_value.value->is_dict()) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: failed to parse server info json: "
        << parsed_value.error_message;
    ScheduleServerInfoRetry();
    return;
  }
  absl::optional<int> epoch = parsed_value.value->FindIntKey("currentEpoch");
  std::string* next_epoch_time_str =
      parsed_value.value->FindStringKey("nextEpochTime");
  if (!epoch || !next_epoch_time_str) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: failed to parse server info json: "
           "missing fields";
    ScheduleServerInfoRetry();
    return;
  }
  base::Time next_epoch_time;
  if (!base::Time::FromString(next_epoch_time_str->c_str(), &next_epoch_time) ||
      next_epoch_time <= base::Time::Now()) {
    LOG(ERROR)
        << "BraveP3AStarRandomnessMeta: failed to parse server info next "
           "epoch time";
    ScheduleServerInfoRetry();
    return;
  }
  const std::string* pk_value = parsed_value.value->FindStringKey("publicKey");
  ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> pk =
      DecodeServerPublicKey(pk_value);
  if (pk_value != nullptr) {
    local_state_->SetString(kCurrentPKPrefName, *pk_value);
  }
  local_state_->SetInteger(kCurrentEpochPrefName, *epoch);
  local_state_->SetTime(kNextEpochTimePrefName, next_epoch_time);
  rnd_server_info_.reset(new RandomnessServerInfo(
      static_cast<uint8_t>(*epoch), next_epoch_time, std::move(pk)));
  current_backoff_time_ = base::TimeDelta();
  LOG(ERROR) << "BraveP3AStarRandomnessMeta: server info retrieved";
  info_callback_.Run(rnd_server_info_.get());
}

}  // namespace brave
