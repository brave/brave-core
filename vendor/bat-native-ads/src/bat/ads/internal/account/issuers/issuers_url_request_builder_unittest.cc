/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers_url_request_builder.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIssuersUrlRequestBuilderTest : public UnitTestBase {
 protected:
  BatAdsIssuersUrlRequestBuilderTest() = default;

  ~BatAdsIssuersUrlRequestBuilderTest() override = default;
};

TEST_F(BatAdsIssuersUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  IssuersUrlRequestBuilder url_request_builder;

  // Act
  mojom::UrlRequestPtr url_request = url_request_builder.Build();

  // Assert
  mojom::UrlRequestPtr expected_url_request = mojom::UrlRequest::New();
  expected_url_request->url =
      GURL(R"(https://static.ads.bravesoftware.com/v1/issuers/)");
  expected_url_request->method = mojom::UrlRequestMethod::kGet;

  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace ads
