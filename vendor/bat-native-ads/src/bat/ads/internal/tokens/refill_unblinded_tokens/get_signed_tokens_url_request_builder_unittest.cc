/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsGetSignedTokensUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  WalletInfo wallet;
  wallet.id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.secret_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  const std::string nonce = "716c3381-66e6-46e4-962f-15d01455b5b9";

  GetSignedTokensUrlRequestBuilder url_request_builder(wallet, nonce);

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url =
      R"(https://ads-serve.brave.software/v1/confirmation/token/d4ed0af0-bfa9-464b-abd7-67b29d891b8b?nonce=716c3381-66e6-46e4-962f-15d01455b5b9)";
  expected_url_request->method = mojom::UrlRequestMethod::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
