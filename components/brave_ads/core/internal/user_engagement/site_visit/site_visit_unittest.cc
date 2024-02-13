/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"

#include <memory>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_observer_mock.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSiteVisitTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    site_visit_ = std::make_unique<SiteVisit>();
    site_visit_->AddObserver(&observer_mock_);
  }

  void TearDown() override {
    site_visit_->RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  std::unique_ptr<SiteVisit> site_visit_;
  ::testing::StrictMock<SiteVisitObserverMock> observer_mock_;

  ::testing::InSequence s_;
};

TEST_F(BraveAdsSiteVisitTest, DoNotLandOnPageIfInvalidAd) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad;
  site_visit_->SetLastClickedAd(ad);

  // Act & Assert
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest,
       DoNotLandOnPageIfTheUrlDoesNotMatchTheLastClickedAd) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);
  site_visit_->MaybeLandOnPage(/*tab_id=*/1,
                               {GURL("https://basicattentiontoken.org")});

  // Act & Assert
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest, DoNotLandOnPageIfTheSameAdIsAlreadyLanding) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);
  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, Now() + kPageLandAfter.Get()));
  EXPECT_CALL(observer_mock_, OnDidLandOnPage(ad));
  site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});

  // Act & Assert
  site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest, LandOnPageIfAnotherAdIsAlreadyLanded) {
  // Arrange
  {
    NotifyTabDidChange(
        /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
        /*is_visible=*/true);

    const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                      /*should_use_random_uuids=*/true);
    site_visit_->SetLastClickedAd(ad_1);
    EXPECT_CALL(observer_mock_,
                OnMaybeLandOnPage(ad_1, Now() + kPageLandAfter.Get()));
    site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});
  }

  {
    NotifyTabDidChange(
        /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
        /*is_visible=*/true);

    const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                      /*should_use_random_uuids=*/true);
    site_visit_->SetLastClickedAd(ad_2);
    EXPECT_CALL(observer_mock_,
                OnMaybeLandOnPage(ad_2, Now() + kPageLandAfter.Get()));
    EXPECT_CALL(observer_mock_, OnDidLandOnPage(ad_2));
    site_visit_->MaybeLandOnPage(/*tab_id=*/2, {GURL("https://brave.com")});
  }

  // Act & Assert
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest,
       LandOnPageIfTheTabIsVisibleAndTheUrlIsTheSameAsTheDomainOrHost) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, Now() + kPageLandAfter.Get()));
  EXPECT_CALL(observer_mock_, OnDidLandOnPage(ad));
  site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest, DoNotLandOnPageIfNotVisible) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/new_tab")},
      /*is_visible=*/false);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, Now() + kPageLandAfter.Get()));
  EXPECT_CALL(observer_mock_, OnDidNotLandOnPage(ad));
  site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest,
       DoNotLandOnPageIfTheVisibleTabUrlIsNotTheSameAsTheDomainOrHost) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, Now() + kPageLandAfter.Get()));
  EXPECT_CALL(observer_mock_, OnDidNotLandOnPage(ad));
  site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest, CancelPageLandIfTheTabIsClosed) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, Now() + kPageLandAfter.Get()));
  EXPECT_CALL(observer_mock_, OnCanceledPageLand(ad, /*tab_id=*/1));
  site_visit_->MaybeLandOnPage(/*tab_id=*/1, {GURL("https://brave.com")});
  NotifyDidCloseTab(/*tab_id=*/1);
}

}  // namespace brave_ads
