/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_INTERFACE_H_

#include <vector>

namespace brave_ads::privacy {

namespace cbr {
class Token;
}  // namespace cbr

class TokenGeneratorInterface {
 public:
  virtual ~TokenGeneratorInterface() = default;

  virtual std::vector<cbr::Token> Generate(size_t count) const = 0;
};

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_INTERFACE_H_
