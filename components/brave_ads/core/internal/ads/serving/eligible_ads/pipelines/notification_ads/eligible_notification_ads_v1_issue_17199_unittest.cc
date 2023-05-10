/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/pipelines/notification_ads/eligible_notification_ads_v1.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/user_model_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEligibleNotificationAdsV1Issue17199Test : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    CopyFileFromTestPathToTempPath("database_issue_17199.sqlite",
                                   kDatabaseFilename);

    CopyFileFromTestPathToTempPath("client_issue_17199.json",
                                   kClientStateFilename);
  }
};

TEST_F(BraveAdsEligibleNotificationAdsV1Issue17199Test, GetEligibleAds) {
  // Arrange
  AdvanceClockTo(TimeFromString("4 July 2021", /*is_local*/ false));

  // Act
  SubdivisionTargeting subdivision_targeting;
  AntiTargetingResource anti_targeting_resource;
  EligibleNotificationAdsV1 eligible_ads(subdivision_targeting,
                                         anti_targeting_resource);

  eligible_ads.GetForUserModel(
      BuildUserModel({/*interest_segments*/ "technology & computing-computing"},
                     /*latent_interest_segments*/ {},
                     /*purchase_intent_segments*/ {},
                     /*text_embedding_html_events*/ {}),
      base::BindOnce([](const bool had_opportunity,
                        const CreativeNotificationAdList& creative_ads) {
        EXPECT_TRUE(had_opportunity);
        EXPECT_FALSE(creative_ads.empty());
      }));

  // Assert
}

}  // namespace brave_ads
