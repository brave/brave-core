/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_p3a_star.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave {

namespace {

constexpr std::size_t kP3AStarCurrentThreshold = 50;

}  // namespace

BraveP3AStar::BraveP3AStar(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    StarMessageCallback message_callback,
    BraveP3AStarRandomnessMeta::RandomnessServerInfoCallback info_callback,
    BraveP3AConfig* config)
    : rand_meta_manager_(local_state,
                         url_loader_factory,
                         info_callback,
                         config),
      rand_points_manager_(
          url_loader_factory,
          base::BindRepeating(&BraveP3AStar::HandleRandomnessData,
                              base::Unretained(this)),
          config),
      message_callback_(message_callback),
      null_public_key_(nested_star::get_ppoprf_null_public_key()) {
  UpdateRandomnessServerInfo();
}

BraveP3AStar::~BraveP3AStar() {}

void BraveP3AStar::RegisterPrefs(PrefRegistrySimple* registry) {
  BraveP3AStarRandomnessMeta::RegisterPrefs(registry);
}

void BraveP3AStar::UpdateRandomnessServerInfo() {
  rand_meta_manager_.RequestServerInfo();
}

bool BraveP3AStar::StartMessagePreparation(std::string histogram_name,
                                           std::string serialized_log) {
  auto* rnd_server_info = rand_meta_manager_.GetCachedRandomnessServerInfo();
  if (rnd_server_info == nullptr) {
    LOG(ERROR) << "BraveP3AStar: measurement preparation failed due to "
                  "unavailable server info";
    return false;
  }
  std::vector<std::string> layers =
      base::SplitString(serialized_log, kP3AMessageStarLayerSeparator,
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);

  uint8_t epoch = rnd_server_info->current_epoch;

  auto prepare_res = nested_star::prepare_measurement(layers, epoch);
  if (!prepare_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStar: measurement preparation failed: "
               << prepare_res.error.c_str();
    return false;
  }

  auto req = nested_star::construct_randomness_request(*prepare_res.state);

  rand_points_manager_.SendRandomnessRequest(
      histogram_name, &rand_meta_manager_, rnd_server_info->current_epoch,
      std::move(prepare_res.state), req);

  return true;
}

void BraveP3AStar::HandleRandomnessData(
    std::string histogram_name,
    uint8_t epoch,
    ::rust::Box<nested_star::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::unique_ptr<rust::Vec<nested_star::VecU8>> resp_points,
    std::unique_ptr<rust::Vec<nested_star::VecU8>> resp_proofs) {
  if (resp_points == nullptr || resp_proofs == nullptr) {
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  if (resp_points->empty()) {
    LOG(ERROR) << "BraveP3AStar: no points for randomness request";
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }
  std::string final_msg;
  if (!ConstructFinalMessage(randomness_request_state, *resp_points,
                             *resp_proofs, &final_msg)) {
    message_callback_.Run(histogram_name, epoch, nullptr);
    return;
  }

  message_callback_.Run(
      histogram_name, epoch,
      std::unique_ptr<std::string>(new std::string(final_msg)));
}

bool BraveP3AStar::ConstructFinalMessage(
    rust::Box<nested_star::RandomnessRequestStateWrapper>&
        randomness_request_state,
    const rust::Vec<nested_star::VecU8>& resp_points,
    const rust::Vec<nested_star::VecU8>& resp_proofs,
    std::string* output) {
  auto* rnd_server_info = rand_meta_manager_.GetCachedRandomnessServerInfo();
  DCHECK(rnd_server_info);
  auto msg_res = nested_star::construct_message(
      resp_points, resp_proofs, *randomness_request_state,
      resp_proofs.empty() ? *null_public_key_ : *rnd_server_info->public_key,
      {}, kP3AStarCurrentThreshold);
  if (!msg_res.error.empty()) {
    LOG(ERROR) << "BraveP3AStar: message construction failed: "
               << msg_res.error.c_str();
    return false;
  }

  *output = base::Base64Encode(msg_res.data);
  return true;
}

}  // namespace brave
