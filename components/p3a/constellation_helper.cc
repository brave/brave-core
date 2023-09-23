/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/constellation_helper.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace p3a {

namespace {

constexpr std::size_t kP3AConstellationCurrentThreshold = 50;

}  // namespace

ConstellationHelper::ConstellationHelper(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    ConstellationMessageCallback message_callback,
    StarRandomnessMeta::RandomnessServerInfoCallback info_callback,
    const P3AConfig* config)
    : rand_meta_manager_(local_state,
                         url_loader_factory,
                         info_callback,
                         config),
      rand_points_manager_(
          url_loader_factory,
          base::BindRepeating(&ConstellationHelper::HandleRandomnessData,
                              base::Unretained(this)),
          config),
      message_callback_(message_callback),
      null_public_key_(constellation::get_ppoprf_null_public_key()) {}

ConstellationHelper::~ConstellationHelper() {}

void ConstellationHelper::RegisterPrefs(PrefRegistrySimple* registry) {
  StarRandomnessMeta::RegisterPrefs(registry);
}

void ConstellationHelper::UpdateRandomnessServerInfo(MetricLogType log_type) {
  rand_meta_manager_.RequestServerInfo(log_type);
}

bool ConstellationHelper::StartMessagePreparation(std::string histogram_name,
                                                  MetricLogType log_type,
                                                  std::string serialized_log) {
  auto* rnd_server_info =
      rand_meta_manager_.GetCachedRandomnessServerInfo(log_type);
  if (rnd_server_info == nullptr) {
    LOG(ERROR) << "ConstellationHelper: measurement preparation failed due to "
                  "unavailable server info";
    return false;
  }
  std::vector<std::string> layers =
      base::SplitString(serialized_log, kP3AMessageConstellationLayerSeparator,
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);

  uint8_t epoch = rnd_server_info->current_epoch;

  auto prepare_res = constellation::prepare_measurement(layers, epoch);
  if (!prepare_res.error.empty()) {
    LOG(ERROR) << "ConstellationHelper: measurement preparation failed: "
               << prepare_res.error.c_str();
    return false;
  }

  auto req = constellation::construct_randomness_request(*prepare_res.state);

  rand_points_manager_.SendRandomnessRequest(
      histogram_name, log_type, rnd_server_info->current_epoch,
      &rand_meta_manager_, std::move(prepare_res.state), req);

  return true;
}

void ConstellationHelper::HandleRandomnessData(
    std::string histogram_name,
    MetricLogType log_type,
    uint8_t epoch,
    ::rust::Box<constellation::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::unique_ptr<rust::Vec<constellation::VecU8>> resp_points,
    std::unique_ptr<rust::Vec<constellation::VecU8>> resp_proofs) {
  if (resp_points == nullptr || resp_proofs == nullptr) {
    message_callback_.Run(histogram_name, log_type, epoch, nullptr);
    return;
  }
  if (resp_points->empty()) {
    LOG(ERROR) << "ConstellationHelper: no points for randomness request";
    message_callback_.Run(histogram_name, log_type, epoch, nullptr);
    return;
  }
  std::string final_msg;
  if (!ConstructFinalMessage(log_type, randomness_request_state, *resp_points,
                             *resp_proofs, &final_msg)) {
    message_callback_.Run(histogram_name, log_type, epoch, nullptr);
    return;
  }

  message_callback_.Run(histogram_name, log_type, epoch,
                        std::make_unique<std::string>(final_msg));
}

bool ConstellationHelper::ConstructFinalMessage(
    MetricLogType log_type,
    rust::Box<constellation::RandomnessRequestStateWrapper>&
        randomness_request_state,
    const rust::Vec<constellation::VecU8>& resp_points,
    const rust::Vec<constellation::VecU8>& resp_proofs,
    std::string* output) {
  auto* rnd_server_info =
      rand_meta_manager_.GetCachedRandomnessServerInfo(log_type);
  if (!rnd_server_info) {
    LOG(ERROR) << "ConstellationHelper: failed to get server info while "
                  "constructing message";
    return false;
  }
  auto msg_res = constellation::construct_message(
      resp_points, resp_proofs, *randomness_request_state,
      resp_proofs.empty() ? *null_public_key_ : *rnd_server_info->public_key,
      {}, kP3AConstellationCurrentThreshold);
  if (!msg_res.error.empty()) {
    LOG(ERROR) << "ConstellationHelper: message construction failed: "
               << msg_res.error.c_str();
    return false;
  }

  *output = base::Base64Encode(msg_res.data);
  return true;
}

}  // namespace p3a
