/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_CRYPTO_KEY_PAIR_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_CRYPTO_KEY_PAIR_INFO_H_

#include <cstdint>
#include <vector>

namespace ads::crypto {

struct KeyPairInfo final {
  KeyPairInfo();

  KeyPairInfo(const KeyPairInfo& other);
  KeyPairInfo& operator=(const KeyPairInfo& other);

  KeyPairInfo(KeyPairInfo&& other) noexcept;
  KeyPairInfo& operator=(KeyPairInfo&& other) noexcept;

  ~KeyPairInfo();

  bool operator==(const KeyPairInfo& other) const;
  bool operator!=(const KeyPairInfo& other) const;

  bool IsValid() const;

  std::vector<uint8_t> public_key;
  std::vector<uint8_t> secret_key;
};

}  // namespace ads::crypto

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_CRYPTO_KEY_PAIR_INFO_H_
