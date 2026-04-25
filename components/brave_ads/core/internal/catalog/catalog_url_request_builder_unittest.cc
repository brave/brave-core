/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCatalogUrlRequestBuilderTest : public test::TestBase {};

TEST_F(BraveAdsCatalogUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  CatalogUrlRequestBuilder url_request_builder;

  // Act
  const mojom::UrlRequestInfoPtr mojom_url_request =
      url_request_builder.Build();

  // Assert
  const mojom::UrlRequestInfoPtr expected_mojom_url_request =
      mojom::UrlRequestInfo::New();
  expected_mojom_url_request->url =
      GURL("https://static.ads.bravesoftware.com/v9/catalog");
  expected_mojom_url_request->method = mojom::UrlRequestMethodType::kGet;
  EXPECT_EQ(expected_mojom_url_request, mojom_url_request);
}

}  // namespace brave_ads
