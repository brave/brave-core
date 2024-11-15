/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_INTERFACE_H_

#include <cstddef>

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"

namespace brave_ads {

class TokenGeneratorInterface {
 public:
  virtual ~TokenGeneratorInterface() = default;

  virtual cbr::TokenList Generate(size_t count) const = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_GENERATOR_INTERFACE_H_
