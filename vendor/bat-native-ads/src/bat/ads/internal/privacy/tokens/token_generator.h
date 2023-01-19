/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_H_

#include <vector>

#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"

namespace ads::privacy {

namespace cbr {
class Token;
}  // namespace cbr

class TokenGenerator : public TokenGeneratorInterface {
 public:
  std::vector<cbr::Token> Generate(int count) const override;
};

}  // namespace ads::privacy

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_H_
