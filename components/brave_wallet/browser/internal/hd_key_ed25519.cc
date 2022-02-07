/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {
HDKeyEd25519::HDKeyEd25519(
    rust::Box<Ed25519DalekExtendedSecretKeyResult> private_key)
    : private_key_(std::move(private_key)) {
  CHECK(private_key_->is_ok());
}
HDKeyEd25519::~HDKeyEd25519() {}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromSeed(
    const std::vector<uint8_t>& seed) {
  auto master_private_key = generate_ed25519_extended_secrect_key_from_seed(
      rust::Slice<const uint8_t>{seed.data(), seed.size()});
  if (!master_private_key->is_ok()) {
    VLOG(0) << std::string(master_private_key->error_message());
    return nullptr;
  }
  return std::make_unique<HDKeyEd25519>(std::move(master_private_key));
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromPrivateKey(
    const std::vector<uint8_t>& private_key) {
  auto master_private_key = generate_ed25519_extended_secrect_key_from_bytes(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  if (!master_private_key->is_ok()) {
    VLOG(0) << std::string(master_private_key->error_message());
    return nullptr;
  }
  return std::make_unique<HDKeyEd25519>(std::move(master_private_key));
}

std::unique_ptr<HDKeyBase> HDKeyEd25519::DeriveChild(uint32_t index) {
  auto child_private_key = private_key_->unwrap().derive_child(index);
  if (!child_private_key->is_ok()) {
    VLOG(0) << std::string(child_private_key->error_message());
    return nullptr;
  }
  auto child_key = std::make_unique<HDKeyEd25519>(std::move(child_private_key));
  return std::unique_ptr<HDKeyBase>{child_key.release()};
}

std::unique_ptr<HDKeyBase> HDKeyEd25519::DeriveChildFromPath(
    const std::string& path) {
  auto child_private_key = private_key_->unwrap().derive(path);
  if (!child_private_key->is_ok()) {
    VLOG(0) << std::string(child_private_key->error_message());
    return nullptr;
  }
  auto child_key = std::make_unique<HDKeyEd25519>(std::move(child_private_key));
  return std::unique_ptr<HDKeyBase>{child_key.release()};
}

std::vector<uint8_t> HDKeyEd25519::Sign(const std::vector<uint8_t>& msg,
                                        int* recid) {
  auto signature_result = private_key_->unwrap().sign(
      rust::Slice<const uint8_t>{msg.data(), msg.size()});
  if (!signature_result->is_ok()) {
    VLOG(0) << std::string(signature_result->error_message());
    return std::vector<uint8_t>();
  }
  auto signature_bytes = signature_result->unwrap().to_bytes();
  return std::vector<uint8_t>(signature_bytes.begin(), signature_bytes.end());
}

bool HDKeyEd25519::Verify(const std::vector<uint8_t>& msg,
                          const std::vector<uint8_t>& sig) {
  if (sig.size() != 64)
    return false;
  std::array<uint8_t, 64> signature;
  std::copy_n(sig.begin(), 64, signature.begin());
  auto verification_result = private_key_->unwrap().verify(
      rust::Slice<const uint8_t>{msg.data(), msg.size()}, std::move(signature));
  if (!verification_result->is_ok()) {
    VLOG(0) << std::string(verification_result->error_message());
    return false;
  }
  return true;
}

std::string HDKeyEd25519::GetEncodedPrivateKey() const {
  return GetBase58EncodedKeypair();
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  auto public_key = private_key_->unwrap().public_key_raw();
  return EncodeBase58(public_key);
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  auto keypair = private_key_->unwrap().keypair_raw();
  return EncodeBase58(keypair);
}

}  // namespace brave_wallet
