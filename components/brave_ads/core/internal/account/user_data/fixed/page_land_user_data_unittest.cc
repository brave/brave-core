/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/page_land_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPageLandUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsPageLandUserDataTest,
       BuildPageLandUserDataForHttpResponseStatusErrorPage) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "httpResponseStatus": "errorPage"
                    })"),
            BuildPageLandUserData(
                TabInfo{/*id=*/1,
                        /*is_visible=*/true,
                        /*redirect_chain=*/{GURL("https://brave.com")},
                        /*is_error_page=*/true,
                        /*is_playing_media=*/false}));
}

TEST_F(BraveAdsPageLandUserDataTest,
       DoNotBuildPageLandUserDataForHttpResponseStatusNonErrorPage) {
  // Act & Assert
  EXPECT_TRUE(BuildPageLandUserData(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false})
                  .empty());
}

}  // namespace brave_ads
