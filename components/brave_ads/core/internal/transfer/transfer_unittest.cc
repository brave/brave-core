/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/transfer/transfer.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsTransferTest : public TransferObserver, public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    transfer_ = std::make_unique<Transfer>();
    transfer_->AddObserver(this);
  }

  void TearDown() override {
    transfer_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnDidTransferAd(const AdInfo& /*ad*/) override { transfer_count_++; }

  void OnFailedToTransferAd(const AdInfo& /*ad*/) override {
    transfer_count_--;
  }

  int GetTransferCount() const { return transfer_count_; }

  std::unique_ptr<Transfer> transfer_;

 private:
  int transfer_count_ = 0;
};

TEST_F(BatAdsTransferTest, DoNotTransferAdIfUrlIsMissingHTTPOrHTTPSScheme) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("brave.com")});
  AdvanceClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(0, GetTransferCount());
}

TEST_F(BatAdsTransferTest,
       DoNotTransferAdIfTheUrlDoesNotMatchTheLastClickedAd) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("brave.com")});
  AdvanceClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(0, GetTransferCount());
}

TEST_F(BatAdsTransferTest, DoNotTransferAdIfTheSameAdIsAlreadyTransferring) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  // Act
  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(1, GetTransferCount());
}

TEST_F(BatAdsTransferTest, TransferAdIfAnotherAdIsAlreadyTransferring) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  transfer_->MaybeTransferAd(1, {GURL("https://foobar.com")});

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ false,
      /*is_incognito*/ false);

  NotifyTabDidChange(
      /*id*/ 2, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  // Act
  transfer_->MaybeTransferAd(2, {GURL("https://brave.com")});

  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(1, GetTransferCount());
}

TEST_F(BatAdsTransferTest,
       TransferAdIfTheTabIsVisibleAndTheUrlIsTheSameAsTheDomainOrHost) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(1, GetTransferCount());
}

TEST_F(BatAdsTransferTest, FailToTransferAdIfNotVisible) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ false,
      /*is_incognito*/ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(-1, GetTransferCount());
}

TEST_F(BatAdsTransferTest,
       FailToTransferAdIfTheTabUrlIsNotTheSameAsTheDomainOrHost) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_guids*/ false);

  transfer_->SetLastClickedAd(ad);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  // Act
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://foobar.com")},
      /*is_visible*/ true,
      /*is_incognito*/ false);

  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(-1, GetTransferCount());
}

}  // namespace brave_ads
