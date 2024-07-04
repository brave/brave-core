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
#include "base/rand_util.h"
#include "base/strings/string_split.h"
#include "brave/components/p3a/features.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/p3a_config.h"
#include "brave/components/p3a/p3a_message.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace p3a {

namespace {

// Probability of submitting a true report in the Nebula protocol
//
// This is computed from the differential privacy budget ε = 1.0 as
//
//     kNebulaParticipationRate = alpha * (1 - exp(-epsilon))
//
constexpr double kNebulaParticipationRate = 0.105;

// Probability of submitting a randomized response in the Nebula protocol
//
// This sets the fraction of clients not participating by submitting
// a true report for threshold aggregration which instead submit a
// privacy-enhancing dummy report. It must be sufficiently large to
// provide privacy coverage for the size of the P3A question domain,
// but values significantly less than 1.0 improve bandwidth efficiency
// by reducing the number of reports clients must send which will not
// be aggregated.
constexpr double kNebulaScramblingRate = 0.05;

// Check and prepare a response under the Nebula protocol
//
// Decides based on the probability constants above whether
// to participate in the random sampling, and whether to
// scramble the message vector before submission.
//
// The report should only be submitted if this function
// returns true. Otherwise the message should be discarded.
bool MaybeScrambleForNebula(std::vector<std::string>* layers) {
  bool should_participate = base::RandDouble() < kNebulaParticipationRate;
  if (should_participate) {
    return true;
  }
  bool should_scramble = base::RandDouble() < kNebulaScramblingRate;
  if (should_scramble) {
    std::vector<uint8_t> random_buffer(30);
    base::RandBytes(random_buffer);

    CHECK(layers);
    CHECK(!layers->empty());
    (*layers)[0] = base::Base64Encode(random_buffer);
    return true;
  }
  return false;
}

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
          config),
      message_callback_(message_callback),
      null_public_key_(constellation::get_ppoprf_null_public_key()) {}

ConstellationHelper::~ConstellationHelper() = default;

void ConstellationHelper::RegisterPrefs(PrefRegistrySimple* registry) {
  StarRandomnessMeta::RegisterPrefs(registry);
}

void ConstellationHelper::UpdateRandomnessServerInfo(MetricLogType log_type) {
  rand_meta_manager_.RequestServerInfo(log_type);
}

bool ConstellationHelper::StartMessagePreparation(std::string histogram_name,
                                                  MetricLogType log_type,
                                                  std::string serialized_log,
                                                  bool is_nebula) {
  auto* rnd_server_info =
      rand_meta_manager_.GetCachedRandomnessServerInfo(log_type);
  if (rnd_server_info == nullptr) {
    LOG(ERROR) << "ConstellationHelper: measurement preparation failed due to "
                  "unavailable server info";
    return false;
  }

  uint8_t epoch = rnd_server_info->current_epoch;

  std::vector<std::string> layers =
      base::SplitString(serialized_log, kP3AMessageConstellationLayerSeparator,
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);

  if (is_nebula && !MaybeScrambleForNebula(&layers)) {
    // Do not send measurement since client is unable to participate,
    // but mark the request as successful so the client does not retry
    // transmission.
    message_callback_.Run(histogram_name, log_type, epoch, true, nullptr);
    return true;
  }

  auto prepare_res = constellation::prepare_measurement(layers, epoch);
  if (!prepare_res.error.empty()) {
    LOG(ERROR) << "ConstellationHelper: measurement preparation failed: "
               << prepare_res.error.c_str();
    return false;
  }

  auto req = constellation::construct_randomness_request(*prepare_res.state);

  rand_points_manager_.SendRandomnessRequest(
      log_type, epoch, &rand_meta_manager_, req,
      base::BindOnce(&ConstellationHelper::HandleRandomnessData,
                     base::Unretained(this), histogram_name, log_type,
                     rnd_server_info->current_epoch, is_nebula,
                     std::move(prepare_res.state)));

  return true;
}

void ConstellationHelper::HandleRandomnessData(
    std::string histogram_name,
    MetricLogType log_type,
    uint8_t epoch,
    bool is_nebula,
    ::rust::Box<constellation::RandomnessRequestStateWrapper>
        randomness_request_state,
    std::unique_ptr<rust::Vec<constellation::VecU8>> resp_points,
    std::unique_ptr<rust::Vec<constellation::VecU8>> resp_proofs) {
  if (resp_points == nullptr || resp_proofs == nullptr) {
    message_callback_.Run(histogram_name, log_type, epoch, false, nullptr);
    return;
  }
  if (resp_points->empty()) {
    LOG(ERROR) << "ConstellationHelper: no points for randomness request";
    message_callback_.Run(histogram_name, log_type, epoch, false, nullptr);
    return;
  }

  size_t threshold =
      is_nebula ? kNebulaThreshold : kConstellationDefaultThreshold;

  std::string final_msg;
  if (!ConstructFinalMessage(log_type, threshold, randomness_request_state,
                             *resp_points, *resp_proofs, &final_msg)) {
    message_callback_.Run(histogram_name, log_type, epoch, false, nullptr);
    return;
  }

  message_callback_.Run(histogram_name, log_type, epoch, true,
                        std::make_unique<std::string>(final_msg));
}

bool ConstellationHelper::ConstructFinalMessage(
    MetricLogType log_type,
    size_t threshold,
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
      {}, threshold);
  if (!msg_res.error.empty()) {
    LOG(ERROR) << "ConstellationHelper: message construction failed: "
               << msg_res.error.c_str();
    return false;
  }

  *output = base::Base64Encode(msg_res.data);
  return true;
}

}  // namespace p3a
