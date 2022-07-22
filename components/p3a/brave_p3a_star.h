/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece_forward.h"
#include "brave/components/nested_star/src/lib.rs.h"
#include "brave/components/p3a/brave_p3a_star_randomness.h"

class PrefService;
class PrefRegistrySimple;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave {

class MessageMetainfo;

struct BraveP3AConfig;

class BraveP3AStar {
 public:
  using StarMessageCallback = base::RepeatingCallback<void(
      std::string histogram_name,
      uint8_t epoch,
      std::unique_ptr<std::string> serialized_message)>;

  BraveP3AStar(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      StarMessageCallback message_callback,
      BraveP3AStarRandomness::RandomnessServerInfoCallback info_callback,
      BraveP3AConfig* config);
  ~BraveP3AStar();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateRandomnessServerInfo();

  bool StartMessagePreparation(std::string histogram_name,
                               std::string serialized_log);

 private:
  void HandleRandomnessData(
      std::string histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::unique_ptr<rust::Vec<nested_star::VecU8>> resp_points,
      std::unique_ptr<rust::Vec<nested_star::VecU8>> resp_proofs);

  bool ConstructFinalMessage(
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>&
          randomness_request_state,
      const rust::Vec<nested_star::VecU8>& resp_points,
      const rust::Vec<nested_star::VecU8>& resp_proofs,
      std::string* output);

  BraveP3AStarRandomness randomness_manager_;

  StarMessageCallback message_callback_;

  ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> null_public_key_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_H_
