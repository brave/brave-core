/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star_manager.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_reader.h"
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

  auto prepare_res =
      nested_star::prepare_measurement(layers, rnd_server_info_->current_epoch);
  if (!prepare_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: measurement preparation failed: "
               << prepare_res.error.c_str();
    return false;
  }

  auto req_res = nested_star::construct_randomness_request(*prepare_res.state);
  if (!req_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: randomness req preparation failed: "
               << req_res.error.c_str();
    return false;
  }

  if (use_local_randomness_) {
    // For dev/test purposes only
    auto local_rand_res = nested_star::generate_local_randomness(req_res.data);
    if (!local_rand_res.error.empty()) {
      LOG(ERROR) << "BraveP3AStarManager: generating local randomness failed: "
                 << local_rand_res.error.c_str();
      return false;
    }

    std::string local_rand_data(local_rand_res.data);

    std::string msg_output;
    if (!ConstructFinalMessage(prepare_res.state, local_rand_data,
                               &msg_output)) {
      return false;
    }

    message_callback_.Run(
        histogram_name, rnd_server_info_->current_epoch,
        std::unique_ptr<std::string>(new std::string(msg_output)));
  } else {
    SendRandomnessRequest(histogram_name, rnd_server_info_->current_epoch,
                          std::move(prepare_res.state),
                          std::string(req_res.data));
  }

  return true;
}

void BraveP3AStarManager::RequestRandomnessServerInfo() {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = randomness_server_url_;

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
    std::string rand_req_data) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = randomness_server_url_;
  resource_request->method = "POST";

  rnd_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetRandomnessServerInfoAnnotation());

  rnd_url_loader_->AttachStringForUpload(rand_req_data, "application/json");

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
  rnd_url_loader_.reset();
  if (!response_body || response_body->empty()) {
    LOG(ERROR)
        << "BraveP3AStarManager: no response body for randomness request";
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  std::string final_msg;
  if (!ConstructFinalMessage(randomness_request_state, *response_body,
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
  rnd_info_url_loader_.reset();
  if (!response_body || response_body->empty()) {
    LOG(ERROR) << "BraveP3AStarManager: no response body for randomness server "
                  "info request";
    info_callback_.Run(nullptr);
    return;
  }
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
    const std::string& rand_resp_data,
    std::string* output) {
  auto msg_res = nested_star::construct_message(
      rand_resp_data, *randomness_request_state, *current_public_key_, {},
      kP3AStarCurrentThreshold);
  if (!msg_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStarManager: message construction failed: "
               << msg_res.error.c_str();
    return false;
  }

  *output = base::Base64Encode(msg_res.data);
  return true;
}

}  // namespace brave
