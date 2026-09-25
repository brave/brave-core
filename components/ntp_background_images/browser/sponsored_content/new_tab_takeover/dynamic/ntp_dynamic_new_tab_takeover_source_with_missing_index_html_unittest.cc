/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/dcheck_is_on.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/dynamic/test/ntp_dynamic_new_tab_takeover_source_test_base.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/test/ntp_sponsored_content_source_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

// Tests for a campaign where all creatives are invalid rich media (missing
// index.html).
class NTPDynamicNewTabTakeoverSourceMissingIndexHtmlTest
    : public test::NTPDynamicNewTabTakeoverSourceTestBase {
 protected:
  void SetUp() override {
    test::NTPDynamicNewTabTakeoverSourceTestBase::SetUp();
    SimulateDeprecatedOnSponsoredContentDidUpdate(
        test::GetSponsoredImagesComponentPath()
            .AppendASCII("new_tab_takeover")
            .AppendASCII("dynamic_with_missing_index_html"));
  }
};

// `DUMP_WILL_BE_NOTREACHED` aborts the process in non-official `DCHECK` builds.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
TEST_F(NTPDynamicNewTabTakeoverSourceMissingIndexHtmlTest,
       CampaignIsRemovedIfAllCreativesAreInvalid) {
  EXPECT_FALSE(background_images_service_->GetNewTabTakeover(
      /*supports_dynamic_new_tab_takeover=*/true));
}
#endif  // defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()

}  // namespace ntp_background_images
