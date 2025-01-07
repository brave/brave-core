/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_IMPL_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_IMPL_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/web_discovery/browser/anonymous_credentials/lib.rs.h"
#include "brave/components/web_discovery/browser/background_credential_helper.h"
#include "brave/components/web_discovery/browser/rsa.h"
#include "crypto/rsa_private_key.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace web_discovery {

class BackgroundCredentialHelperImpl : public BackgroundCredentialHelper {
 public:
  BackgroundCredentialHelperImpl();
  ~BackgroundCredentialHelperImpl() override;

  BackgroundCredentialHelperImpl(const BackgroundCredentialHelperImpl&) =
      delete;
  BackgroundCredentialHelperImpl& operator=(
      const BackgroundCredentialHelperImpl&) = delete;

  void UseFixedSeedForTesting() override;

  std::unique_ptr<EncodedRSAKeyPair> GenerateRSAKey() override;
  void SetRSAKey(
      std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key) override;
  std::optional<GenerateJoinRequestResult> GenerateJoinRequest(
      std::string pre_challenge) override;
  std::optional<std::string> FinishJoin(
      std::string date,
      std::vector<uint8_t> group_pub_key,
      std::vector<uint8_t> gsk,
      std::vector<uint8_t> join_resp_bytes) override;
  std::optional<std::vector<uint8_t>> PerformSign(
      std::vector<uint8_t> msg,
      std::vector<uint8_t> basename,
      std::optional<std::vector<uint8_t>> gsk_bytes,
      std::optional<std::vector<uint8_t>> credential_bytes) override;

 private:
  rust::Box<AnonymousCredentialsManager> anonymous_credentials_manager_;
  std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key_;
};

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_BACKGROUND_CREDENTIAL_HELPER_IMPL_H_
