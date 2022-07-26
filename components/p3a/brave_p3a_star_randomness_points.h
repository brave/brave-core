/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_RANDOMNESS_POINTS_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_RANDOMNESS_POINTS_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "brave/components/nested_star/src/lib.rs.h"
#include "brave/components/p3a/brave_p3a_config.h"
#include "brave/components/p3a/brave_p3a_star_randomness_meta.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave {

struct BraveP3AConfig;

class BraveP3AStarRandomnessPoints {
 public:
  using RandomnessDataCallback = base::RepeatingCallback<void(
      std::string histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::unique_ptr<rust::Vec<nested_star::VecU8>> resp_points,
      std::unique_ptr<rust::Vec<nested_star::VecU8>> resp_proofs)>;

  BraveP3AStarRandomnessPoints(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      RandomnessDataCallback data_callback,
      BraveP3AConfig* config);
  ~BraveP3AStarRandomnessPoints();

  void SendRandomnessRequest(
      std::string histogram_name,
      BraveP3AStarRandomnessMeta* randomness_meta,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      const rust::Vec<nested_star::VecU8>& rand_req_points);

 private:
  void HandleRandomnessResponse(
      std::string histogram_name,
      BraveP3AStarRandomnessMeta* randomness_meta,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::unique_ptr<std::string> response_body);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  RandomnessDataCallback data_callback_;

  BraveP3AConfig* config_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_RANDOMNESS_POINTS_H_
