/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/get_signed_tokens_url_request_builder.h"

#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/flags/flag_manager.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsGetSignedTokensUrlRequestBuilderTest : public UnitTestBase {};

TEST_F(BatAdsGetSignedTokensUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  const std::string nonce = "716c3381-66e6-46e4-962f-15d01455b5b9";

  GetSignedTokensUrlRequestBuilder url_request_builder(GetWalletForTesting(),
                                                       nonce);

  // Act
  mojom::UrlRequestInfoPtr const url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestInfoPtr expected_url_request = mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      "https://mywallet.ads.bravesoftware.com/v3/confirmation/token/"
      "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7?nonce=716c3381-66e6-46e4-962f-"
      "15d01455b5b9");
  expected_url_request->method = mojom::UrlRequestMethodType::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
