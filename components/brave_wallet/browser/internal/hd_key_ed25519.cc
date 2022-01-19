/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

#include <utility>

#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {
HDKeyEd25519::HDKeyEd25519(rust::Box<Ed25519DalekExtendedSecretKey> private_key)
    : private_key_(std::move(private_key)) {}
HDKeyEd25519::~HDKeyEd25519() {}

// static
std::unique_ptr<HDKeyEd25519> HDKeyEd25519::GenerateFromSeed(
    const std::vector<uint8_t>& seed) {
  auto master_private_key = generate_ed25519_extended_secrect_key_from_seed(
      rust::Slice<const uint8_t>{seed.data(), seed.size()});
  return std::make_unique<HDKeyEd25519>(std::move(master_private_key));
}

std::unique_ptr<HDKeyBase> HDKeyEd25519::DeriveChild(uint32_t index) {
  auto child_private_key = private_key_->derive_child(index);
  if (!child_private_key->is_valid())
    return nullptr;
  auto child_key = std::make_unique<HDKeyEd25519>(std::move(child_private_key));
  return std::unique_ptr<HDKeyBase>{child_key.release()};
}

std::unique_ptr<HDKeyBase> HDKeyEd25519::DeriveChildFromPath(
    const std::string& path) {
  auto child_private_key = private_key_->derive(path);
  if (!child_private_key->is_valid())
    return nullptr;
  auto child_key = std::make_unique<HDKeyEd25519>(std::move(child_private_key));
  return std::unique_ptr<HDKeyBase>{child_key.release()};
}

std::vector<uint8_t> HDKeyEd25519::Sign(const std::vector<uint8_t>& msg,
                                        int* recid) {
  return std::vector<uint8_t>();
}

bool HDKeyEd25519::Verify(const std::vector<uint8_t>& msg,
                          const std::vector<uint8_t>& sig) {
  return false;
}

std::string HDKeyEd25519::GetHexEncodedPrivateKey() const {
  return std::string();
}

std::string HDKeyEd25519::GetBase58EncodedPublicKey() const {
  auto public_key = private_key_->public_key_raw();
  return EncodeBase58(public_key);
}

std::string HDKeyEd25519::GetBase58EncodedKeypair() const {
  auto keypair = private_key_->keypair_raw();
  return EncodeBase58(keypair);
}

}  // namespace brave_wallet
