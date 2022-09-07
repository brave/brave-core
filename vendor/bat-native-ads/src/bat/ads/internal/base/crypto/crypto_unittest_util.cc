/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/crypto/crypto_unittest_util.h"

#include "tweetnacl.h"  // NOLINT

namespace ads::security {

std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& ciphertext,
                             const std::vector<uint8_t>& nonce,
                             const std::vector<uint8_t>& ephemeral_public_key,
                             const std::vector<uint8_t>& secret_key) {
  std::vector<uint8_t> padded_plaintext(ciphertext.size());
  crypto_box_open(&padded_plaintext.front(), &ciphertext.front(),
                  ciphertext.size(), &nonce.front(),
                  &ephemeral_public_key.front(), &secret_key.front());

  std::vector<uint8_t> plaintext(
      padded_plaintext.cbegin() + crypto_box_ZEROBYTES,
      padded_plaintext.cend());

  return plaintext;
}

}  // namespace ads::security
