/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_POINTS_H_
#define BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_POINTS_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "brave/components/p3a/constellation/rs/cxx/src/lib.rs.h"
#include "brave/components/p3a/metric_log_type.h"
#include "brave/components/p3a/star_randomness_meta.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace p3a {

struct P3AConfig;

// Handles sending requests/handling responses to/from the randomness
// server in order to receive randomness point data for STAR measurements.
class StarRandomnessPoints {
 public:
  using RandomnessDataCallback = base::OnceCallback<void(
      std::unique_ptr<rust::Vec<constellation::VecU8>> resp_points,
      std::unique_ptr<rust::Vec<constellation::VecU8>> resp_proofs)>;

  StarRandomnessPoints(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const P3AConfig* config);
  ~StarRandomnessPoints();
  StarRandomnessPoints(const StarRandomnessPoints&) = delete;
  StarRandomnessPoints& operator=(const StarRandomnessPoints&) = delete;

  void SendRandomnessRequest(
      MetricLogType log_type,
      uint8_t epoch,
      StarRandomnessMeta* randomness_meta,
      const rust::Vec<constellation::VecU8>& rand_req_points,
      RandomnessDataCallback callback);

 private:
  void HandleRandomnessResponse(MetricLogType log_type,
                                StarRandomnessMeta* randomness_meta,
                                RandomnessDataCallback callback,
                                std::unique_ptr<std::string> response_body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::flat_map<MetricLogType, std::unique_ptr<network::SimpleURLLoader>>
      url_loaders_;

  const raw_ptr<const P3AConfig> config_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_STAR_RANDOMNESS_POINTS_H_
