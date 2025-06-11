/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/background_credential_helper.h"

#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/span_rust.h"
#include "base/logging.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/web_discovery/browser/anonymous_credentials/lib.rs.h"
#include "brave/components/web_discovery/browser/rsa.h"
#include "crypto/sha2.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace web_discovery {

StartJoinInitialization::StartJoinInitialization(std::string request_b64,
                                                 std::vector<uint8_t> gsk,
                                                 std::string signature)
    : request_b64(std::move(request_b64)),
      gsk(std::move(gsk)),
      signature(std::move(signature)) {}

StartJoinInitialization::~StartJoinInitialization() = default;

StartJoinInitialization::StartJoinInitialization(
    const StartJoinInitialization&) = default;
StartJoinInitialization& StartJoinInitialization::operator=(
    const StartJoinInitialization&) = default;

class BackgroundCredentialHelperImpl : public BackgroundCredentialHelper {
 public:
  BackgroundCredentialHelperImpl()
      : anonymous_credentials_manager_(new_anonymous_credentials_manager()) {}
  ~BackgroundCredentialHelperImpl() override = default;

  BackgroundCredentialHelperImpl(const BackgroundCredentialHelperImpl&) =
      delete;
  BackgroundCredentialHelperImpl& operator=(
      const BackgroundCredentialHelperImpl&) = delete;

  void UseFixedSeedForTesting() override {
    anonymous_credentials_manager_ =
        new_anonymous_credentials_with_fixed_seed();
  }

  std::unique_ptr<crypto::RSAPrivateKey> GenerateAndSetRSAKey() override {
    rsa_private_key_ = web_discovery::GenerateRSAKey();
    if (!rsa_private_key_) {
      return nullptr;
    }
    return rsa_private_key_->Copy();
  }

  void SetRSAKey(
      std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key) override {
    rsa_private_key_ = std::move(rsa_private_key);
  }

  std::optional<StartJoinInitialization> GenerateJoinRequest(
      std::string pre_challenge) override {
    base::AssertLongCPUWorkAllowed();
    CHECK(rsa_private_key_);
    auto challenge = crypto::SHA256Hash(base::as_byte_span(pre_challenge));

    auto join_result = anonymous_credentials_manager_->start_join(
        base::SpanToRustSlice(challenge));

    auto signature = RSASign(rsa_private_key_.get(), join_result.join_request);

    if (!signature) {
      VLOG(1) << "RSA signature failed";
      return std::nullopt;
    }

    return StartJoinInitialization(
        base::Base64Encode(join_result.join_request),
        std::vector<uint8_t>(join_result.gsk.begin(), join_result.gsk.end()),
        *signature);
  }

  std::optional<std::string> FinishJoin(
      std::string date,
      std::vector<uint8_t> group_pub_key,
      std::vector<uint8_t> gsk,
      std::vector<uint8_t> join_resp_bytes) override {
    base::AssertLongCPUWorkAllowed();
    auto pub_key_result =
        load_group_public_key(base::SpanToRustSlice(group_pub_key));
    auto gsk_result = load_credential_big(base::SpanToRustSlice(gsk));
    auto join_resp_result =
        load_join_response(base::SpanToRustSlice(join_resp_bytes));
    if (!pub_key_result->is_ok() || !gsk_result->is_ok() ||
        !join_resp_result->is_ok()) {
      VLOG(1)
          << "Failed to finish credential join due to deserialization error "
             "with group pub key, gsk, or join response: "
          << pub_key_result->error_message().c_str()
          << gsk_result->error_message().c_str()
          << join_resp_result->error_message().c_str();
      return std::nullopt;
    }
    auto finish_res = anonymous_credentials_manager_->finish_join(
        *pub_key_result->unwrap(), *gsk_result->unwrap(),
        join_resp_result->unwrap());
    if (!finish_res->is_ok()) {
      VLOG(1) << "Failed to finish credential join for " << date << ": "
              << finish_res->error_message().c_str();
      return std::nullopt;
    }
    return base::Base64Encode(finish_res->unwrap());
  }

  std::optional<std::vector<uint8_t>> PerformSign(
      std::vector<uint8_t> msg,
      std::vector<uint8_t> basename,
      std::optional<std::vector<uint8_t>> gsk_bytes,
      std::optional<std::vector<uint8_t>> credential_bytes) override {
    base::AssertLongCPUWorkAllowed();
    if (gsk_bytes && credential_bytes) {
      auto gsk_result = load_credential_big(base::SpanToRustSlice(*gsk_bytes));
      auto credential_result =
          load_user_credentials(base::SpanToRustSlice(*credential_bytes));
      if (!gsk_result->is_ok() || !credential_result->is_ok()) {
        VLOG(1) << "Failed to sign due to deserialization error with gsk, or "
                   "user credential: "
                << gsk_result->error_message().c_str()
                << credential_result->error_message().c_str();
        return std::nullopt;
      }
      anonymous_credentials_manager_->set_gsk_and_credentials(
          gsk_result->unwrap(), credential_result->unwrap());
    }
    auto sig_res = anonymous_credentials_manager_->sign(
        base::SpanToRustSlice(msg), base::SpanToRustSlice(basename));
    if (!sig_res->is_ok()) {
      VLOG(1) << "Failed to sign: " << sig_res->error_message().c_str();
      return std::nullopt;
    }
    auto sig_data = sig_res->unwrap();
    return std::vector<uint8_t>(sig_data.begin(), sig_data.end());
  }

 private:
  rust::Box<AnonymousCredentialsManager> anonymous_credentials_manager_;
  std::unique_ptr<crypto::RSAPrivateKey> rsa_private_key_;
};

std::unique_ptr<BackgroundCredentialHelper>
BackgroundCredentialHelper::Create() {
  return std::make_unique<BackgroundCredentialHelperImpl>();
}

}  // namespace web_discovery
