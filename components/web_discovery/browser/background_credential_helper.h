/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/web_discovery/browser/rsa.h"
#include "crypto/rsa_private_key.h"

namespace web_discovery {

struct GenerateJoinRequestResult {
  GenerateJoinRequestResult(std::string join_request_b64,
                            std::vector<uint8_t> join_gsk,
                            std::string signature);

  ~GenerateJoinRequestResult();

  GenerateJoinRequestResult(const GenerateJoinRequestResult&);
  GenerateJoinRequestResult& operator=(const GenerateJoinRequestResult&);

  std::string join_request_b64;
  std::vector<uint8_t> join_gsk;
  std::string signature;
};

class BackgroundCredentialHelper {
 public:
  static std::unique_ptr<BackgroundCredentialHelper> Create();

  virtual ~BackgroundCredentialHelper() = default;

  virtual void UseFixedSeedForTesting() = 0;

  virtual std::unique_ptr<RSAKeyInfo> GenerateRSAKey() = 0;
  virtual void SetRSAKey(
      std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key) = 0;
  virtual std::optional<GenerateJoinRequestResult> GenerateJoinRequest(
      std::string pre_challenge) = 0;
  virtual std::optional<std::string> FinishJoin(
      std::string date,
      std::vector<uint8_t> group_pub_key,
      std::vector<uint8_t> gsk,
      std::vector<uint8_t> join_resp_bytes) = 0;
  virtual std::optional<std::vector<uint8_t>> PerformSign(
      std::vector<uint8_t> msg,
      std::vector<uint8_t> basename,
      std::optional<std::vector<uint8_t>> gsk_bytes,
      std::optional<std::vector<uint8_t>> credential_bytes) = 0;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_
