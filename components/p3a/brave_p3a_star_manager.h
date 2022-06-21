/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_MANAGER_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_MANAGER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece_forward.h"
#include "base/time/time.h"
#include "brave/components/nested_star/src/lib.rs.h"
#include "url/gurl.h"

class PrefService;
class PrefRegistrySimple;

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave {

struct MessageMetainfo;

struct RandomnessServerInfo {
  uint8_t current_epoch;
  base::Time next_epoch_time;
};

class BraveP3AStarManager {
 public:
  using StarMessageCallback = base::RepeatingCallback<void(
      const char* histogram_name,
      uint8_t epoch,
      std::unique_ptr<std::string> serialized_message)>;
  using RandomnessServerInfoCallback =
      base::RepeatingCallback<void(RandomnessServerInfo* server_info)>;

  BraveP3AStarManager(
      PrefService* local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      StarMessageCallback message_callback,
      RandomnessServerInfoCallback info_callback,
      const GURL& randomness_server_url,
      const GURL& randomness_server_info_url,
      bool use_local_randomness);
  ~BraveP3AStarManager();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  void UpdateRandomnessServerInfo();

  bool StartMessagePreparation(const char* histogram_name,
                               std::string serialized_log);

 private:
  void RequestRandomnessServerInfo();

  void SendRandomnessRequest(
      const char* histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      const rust::Vec<nested_star::VecU8>& rand_req_points);

  void HandleRandomnessResponse(
      const char* histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::unique_ptr<std::string> response_body);

  void HandleRandomnessData(
      const char* histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      const rust::Vec<nested_star::VecU8>& resp_points,
      const rust::Vec<nested_star::VecU8>& resp_proofs);

  void HandleRandomnessServerInfoResponse(
      std::unique_ptr<std::string> response_body);

  bool ConstructFinalMessage(
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>&
          randomness_request_state,
      const rust::Vec<nested_star::VecU8>& resp_points,
      const rust::Vec<nested_star::VecU8>& resp_proofs,
      std::string* output);

  ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> current_public_key_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> rnd_url_loader_;
  std::unique_ptr<network::SimpleURLLoader> rnd_info_url_loader_;

  PrefService* local_state_;

  StarMessageCallback message_callback_;
  RandomnessServerInfoCallback info_callback_;

  std::unique_ptr<RandomnessServerInfo> rnd_server_info_;

  GURL randomness_server_url_;
  GURL randomness_server_info_url_;
  bool use_local_randomness_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_MANAGER_H_
