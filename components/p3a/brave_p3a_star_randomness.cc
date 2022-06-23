/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_randomness.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/nested_star/src/lib.rs.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/network_annotations.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

constexpr char kCurrentPKPrefName[] = "brave.p3a.current_pk";
constexpr char kCurrentEpochPrefName[] = "brave.p3a.current_epoch";
constexpr char kNextEpochTimePrefName[] = "brave.p3a.next_epoch_time";
constexpr std::size_t kMaxRandomnessResponseSize = 131072;

std::unique_ptr<rust::Vec<nested_star::VecU8>> DecodeBase64List(
    const base::Value* list) {
  std::unique_ptr<rust::Vec<nested_star::VecU8>> result(
      new rust::Vec<nested_star::VecU8>());
  for (const base::Value& list_entry : list->GetList()) {
    const std::string* entry_str = list_entry.GetIfString();
    if (entry_str == nullptr) {
      LOG(ERROR) << "BraveP3AStarRandomness: list value is not string";
      return nullptr;
    }
    nested_star::VecU8 entry_dec_vec;
    absl::optional<std::vector<uint8_t>> entry_dec =
        base::Base64Decode(*entry_str);
    if (!entry_dec.has_value()) {
      LOG(ERROR) << "BraveP3AStarRandomness: failed to decode base64 value";
      return nullptr;
    }
    std::copy(entry_dec->cbegin(), entry_dec->cend(),
              std::back_inserter(entry_dec_vec.data));
    result->push_back(entry_dec_vec);
  }
  return result;
}

::rust::Box<nested_star::PPOPRFPublicKeyWrapper> DecodeServerPublicKey(
    const std::string* pk_base64) {
  if (pk_base64 == nullptr || pk_base64->empty()) {
    LOG(ERROR) << "BraveP3AStarRandomness: no pk available, will not validate "
                  "randomness";
    return nested_star::get_ppoprf_null_public_key();
  }
  absl::optional<std::vector<uint8_t>> dec_pk = base::Base64Decode(*pk_base64);
  if (!dec_pk.has_value()) {
    LOG(ERROR) << "BraveP3AStarRandomness: bad pk base64, will not validate "
                  "randomness";
    return nested_star::get_ppoprf_null_public_key();
  }
  ::rust::Slice<const uint8_t> dec_pk_slice{dec_pk.value().data(),
                                            dec_pk.value().size()};
  nested_star::PPOPRFPublicKeyResult pk_res =
      nested_star::load_ppoprf_public_key(dec_pk_slice);
  if (!pk_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarRandomness: failed to load pk: "
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

BraveP3AStarRandomness::BraveP3AStarRandomness(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    RandomnessServerInfoCallback info_callback,
    RandomnessDataCallback data_callback,
    BraveP3AConfig* config)
    : url_loader_factory_(url_loader_factory),
      local_state_(local_state),
      info_callback_(info_callback),
      data_callback_(data_callback),
      config_(config) {}

BraveP3AStarRandomness::~BraveP3AStarRandomness() {}

void BraveP3AStarRandomness::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kCurrentPKPrefName, std::string());
  registry->RegisterIntegerPref(kCurrentEpochPrefName, -1);
  registry->RegisterTimePref(kNextEpochTimePrefName, base::Time());
}

void BraveP3AStarRandomness::RequestRandomnessServerInfo() {
  if (rnd_server_info_ == nullptr) {
    // if rnd_server_info is null, we can assume the call is from
    // initialization, and not an update call. Using cached server info, if
    // available.
    base::Time saved_next_epoch_time =
        local_state_->GetTime(kNextEpochTimePrefName);
    if (saved_next_epoch_time > base::Time::Now()) {
      int saved_epoch = local_state_->GetInteger(kCurrentEpochPrefName);
      std::string saved_pk = local_state_->GetString(kCurrentPKPrefName);

      rnd_server_info_.reset(new RandomnessServerInfo(
          static_cast<uint8_t>(saved_epoch), saved_next_epoch_time,
          DecodeServerPublicKey(&saved_pk)));
      info_callback_.Run(rnd_server_info_.get());
      return;
    }
  }

  rnd_server_info_.reset(nullptr);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = config_->star_randomness_info_url;

  rnd_info_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessRequestAnnotation());

  rnd_info_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(
          &BraveP3AStarRandomness::HandleRandomnessServerInfoResponse,
          base::Unretained(this)),
      kMaxRandomnessResponseSize);
}

void BraveP3AStarRandomness::SendRandomnessRequest(
    const char* histogram_name,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    const rust::Vec<nested_star::VecU8>& rand_req_points) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = config_->star_randomness_url;
  resource_request->method = "POST";

  rnd_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessServerInfoAnnotation());

  base::Value payload_dict(base::Value::Type::DICT);
  base::Value points_list(base::Value::Type::LIST);
  for (const auto& point_data : rand_req_points) {
    points_list.Append(base::Base64Encode(point_data.data));
  }
  payload_dict.SetKey("points", std::move(points_list));
  payload_dict.SetIntKey("epoch", epoch);

  std::string payload_str;
  if (!base::JSONWriter::Write(payload_dict, &payload_str)) {
    LOG(ERROR)
        << "BraveP3AStarRandomness: failed to serialize randomness req payload";
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }

  rnd_url_loader_->AttachStringForUpload(payload_str, "application/json");

  rnd_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarRandomness::HandleRandomnessResponse,
                     base::Unretained(this), histogram_name, epoch,
                     std::move(randomness_request_state)),
      kMaxRandomnessResponseSize);
}

RandomnessServerInfo* BraveP3AStarRandomness::GetCachedRandomnessServerInfo() {
  return rnd_server_info_.get();
}

void BraveP3AStarRandomness::HandleRandomnessResponse(
    const char* histogram_name,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str =
        net::ErrorToShortString(rnd_url_loader_->NetError());
    rnd_url_loader_.reset();
    LOG(ERROR)
        << "BraveP3AStarRandomness: no response body for randomness request, "
        << "net error: " << error_str;
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  rnd_url_loader_.reset();
  base::JSONReader::ValueWithError parsed_body =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!parsed_body.value.has_value() || !parsed_body.value->is_dict()) {
    LOG(ERROR)
        << "BraveP3AStarRandomness: failed to parse randomness response json: "
        << parsed_body.error_message;
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  const base::Value* points_value = parsed_body.value->FindListKey("points");
  const base::Value* proofs_value = parsed_body.value->FindListKey("proofs");
  if (points_value == nullptr) {
    LOG(ERROR) << "BraveP3AStarRandomness: failed to find points list in "
                  "randomness response";
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<nested_star::VecU8>> points_vec =
      DecodeBase64List(points_value);
  if (points_vec == nullptr) {
    data_callback_.Run(histogram_name, epoch,
                       std::move(randomness_request_state), nullptr, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<nested_star::VecU8>> proofs_vec;
  if (proofs_value != nullptr) {
    proofs_vec = DecodeBase64List(proofs_value);
    if (!proofs_vec) {
      data_callback_.Run(histogram_name, epoch,
                         std::move(randomness_request_state), nullptr, nullptr);
      return;
    }
  } else {
    proofs_vec.reset(new rust::Vec<nested_star::VecU8>());
  }
  data_callback_.Run(histogram_name, epoch, std::move(randomness_request_state),
                     std::move(points_vec), std::move(proofs_vec));
}

void BraveP3AStarRandomness::HandleRandomnessServerInfoResponse(
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str =
        net::ErrorToShortString(rnd_info_url_loader_->NetError());
    LOG(ERROR)
        << "BraveP3AStarRandomness: no response body for randomness server "
        << "info request, net error: " << error_str;
    rnd_info_url_loader_.reset();
    info_callback_.Run(nullptr);
    return;
  }
  rnd_info_url_loader_.reset();
  base::JSONReader::ValueWithError parsed_value =
      base::JSONReader::ReadAndReturnValueWithError(
          *response_body, base::JSONParserOptions::JSON_PARSE_RFC);
  if (!parsed_value.value || !parsed_value.value->is_dict()) {
    LOG(ERROR) << "BraveP3AStarRandomness: failed to parse server info json: "
               << parsed_value.error_message;
    info_callback_.Run(nullptr);
    return;
  }
  absl::optional<int> epoch = parsed_value.value->FindIntKey("currentEpoch");
  std::string* next_epoch_time_str =
      parsed_value.value->FindStringKey("nextEpochTime");
  if (!epoch || !next_epoch_time_str) {
    LOG(ERROR) << "BraveP3AStarRandomness: failed to parse server info json: "
                  "missing fields";
    info_callback_.Run(nullptr);
    return;
  }
  base::Time next_epoch_time;
  if (!base::Time::FromString(next_epoch_time_str->c_str(), &next_epoch_time) ||
      next_epoch_time <= base::Time::Now()) {
    LOG(ERROR) << "BraveP3AStarRandomness: failed to parse server info next "
                  "epoch time";
    info_callback_.Run(nullptr);
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
  info_callback_.Run(rnd_server_info_.get());
}

}  // namespace brave
