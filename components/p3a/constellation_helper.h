/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_CONSTELLATION_HELPER_H_
#define BRAVE_COMPONENTS_P3A_CONSTELLATION_HELPER_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "brave/components/p3a/constellation/rs/cxx/src/lib.rs.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/star_randomness_meta.h"
#include "brave/components/p3a/star_randomness_points.h"

class PrefService;
class PrefRegistrySimple;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace p3a {

// Heuristic aggregation threshold used for STAR/Constellation
inline constexpr size_t kConstellationDefaultThreshold = 50;

// Aggregation threshold used for Nebula
//
// This is derived from the differential privacy paramaters
// ε = 1.0 (the privacy budget) and δ = 1.0e-8 (should be
// less than the reciprocal of the number of clients) along
// with a parameter α = 1/6 which adjusts the tradeoff between
// this threshold and the sampling probability.
//
// This aggregation threshold is computed as
//
//     kNebulaThreshold = ceil(log(1.0/delta) / Ca)
//     where Ca = log(1.0/alpha) - 1.0 / (1.0 + alpha)
//
inline constexpr size_t kNebulaThreshold = 20;

struct P3AConfig;

// Class that contains high-level methods for preparing/generating
// Constellation encrypted measurements.
class ConstellationHelper {
 public:
  using ConstellationMessageCallback = base::RepeatingCallback<void(
      std::string histogram_name,
      MetricLogType log_type,
      uint8_t epoch,
      bool is_success,
      std::unique_ptr<std::string> serialized_message)>;

  ConstellationHelper(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      ConstellationMessageCallback message_callback,
      StarRandomnessMeta::RandomnessServerInfoCallback info_callback,
      const P3AConfig* config);
  ~ConstellationHelper();

  ConstellationHelper(const ConstellationHelper&) = delete;
  ConstellationHelper& operator=(const ConstellationHelper&) = delete;

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateRandomnessServerInfo(MetricLogType log_type);

  bool StartMessagePreparation(std::string histogram_name,
                               MetricLogType log_type,
                               std::string serialized_log,
                               bool is_nebula);

 private:
  void HandleRandomnessData(
      std::string histogram_name,
      MetricLogType log_type,
      uint8_t epoch,
      bool is_nebula,
      ::rust::Box<constellation::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::unique_ptr<rust::Vec<constellation::VecU8>> resp_points,
      std::unique_ptr<rust::Vec<constellation::VecU8>> resp_proofs);

  bool ConstructFinalMessage(
      MetricLogType log_type,
      size_t threshold,
      ::rust::Box<constellation::RandomnessRequestStateWrapper>&
          randomness_request_state,
      const rust::Vec<constellation::VecU8>& resp_points,
      const rust::Vec<constellation::VecU8>& resp_proofs,
      std::string* output);

  StarRandomnessMeta rand_meta_manager_;
  StarRandomnessPoints rand_points_manager_;

  ConstellationMessageCallback message_callback_;

  ::rust::Box<constellation::PPOPRFPublicKeyWrapper> null_public_key_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_CONSTELLATION_HELPER_H_
