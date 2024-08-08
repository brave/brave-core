/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/background_credential_helper.h"

#include <utility>

#include "base/base64.h"
#include "base/containers/span_rust.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "crypto/sha2.h"

namespace web_discovery {

BackgroundCredentialHelper::BackgroundCredentialHelper()
    : anonymous_credential_manager_(
          anonymous_credentials::new_credential_manager()) {}

BackgroundCredentialHelper::~BackgroundCredentialHelper() = default;

void BackgroundCredentialHelper::UseFixedSeedForTesting() {
  anonymous_credential_manager_ =
      anonymous_credentials::new_credential_manager_with_fixed_seed();
}

std::unique_ptr<RSAKeyInfo> BackgroundCredentialHelper::GenerateRSAKey() {
  auto key_pair = GenerateRSAKeyPair();
  if (!key_pair) {
    return nullptr;
  }
  rsa_private_key_ = std::move(key_pair->key_pair);
  return key_pair;
}

void BackgroundCredentialHelper::SetRSAKey(
    std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key) {
  rsa_private_key_ = std::move(rsa_private_key);
}

std::optional<GenerateJoinRequestResult>
BackgroundCredentialHelper::GenerateJoinRequest(std::string pre_challenge) {
  base::AssertLongCPUWorkAllowed();
  CHECK(rsa_private_key_);
  auto challenge = crypto::SHA256Hash(base::as_byte_span(pre_challenge));

  auto join_request = anonymous_credential_manager_->start_join(
      base::SpanToRustSlice(challenge));

  auto signature = RSASign(rsa_private_key_.get(), join_request.join_request);

  if (!signature) {
    VLOG(1) << "RSA signature failed";
    return std::nullopt;
  }

  return GenerateJoinRequestResult{.start_join_result = join_request,
                                   .signature = *signature};
}

std::optional<std::string> BackgroundCredentialHelper::FinishJoin(
    std::string date,
    std::vector<const uint8_t> group_pub_key,
    std::vector<const uint8_t> gsk,
    std::vector<const uint8_t> join_resp_bytes) {
  base::AssertLongCPUWorkAllowed();
  auto pub_key_result = anonymous_credentials::load_group_public_key(
      base::SpanToRustSlice(group_pub_key));
  auto gsk_result =
      anonymous_credentials::load_credential_big(base::SpanToRustSlice(gsk));
  auto join_resp_result = anonymous_credentials::load_join_response(
      base::SpanToRustSlice(join_resp_bytes));
  if (!pub_key_result.error_message.empty() ||
      !gsk_result.error_message.empty() ||
      !join_resp_result.error_message.empty()) {
    VLOG(1) << "Failed to finish credential join due to deserialization error "
               "with group pub key, gsk, or join response: "
            << pub_key_result.error_message.c_str()
            << gsk_result.error_message.c_str()
            << join_resp_result.error_message.c_str();
    return std::nullopt;
  }
  auto finish_res = anonymous_credential_manager_->finish_join(
      *pub_key_result.value, *gsk_result.value,
      std::move(join_resp_result.value));
  if (!finish_res.error_message.empty()) {
    VLOG(1) << "Failed to finish credential join for " << date << ": "
            << finish_res.error_message.c_str();
    return std::nullopt;
  }
  return base::Base64Encode(finish_res.data);
}

std::optional<std::vector<const uint8_t>>
BackgroundCredentialHelper::PerformSign(
    std::vector<const uint8_t> msg,
    std::vector<const uint8_t> basename,
    std::optional<std::vector<uint8_t>> gsk_bytes,
    std::optional<std::vector<uint8_t>> credential_bytes) {
  base::AssertLongCPUWorkAllowed();
  if (gsk_bytes && credential_bytes) {
    auto gsk_result = anonymous_credentials::load_credential_big(
        base::SpanToRustSlice(*gsk_bytes));
    auto credential_result = anonymous_credentials::load_user_credentials(
        base::SpanToRustSlice(*credential_bytes));
    if (!gsk_result.error_message.empty() ||
        !credential_result.error_message.empty()) {
      VLOG(1) << "Failed to sign due to deserialization error with gsk, or "
                 "user credential: "
              << gsk_result.error_message.c_str()
              << credential_result.error_message.c_str();
      return std::nullopt;
    }
    anonymous_credential_manager_->set_gsk_and_credentials(
        std::move(gsk_result.value), std::move(credential_result.value));
  }
  auto sig_res = anonymous_credential_manager_->sign(
      base::SpanToRustSlice(msg), base::SpanToRustSlice(basename));
  if (!sig_res.error_message.empty()) {
    VLOG(1) << "Failed to sign: " << sig_res.error_message.c_str();
    return std::nullopt;
  }
  return std::vector<const uint8_t>(sig_res.data.begin(), sig_res.data.end());
}

}  // namespace web_discovery
