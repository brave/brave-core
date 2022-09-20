/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WALLET_CONNECT_ENCRYPTOR_H_
#define BRAVE_COMPONENTS_WALLET_CONNECT_ENCRYPTOR_H_

#include <array>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/wallet_connect/wallet_connect.h"

namespace wallet_connect {

// The encryptor will perform AES-256-CBC encryption/decryption alogn with
// HMAC-SHA256
class Encryptor {
 public:
  explicit Encryptor(const std::array<uint8_t, 32>& key);
  ~Encryptor();

  base::expected<types::EncryptionPayload, std::string> Encrypt(
      const std::vector<uint8_t>& data);

  base::expected<std::vector<uint8_t>, std::string> Decrypt(
      const types::EncryptionPayload& payload);

 private:
  std::array<uint8_t, 32> key_;
};

}  // namespace wallet_connect

#endif  // BRAVE_COMPONENTS_WALLET_CONNECT_ENCRYPTOR_H_
