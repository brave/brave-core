/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_KEY_PAIR_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_KEY_PAIR_INFO_H_

#include <cstdint>
#include <vector>

namespace ads {
namespace security {

struct KeyPairInfo final {
  KeyPairInfo();
  KeyPairInfo(const KeyPairInfo& info);
  ~KeyPairInfo();

  bool operator==(const KeyPairInfo& rhs) const;
  bool operator!=(const KeyPairInfo& rhs) const;

  bool IsValid() const;

  std::vector<uint8_t> public_key;
  std::vector<uint8_t> secret_key;
};

}  // namespace security
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_KEY_PAIR_INFO_H_
