/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/flags/flag_manager_util.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsGetSignedTokensUrlRequestBuilderTest : public UnitTestBase {
 protected:
  BatAdsGetSignedTokensUrlRequestBuilderTest() = default;

  ~BatAdsGetSignedTokensUrlRequestBuilderTest() override = default;
};

TEST_F(BatAdsGetSignedTokensUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  SetEnvironmentTypeForTesting(EnvironmentType::kStaging);

  WalletInfo wallet;
  wallet.id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.secret_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  const std::string nonce = "716c3381-66e6-46e4-962f-15d01455b5b9";

  GetSignedTokensUrlRequestBuilder url_request_builder(wallet, nonce);

  // Act
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://mywallet.ads.bravesoftware.com/v2/confirmation/token/d4ed0af0-bfa9-464b-abd7-67b29d891b8b?nonce=716c3381-66e6-46e4-962f-15d01455b5b9)");
  expected_url_request->method = mojom::UrlRequestMethodType::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
