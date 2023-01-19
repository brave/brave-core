/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/opted_in_credential_json_writer.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kExpectedJson[] =
    R"({"signature":"XsaQ/XqKiWfeTCjFDhkyldsx0086qu6tjgJDCKo+f7kA0eA+mdf3Ae+BjPcDDQ8JfVbVQkI5ub394qdTmE2bRw==","t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="})";

}  // namespace

class BatAdsOptedInCredentialJsonWriterTest : public UnitTestBase {};

TEST_F(BatAdsOptedInCredentialJsonWriterTest, WriteJson) {
  // Arrange
  const privacy::UnblindedTokenList unblinded_tokens =
      privacy::GetUnblindedTokens(1);
  ASSERT_TRUE(!unblinded_tokens.empty());
  const privacy::UnblindedTokenInfo& unblinded_token = unblinded_tokens.front();

  // Act
  const absl::optional<std::string> json = json::writer::WriteOptedInCredential(
      unblinded_token, /*payload*/ "definition: the weight of a payload");
  ASSERT_TRUE(json);

  // Assert
  EXPECT_EQ(kExpectedJson, *json);
}

}  // namespace ads
