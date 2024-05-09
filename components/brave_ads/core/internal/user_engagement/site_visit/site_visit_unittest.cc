/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"

#include <memory>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
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

    NotifyBrowserDidEnterForeground();
    NotifyBrowserDidBecomeActive();
  }

  void TearDown() override {
    site_visit_->RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  std::unique_ptr<SiteVisit> site_visit_;
  SiteVisitObserverMock observer_mock_;

  ::testing::InSequence s_;
};

TEST_F(BraveAdsSiteVisitTest, DoNotLandOnPageIfTheLastClickedAdIsInvalid) {
  // Arrange
  const AdInfo ad;
  site_visit_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidLandOnPage).Times(0);
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest,
       DoNotLandOnPageIfTheRedirectChainDoesNotMatchTheLastClickedAd) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidLandOnPage).Times(0);
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest, DoNotLandOnPageIfTheSameTabIsAlreadyLanding) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/about")},
      /*is_error_page=*/false, /*is_visible=*/true);

  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad));
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(
    BraveAdsSiteVisitTest,
    SuspendPageLandWhenTabBecomesOccludedThenResumePageLandWhenTabBecomesVisible) {
  // Tab 1 (Visible/Start page landing)
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad_1);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad_1, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Tab 1 (Occluded/Suspend page landing)
  AdvanceClockBy(kPageLandAfter.Get() - base::Seconds(3));

  EXPECT_CALL(observer_mock_,
              OnDidSuspendPageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/false,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(3)));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/false);

  ASSERT_FALSE(HasPendingTasks());

  // Tab 2 (Visible/Start page landing)
  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad_2);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad_2, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Tab 2 (Occluded/Suspend page landing)
  AdvanceClockBy(kPageLandAfter.Get() - base::Seconds(7));

  EXPECT_CALL(observer_mock_,
              OnDidSuspendPageLand(
                  TabInfo{/*id=*/2,
                          /*is_visible=*/false,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(7)));
  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/false);
  ASSERT_FALSE(HasPendingTasks());

  // Tab 1 (Visible/Resume page landing)
  EXPECT_CALL(observer_mock_,
              OnDidResumePageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(3)));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad_1));
  FastForwardClockToNextPendingTask();

  // Tab 2 (Visible/Resume page landing)
  EXPECT_CALL(observer_mock_,
              OnDidResumePageLand(
                  TabInfo{/*id=*/2,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(7)));
  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/2,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad_2));
  FastForwardClockToNextPendingTask();
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(
    BraveAdsSiteVisitTest,
    SuspendPageLandWhenBrowserEntersBackgroundThenResumePageLandWhenBrowserEntersForeground) {
  // Tab (Start page landing)
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Browser (Entered background/Suspend page landing)
  AdvanceClockBy(kPageLandAfter.Get() - base::Seconds(3));

  EXPECT_CALL(observer_mock_,
              OnDidSuspendPageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(3)));

  NotifyBrowserDidEnterBackground();
  ASSERT_FALSE(HasPendingTasks());

  // Tab 1 (Visible/Resume page landing)
  EXPECT_CALL(observer_mock_,
              OnDidResumePageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(3)));

  NotifyBrowserDidEnterForeground();
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad));
  FastForwardClockToNextPendingTask();
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(
    BraveAdsSiteVisitTest,
    SuspendPageLandWhenBrowserResignsActiveThenResumePageLandWhenBrowserBecomesActive) {
  // Tab (Start page landing)
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Browser (Entered background/Suspend page landing)
  AdvanceClockBy(kPageLandAfter.Get() - base::Seconds(3));

  EXPECT_CALL(observer_mock_,
              OnDidSuspendPageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(3)));

  NotifyBrowserDidResignActive();
  ASSERT_FALSE(HasPendingTasks());

  // Tab 1 (Visible/Resume page landing)
  EXPECT_CALL(observer_mock_,
              OnDidResumePageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  /*remaining_time=*/base::Seconds(3)));

  NotifyBrowserDidBecomeActive();
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad));
  FastForwardClockToNextPendingTask();
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsSiteVisitTest,
       LandOnPagesForMultipleSiteVisitsOccurringAtTheSameTime) {
  // Tab 1 (Visible/Start page landing)
  const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad_1);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad_1, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Tab 1 (Occluded/Suspend page landing)
  EXPECT_CALL(observer_mock_,
              OnDidSuspendPageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/false,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/false);
  ASSERT_FALSE(HasPendingTasks());

  // Tab 2 (Visible/Start page landing)
  const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                    /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad_2);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad_2, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Tab 2 (Occluded/Suspend page landing)
  EXPECT_CALL(observer_mock_,
              OnDidSuspendPageLand(
                  TabInfo{/*id=*/2,
                          /*is_visible=*/false,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/false);
  ASSERT_FALSE(HasPendingTasks());

  // Tab 1 (Visible/Resume page landing)
  EXPECT_CALL(observer_mock_,
              OnDidResumePageLand(
                  TabInfo{/*id=*/1,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad_1));
  FastForwardClockToNextPendingTask();

  // Tab 2 (Visible/Resume page landing)
  EXPECT_CALL(observer_mock_,
              OnDidResumePageLand(
                  TabInfo{/*id=*/2,
                          /*is_visible=*/true,
                          /*redirect_chain=*/{GURL("https://brave.com")},
                          /*is_error_page=*/false,
                          /*is_playing_media=*/false},
                  kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/2,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad_2));
  FastForwardClockToNextPendingTask();

  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsSiteVisitTest,
       LandOnPageIfTheTabIsVisibleAndTheRedirectChainMatchesTheLastClickedAd) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/false,
                              /*is_playing_media=*/false},
                      ad));
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(
    BraveAdsSiteVisitTest,
    LandOnPageIfTheTabIsVisibleAndTheRedirectChainMatchesTheLastClickedAdForHttpResponseStatusErrorPage) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  EXPECT_CALL(observer_mock_,
              OnMaybeLandOnPage(ad, /*after=*/kPageLandAfter.Get()));
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/true, /*is_visible=*/true);
  ASSERT_EQ(1U, GetPendingTaskCount());

  // Act & Assert
  EXPECT_CALL(
      observer_mock_,
      OnDidLandOnPage(TabInfo{/*id=*/1,
                              /*is_visible=*/true,
                              /*redirect_chain=*/{GURL("https://brave.com")},
                              /*is_error_page=*/true,
                              /*is_playing_media=*/false},
                      ad));
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest, DoNotLandOnPageIfTheTabIsOccluded) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/new_tab")},
      /*is_error_page=*/false, /*is_visible=*/true);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/new_tab")},
      /*is_error_page=*/false, /*is_visible=*/false);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidLandOnPage).Times(0);
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(
    BraveAdsSiteVisitTest,
    DoNotLandOnPageIfTheVisibleTabRedirectChainDoesNotMatchTheLastClickedAd) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnDidLandOnPage).Times(0);
  FastForwardClockBy(kPageLandAfter.Get());
}

TEST_F(BraveAdsSiteVisitTest,
       CancelPageLandIfTheRedirectChainNoLongerMatchesTheAdTargetUrl) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnCanceledPageLand(/*tab_id=*/1, ad));
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_error_page=*/false, /*is_visible=*/true);
}

TEST_F(BraveAdsSiteVisitTest, CancelPageLandIfTheTabIsClosed) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  site_visit_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_error_page=*/false, /*is_visible=*/true);

  // Act & Assert
  EXPECT_CALL(observer_mock_, OnCanceledPageLand(/*tab_id=*/1, ad));
  NotifyDidCloseTab(/*tab_id=*/1);
}

}  // namespace brave_ads
