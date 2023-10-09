/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/transfer/transfer.h"

#include <memory>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer_observer_mock.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/public/transfer/transfer_feature.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTransferTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    transfer_ = std::make_unique<Transfer>();
    transfer_->AddObserver(&observer_mock_);
  }

  void TearDown() override {
    transfer_->RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  std::unique_ptr<Transfer> transfer_;
  ::testing::StrictMock<TransferObserverMock> observer_mock_;

  ::testing::InSequence s_;
};

TEST_F(BraveAdsTransferTest, DoNotTransferInvalidAd) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad;
  transfer_->SetLastClickedAd(ad);

  // Act & Assert
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest,
       DoNotTransferAdIfTheUrlDoesNotMatchTheLastClickedAd) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  transfer_->SetLastClickedAd(ad);
  transfer_->MaybeTransferAd(/*tab_id=*/1,
                             {GURL("https://basicattentiontoken.org")});

  // Act & Assert
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest, DoNotTransferAdIfTheSameAdIsAlreadyTransferring) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  transfer_->SetLastClickedAd(ad);
  EXPECT_CALL(observer_mock_,
              OnWillTransferAd(ad, Now() + kTransferAfter.Get()));
  EXPECT_CALL(observer_mock_, OnDidTransferAd(ad));
  transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});

  // Act & Assert
  transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest, TransferAdIfAnotherAdIsAlreadyTransferring) {
  // Arrange
  {
    NotifyTabDidChange(
        /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
        /*is_visible=*/true);

    const AdInfo ad_1 = test::BuildAd(AdType::kNotificationAd,
                                      /*should_use_random_uuids=*/true);
    transfer_->SetLastClickedAd(ad_1);
    EXPECT_CALL(observer_mock_,
                OnWillTransferAd(ad_1, Now() + kTransferAfter.Get()));
    transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});
  }

  {
    NotifyTabDidChange(
        /*tab_id=*/2, /*redirect_chain=*/{GURL("https://brave.com")},
        /*is_visible=*/true);

    const AdInfo ad_2 = test::BuildAd(AdType::kNotificationAd,
                                      /*should_use_random_uuids=*/true);
    transfer_->SetLastClickedAd(ad_2);
    EXPECT_CALL(observer_mock_,
                OnWillTransferAd(ad_2, Now() + kTransferAfter.Get()));
    EXPECT_CALL(observer_mock_, OnDidTransferAd(ad_2));
    transfer_->MaybeTransferAd(/*tab_id=*/2, {GURL("https://brave.com")});
  }

  // Act & Assert
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest,
       TransferAdIfTheTabIsVisibleAndTheUrlIsTheSameAsTheDomainOrHost) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  transfer_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnWillTransferAd(ad, Now() + kTransferAfter.Get()));
  EXPECT_CALL(observer_mock_, OnDidTransferAd(ad));
  transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest, FailToTransferAdIfNotVisible) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com/new_tab")},
      /*is_visible=*/false);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  transfer_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnWillTransferAd(ad, Now() + kTransferAfter.Get()));
  EXPECT_CALL(observer_mock_, OnFailedToTransferAd(ad));
  transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest,
       FailToTransferAdIfTheVisibleTabUrlIsNotTheSameAsTheDomainOrHost) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1,
      /*redirect_chain=*/{GURL("https://basicattentiontoken.org")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  transfer_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnWillTransferAd(ad, Now() + kTransferAfter.Get()));
  EXPECT_CALL(observer_mock_, OnFailedToTransferAd(ad));
  transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});
  FastForwardClockBy(kTransferAfter.Get());
}

TEST_F(BraveAdsTransferTest, CancelTransferAdIfTheTabIsClosed) {
  // Arrange
  NotifyTabDidChange(
      /*tab_id=*/1, /*redirect_chain=*/{GURL("https://brave.com")},
      /*is_visible=*/true);

  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_use_random_uuids=*/true);
  transfer_->SetLastClickedAd(ad);

  // Act & Assert
  EXPECT_CALL(observer_mock_,
              OnWillTransferAd(ad, Now() + kTransferAfter.Get()));
  EXPECT_CALL(observer_mock_, OnCanceledTransfer(ad, /*tab_id=*/1));
  transfer_->MaybeTransferAd(/*tab_id=*/1, {GURL("https://brave.com")});
  NotifyDidCloseTab(/*tab_id=*/1);
}

}  // namespace brave_ads
