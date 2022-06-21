/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_manager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/p3a/network_annotations.h"
#include "brave/components/p3a/p3a_message.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

constexpr std::size_t kP3AStarCurrentThreshold = 50;
constexpr std::size_t kMaxRandomnessResponseSize = 131072;
constexpr char kCurrentEpochPrefName[] = "brave.p3a.current_epoch";
constexpr char kNextEpochTimePrefName[] = "brave.p3a.next_epoch_time";

std::unique_ptr<rust::Vec<nested_star::VecU8>> DecodeBase64List(
    const base::Value* list) {
  std::unique_ptr<rust::Vec<nested_star::VecU8>> result(
      new rust::Vec<nested_star::VecU8>());
  for (const base::Value& list_entry : list->GetList()) {
    const std::string* entry_str = list_entry.GetIfString();
    if (entry_str == nullptr) {
      LOG(ERROR) << "BraveP3AStarManager: list value is not string";
      return nullptr;
    }
    nested_star::VecU8 entry_dec_vec;
    absl::optional<std::vector<uint8_t>> entry_dec =
        base::Base64Decode(*entry_str);
    if (!entry_dec.has_value()) {
      LOG(ERROR) << "BraveP3AStarManager: failed to decode base64 value";
      return nullptr;
    }
    std::copy(entry_dec->cbegin(), entry_dec->cend(),
              std::back_inserter(entry_dec_vec.data));
    result->push_back(entry_dec_vec);
  }
  return result;
}

}  // namespace

BraveP3AStarManager::BraveP3AStarManager(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    StarMessageCallback message_callback,
    RandomnessServerInfoCallback info_callback,
    const GURL& randomness_server_url,
    const GURL& randomness_server_info_url,
    bool use_local_randomness)
    : current_public_key_(nested_star::get_ppoprf_null_public_key()),
      url_loader_factory_(url_loader_factory),
      local_state_(local_state),
      message_callback_(message_callback),
      info_callback_(info_callback),
      randomness_server_url_(randomness_server_url),
      randomness_server_info_url_(randomness_server_info_url),
      use_local_randomness_(use_local_randomness) {}

BraveP3AStarManager::~BraveP3AStarManager() {}

void BraveP3AStarManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kCurrentEpochPrefName, -1);
  registry->RegisterTimePref(kNextEpochTimePrefName, base::Time());
}

void BraveP3AStarManager::UpdateRandomnessServerInfo() {
  if (rnd_server_info_ == nullptr) {
    // if rnd_server_info is null, we can assume the call is from
    // initialization, and not an update call. Using cached server info, if
    // available.
    base::Time saved_next_epoch_time =
        local_state_->GetTime(kNextEpochTimePrefName);
    if (saved_next_epoch_time > base::Time::Now()) {
      int saved_epoch = local_state_->GetInteger(kCurrentEpochPrefName);
      rnd_server_info_.reset(new RandomnessServerInfo{
          .current_epoch = static_cast<uint8_t>(saved_epoch),
          .next_epoch_time = saved_next_epoch_time});
      info_callback_.Run(rnd_server_info_.get());
      return;
    }
  }
  rnd_server_info_.reset(nullptr);
  RequestRandomnessServerInfo();
}

bool BraveP3AStarManager::StartMessagePreparation(const char* histogram_name,
                                                  std::string serialized_log) {
  if (rnd_server_info_ == nullptr) {
    LOG(ERROR) << "BraveP3AStarManager: measurement preparation failed due to "
                  "unavailable server info";
    return false;
  }
  std::vector<std::string> layers =
      base::SplitString(serialized_log, kP3AMessageStarLayerSeparator,
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);

  uint8_t epoch = rnd_server_info_->current_epoch;

  auto prepare_res = nested_star::prepare_measurement(layers, epoch);
  if (!prepare_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: measurement preparation failed: "
               << prepare_res.error.c_str();
    return false;
  }

  auto req = nested_star::construct_randomness_request(*prepare_res.state);

  if (use_local_randomness_) {
    // For dev/test purposes only
    auto local_rand_res = nested_star::generate_local_randomness(req, epoch);
    if (!local_rand_res.error.empty()) {
      LOG(ERROR) << "BraveP3AStarManager: generating local randomness failed: "
                 << local_rand_res.error.c_str();
      return false;
    }

    std::string msg_output;
    if (!ConstructFinalMessage(prepare_res.state, local_rand_res.points,
                               local_rand_res.proofs, &msg_output)) {
      return false;
    }

    message_callback_.Run(
        histogram_name, rnd_server_info_->current_epoch,
        std::unique_ptr<std::string>(new std::string(msg_output)));
  } else {
    SendRandomnessRequest(histogram_name, rnd_server_info_->current_epoch,
                          std::move(prepare_res.state), req);
  }

  return true;
}

void BraveP3AStarManager::RequestRandomnessServerInfo() {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = randomness_server_info_url_;

  rnd_info_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessRequestAnnotation());

  rnd_info_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarManager::HandleRandomnessServerInfoResponse,
                     base::Unretained(this)),
      kMaxRandomnessResponseSize);
}

void BraveP3AStarManager::SendRandomnessRequest(
    const char* histogram_name,
    uint8_t epoch,
    rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    const rust::Vec<nested_star::VecU8>& rand_req_points) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = randomness_server_url_;
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
        << "BraveP3AStarManager: failed to serialize randomness req payload";
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }

  rnd_url_loader_->AttachStringForUpload(payload_str, "application/json");

  rnd_url_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&BraveP3AStarManager::HandleRandomnessResponse,
                     base::Unretained(this), histogram_name, epoch,
                     std::move(randomness_request_state)),
      kMaxRandomnessResponseSize);
}

void BraveP3AStarManager::HandleRandomnessResponse(
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
        << "BraveP3AStarManager: no response body for randomness request, "
        << "net error: " << error_str;
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  rnd_url_loader_.reset();
  base::JSONReader::ValueWithError parsed_body =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!parsed_body.value.has_value() || !parsed_body.value->is_dict()) {
    LOG(ERROR)
        << "BraveP3AStarManager: failed to parse randomness response json: "
        << parsed_body.error_message;
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  const base::Value* points_value = parsed_body.value->FindListKey("points");
  const base::Value* proofs_value = parsed_body.value->FindListKey("proofs");
  if (points_value == nullptr) {
    LOG(ERROR) << "BraveP3AStarManager: failed to find points list in "
                  "randomness response";
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<nested_star::VecU8>> points_vec =
      DecodeBase64List(points_value);
  if (points_vec == nullptr) {
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  std::unique_ptr<rust::Vec<nested_star::VecU8>> proofs_vec;
  if (proofs_value != nullptr) {
    proofs_vec = DecodeBase64List(proofs_value);
    if (!proofs_vec) {
      message_callback_.Run(histogram_name, epoch, nullptr);
      return;
    }
  } else {
    proofs_vec.reset(new rust::Vec<nested_star::VecU8>());
  }
  HandleRandomnessData(histogram_name, epoch,
                       std::move(randomness_request_state), *points_vec,
                       *proofs_vec);
}

void BraveP3AStarManager::HandleRandomnessData(
    const char* histogram_name,
    uint8_t epoch,
    ::rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    const rust::Vec<nested_star::VecU8>& resp_points,
    const rust::Vec<nested_star::VecU8>& resp_proofs) {
  if (resp_points.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: no points for randomness request";
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  std::string final_msg;
  if (!ConstructFinalMessage(randomness_request_state, resp_points, resp_proofs,
                             &final_msg)) {
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }

  message_callback_.Run(
      histogram_name, epoch,
      std::unique_ptr<std::string>(new std::string(final_msg)));
}

void BraveP3AStarManager::HandleRandomnessServerInfoResponse(
    std::unique_ptr<std::string> response_body) {
  if (!response_body || response_body->empty()) {
    std::string error_str =
        net::ErrorToShortString(rnd_info_url_loader_->NetError());
    LOG(ERROR) << "BraveP3AStarManager: no response body for randomness server "
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
    LOG(ERROR) << "BraveP3AStarManager: failed to parse server info json: "
               << parsed_value.error_message;
    info_callback_.Run(nullptr);
    return;
  }
  absl::optional<int> epoch = parsed_value.value->FindIntKey("currentEpoch");
  std::string* next_epoch_time_str =
      parsed_value.value->FindStringKey("nextEpochTime");
  if (!epoch || !next_epoch_time_str) {
    LOG(ERROR) << "BraveP3AStarManager: failed to parse server info json: "
                  "missing fields";
    info_callback_.Run(nullptr);
    return;
  }
  base::Time next_epoch_time;
  if (!base::Time::FromString(next_epoch_time_str->c_str(), &next_epoch_time) ||
      next_epoch_time <= base::Time::Now()) {
    LOG(ERROR)
        << "BraveP3AStarManager: failed to parse server info next epoch time";
    info_callback_.Run(nullptr);
    return;
  }
  local_state_->SetInteger(kCurrentEpochPrefName, *epoch);
  local_state_->SetTime(kNextEpochTimePrefName, next_epoch_time);
  rnd_server_info_.reset(
      new RandomnessServerInfo{.current_epoch = static_cast<uint8_t>(*epoch),
                               .next_epoch_time = next_epoch_time});
  info_callback_.Run(rnd_server_info_.get());
}

bool BraveP3AStarManager::ConstructFinalMessage(
    rust::Box<nested_star::RandomnessRequestStateWrapper>&
        randomness_request_state,
    const rust::Vec<nested_star::VecU8>& resp_points,
    const rust::Vec<nested_star::VecU8>& resp_proofs,
    std::string* output) {
  auto msg_res = nested_star::construct_message(
      resp_points, resp_proofs, *randomness_request_state, *current_public_key_,
      {}, kP3AStarCurrentThreshold);
  if (!msg_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: message construction failed: "
               << msg_res.error.c_str();
    return false;
  }

  *output = base::Base64Encode(msg_res.data);
  return true;
}

}  // namespace brave
