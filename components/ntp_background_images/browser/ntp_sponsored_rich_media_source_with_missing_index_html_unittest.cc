/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/dcheck_is_on.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_rich_media_source_test_base.h"
#include "brave/components/ntp_background_images/browser/test/ntp_sponsored_source_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

// Tests for a campaign where all creatives are invalid rich media (missing
// index.html).
class NTPSponsoredRichMediaSourceMissingIndexHtmlTest
    : public test::NTPSponsoredRichMediaSourceTestBase {
 protected:
  void SetUp() override {
    test::NTPSponsoredRichMediaSourceTestBase::SetUp();
    SimulateOnSponsoredImagesDataDidUpdate(
        test::GetSponsoredImagesComponentPath().AppendASCII(
            "rich_media_with_missing_index_html"));
  }
};

// `DUMP_WILL_BE_NOTREACHED` aborts the process in non-official `DCHECK` builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST_F(NTPSponsoredRichMediaSourceMissingIndexHtmlTest,
       CampaignIsRemovedIfAllCreativesAreInvalid) {
  EXPECT_FALSE(background_images_service_->GetSponsoredImagesData(
      /*supports_rich_media=*/true));
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

}  // namespace ntp_background_images
