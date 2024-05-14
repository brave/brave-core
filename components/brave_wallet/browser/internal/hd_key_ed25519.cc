/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

#include <utility>

#include "base/check.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {
namespace {
constexpr char kMasterNode[] = "m";
}

HDKeyEd25519::HDKeyEd25519(
    std::string path,
    rust::Box<Ed25519DalekExtendedSecretKeyResult> private_key)
    : path_(std::move(path)), private_key_(std::move(private_key)) {
  CHECK(private_key_->is_ok());
}
HDKeyEd25519::~HDKeyEd25519() = default;

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  auto master_private_key = generate_ed25519_extended_secret_key_from_seed(
      rust::Slice<const uint8_t>{seed.data(), seed.size()});
  if (!master_private_key->is_ok()) {
    VLOG(0) << std::string(master_private_key->error_message());
    return nullptr;
  }
  return std::make_unique<HDKeyEd25519>(kMasterNode,
                                        std::move(master_private_key));
}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromPrivateKey(
    base::span<const uint8_t> private_key) {
  auto master_private_key = generate_ed25519_extended_secret_key_from_bytes(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  if (!master_private_key->is_ok()) {
    VLOG(0) << std::string(master_private_key->error_message());
    return nullptr;
  }
  return std::make_unique<HDKeyEd25519>("", std::move(master_private_key));
}

std::string HDKeyEd25519::GetPath() const {
  return path_;
}

// index should be 0 to 2^31-1
// If anything failed, nullptr will be returned
// Normal derivation is not supported for ed25519.
// https://github.com/satoshilabs/slips/blob/master/slip-0010.md#private-parent-key--private-child-key
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::DeriveHardenedChild(
    uint32_t index) {
  auto child_private_key = private_key_->unwrap().derive_hardened_child(index);
  if (!child_private_key->is_ok()) {
    VLOG(0) << std::string(child_private_key->error_message());
    return nullptr;
  }
  auto child_path =
      base::StrCat({path_, "/", base::NumberToString(index), "'"});
  return std::make_unique<HDKeyEd25519>(std::move(child_path),
                                        std::move(child_private_key));
}

std::unique_ptr<HDKeyEd25519> HDKeyEd25519::DeriveChildFromPath(
    const std::string& path) {
  if (path_ != kMasterNode) {
    VLOG(0) << "must derive only from master key";
    return nullptr;
  }

  auto child_private_key = private_key_->unwrap().derive(path);
  if (!child_private_key->is_ok()) {
    VLOG(0) << std::string(child_private_key->error_message());
    return nullptr;
  }

  return std::make_unique<HDKeyEd25519>(path, std::move(child_private_key));
}

std::vector<uint8_t> HDKeyEd25519::Sign(base::span<const uint8_t> msg) {
  auto signature_result = private_key_->unwrap().sign(
      rust::Slice<const uint8_t>{msg.data(), msg.size()});
  if (!signature_result->is_ok()) {
    VLOG(0) << std::string(signature_result->error_message());
    return std::vector<uint8_t>();
  }
  auto signature_bytes = signature_result->unwrap().to_bytes();
  return std::vector<uint8_t>(signature_bytes.begin(), signature_bytes.end());
}

bool HDKeyEd25519::VerifyForTesting(base::span<const uint8_t> msg,
                                    base::span<const uint8_t> sig) {
  auto verification_result = private_key_->unwrap().verify(
      rust::Slice<const uint8_t>{msg.data(), msg.size()},
      rust::Slice<const uint8_t>{sig.data(), sig.size()});
  if (!verification_result->is_ok()) {
    VLOG(0) << std::string(verification_result->error_message());
    return false;
  }
  return true;
}

std::vector<uint8_t> HDKeyEd25519::GetPrivateKeyBytes() const {
  auto secret_key = private_key_->unwrap().secret_key_raw();
  return {secret_key.begin(), secret_key.end()};
}

std::vector<uint8_t> HDKeyEd25519::GetPublicKeyBytes() const {
  auto public_key = private_key_->unwrap().public_key_raw();
  return {public_key.begin(), public_key.end()};
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
