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
#include "brave/components/nested_star/src/lib.rs.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave {

struct MessageMetainfo;

class BraveP3AStarManager {
 public:
  using StarMessageCallback = base::RepeatingCallback<void(
      const char* histogram_name,
      uint8_t epoch,
      std::unique_ptr<std::string> serialized_message)>;

  BraveP3AStarManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      StarMessageCallback message_callback,
      const GURL& randomness_server_url,
      bool use_local_randomness);
  ~BraveP3AStarManager();

  bool StartMessagePreparation(const char* histogram_name,
                               uint8_t epoch,
                               std::string serialized_log);

  bool ConstructFinalMessage(
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>&
          randomness_request_state,
      const std::string& rand_resp_data,
      std::string* output);

 private:
  void SendRandomnessRequest(
      const char* histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::string rand_req_data);

  void HandleRandomnessResponse(
      const char* histogram_name,
      uint8_t epoch,
      ::rust::Box<nested_star::RandomnessRequestStateWrapper>
          randomness_request_state,
      std::unique_ptr<std::string> response_body);

  ::rust::Box<nested_star::PPOPRFPublicKeyWrapper> current_public_key_;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  StarMessageCallback message_callback_;

  GURL randomness_server_url_;
  bool use_local_randomness_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_STAR_MANAGER_H_
