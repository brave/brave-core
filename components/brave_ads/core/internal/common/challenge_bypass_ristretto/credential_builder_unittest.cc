/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/credential_builder.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/challenge_bypass_ristretto_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsChallengeBypassRistrettoTest, BuildCredential) {
  // Act
  const std::optional<base::Value::Dict> credential = cbr::MaybeBuildCredential(
      cbr::UnblindedToken(cbr::test::kUnblindedTokenBase64),
      /*payload=*/"definition: the weight of a payload");
  ASSERT_TRUE(credential);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "signature": "PblFP7WI3ZC3aAX73A9UvBdqnvl87Wx8nnz9DIbhNjxbFamMZGbwn5Hi+FXsSXg2GZ671rCwQ6Xpwe61JjeW9Q==",
                      "t": "IXDCnZnVEJ0orkbZfr2ut2NQPQ0ofdervKBmQ2hyjcClGCjA3/ExbBumO0ua5cxwo//nN0uKQ60kknru8hRXxw=="
                    })"),
            *credential);
}

}  // namespace brave_ads
