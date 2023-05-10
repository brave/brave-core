/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"

namespace brave_ads::privacy {

namespace cbr {
class Token;
}  // namespace cbr

class TokenGenerator : public TokenGeneratorInterface {
 public:
  std::vector<cbr::Token> Generate(size_t count) const override;
};

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_H_
