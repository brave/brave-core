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

#include "brave/components/web_discovery/browser/anonymous_credentials/rs/cxx/src/lib.rs.h"
#include "brave/components/web_discovery/browser/rsa.h"
#include "crypto/rsa_private_key.h"

namespace web_discovery {

struct GenerateJoinRequestResult {
  anonymous_credentials::StartJoinResult start_join_result;
  std::string signature;
};

class BackgroundCredentialHelper {
 public:
  BackgroundCredentialHelper();
  ~BackgroundCredentialHelper();

  BackgroundCredentialHelper(const BackgroundCredentialHelper&) = delete;
  BackgroundCredentialHelper& operator=(const BackgroundCredentialHelper&) =
      delete;

  void UseFixedSeedForTesting();

  std::unique_ptr<RSAKeyInfo> GenerateRSAKey();
  void SetRSAKey(std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key);
  std::optional<GenerateJoinRequestResult> GenerateJoinRequest(
      std::string pre_challenge);
  std::optional<std::string> FinishJoin(
      std::string date,
      std::vector<const uint8_t> group_pub_key,
      std::vector<const uint8_t> gsk,
      std::vector<const uint8_t> join_resp_bytes);
  std::optional<std::vector<const uint8_t>> PerformSign(
      std::vector<const uint8_t> msg,
      std::vector<const uint8_t> basename,
      std::optional<std::vector<uint8_t>> gsk_bytes,
      std::optional<std::vector<uint8_t>> credential_bytes);

 private:
  rust::Box<anonymous_credentials::CredentialManager>
      anonymous_credential_manager_;
  std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_H_
