/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/dcheck_is_on.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_rich_media_source_test_base.h"
#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_source_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

// Tests for a campaign with both image and rich media creatives where the rich
// media creative is missing index.html.
class NTPSponsoredImageAndRichMediaWithMissingIndexHtmlTest
    : public test::NTPSponsoredRichMediaSourceTestBase {
 protected:
  void SetUp() override {
    test::NTPSponsoredRichMediaSourceTestBase::SetUp();
    SimulateOnSponsoredImagesDataDidUpdate(
        test::GetSponsoredImagesComponentPath().AppendASCII(
            "image_and_rich_media_with_missing_index_html"));
  }
};

// `DUMP_WILL_BE_NOTREACHED()` aborts the process in non-official DCHECK builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST_F(NTPSponsoredImageAndRichMediaWithMissingIndexHtmlTest,
       InvalidRichMediaCreativeIsRemovedButImageCreativeIsKept) {
  const auto* const data = background_images_service_->GetSponsoredImagesData(
      /*supports_rich_media=*/true);
  ASSERT_NE(nullptr, data);
  ASSERT_THAT(data->campaigns, ::testing::SizeIs(1));
  ASSERT_THAT(data->campaigns[0].creatives, ::testing::SizeIs(1));
  EXPECT_EQ(WallpaperType::kImage,
            data->campaigns[0].creatives[0].wallpaper_type);
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

}  // namespace ntp_background_images
