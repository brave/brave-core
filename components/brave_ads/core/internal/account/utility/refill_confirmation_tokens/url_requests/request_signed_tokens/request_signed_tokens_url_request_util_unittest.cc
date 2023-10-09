/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRequestSignedTokensUrlRequestUtilTest : public UnitTestBase {};

TEST_F(BraveAdsRequestSignedTokensUrlRequestUtilTest, ParseNonce) {
  // Act & Assert
  EXPECT_EQ(kGetSignedTokensNonce,
            ParseNonce(base::test::ParseJsonDict(
                test::BuildRequestSignedTokensUrlResponseBody())));
}

TEST_F(BraveAdsRequestSignedTokensUrlRequestUtilTest, DoNotParseMissingNonce) {
  // Act & Assert
  EXPECT_FALSE(ParseNonce(base::test::ParseJsonDict("{}")));
}

}  // namespace brave_ads
