/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_INTERFACE_H_

#include "bat/ads/internal/privacy/tokens/token_aliases.h"

namespace ads {
namespace privacy {
namespace cbr {

class TokenGeneratorInterface {
 public:
  virtual ~TokenGeneratorInterface() = default;

  virtual TokenList Generate(const int count) const = 0;
};

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_INTERFACE_H_
