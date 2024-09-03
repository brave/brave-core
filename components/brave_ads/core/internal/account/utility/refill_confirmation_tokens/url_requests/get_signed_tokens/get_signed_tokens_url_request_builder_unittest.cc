/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder.h"

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsGetSignedTokensUrlRequestBuilderTest : public test::TestBase {};

TEST_F(BraveAdsGetSignedTokensUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  GetSignedTokensUrlRequestBuilder url_request_builder(
      test::Wallet(), /*nonce=*/"716c3381-66e6-46e4-962f-15d01455b5b9");

  // Act
  const mojom::UrlRequestInfoPtr mojom_url_request =
      url_request_builder.Build();

  // Assert
  const mojom::UrlRequestInfoPtr expected_mojom_url_request =
      mojom::UrlRequestInfo::New();
  expected_mojom_url_request->url = GURL(
      R"(https://mywallet.ads.bravesoftware.com/v3/confirmation/token/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=716c3381-66e6-46e4-962f-15d01455b5b9)");
  expected_mojom_url_request->method = mojom::UrlRequestMethodType::kGet;
  EXPECT_EQ(expected_mojom_url_request, mojom_url_request);
}

}  // namespace brave_ads
