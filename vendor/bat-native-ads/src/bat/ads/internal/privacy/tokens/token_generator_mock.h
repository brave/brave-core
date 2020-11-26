/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_MOCK_H_
#define BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_MOCK_H_

#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"

namespace ads {
namespace privacy {

class TokenGeneratorMock : public TokenGenerator {
 public:
  TokenGeneratorMock();

  ~TokenGeneratorMock() override;

  MOCK_METHOD(std::vector<Token>, Generate, (
      const int count), (const));
};

}  // namespace privacy
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_PRIVACY_TOKENS_TOKEN_GENERATOR_MOCK_H_
