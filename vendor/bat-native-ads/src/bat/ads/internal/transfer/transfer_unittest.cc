/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/transfer/transfer.h"

#include <memory>

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTransferTest : public TransferObserver, public UnitTestBase {
 protected:
  BatAdsTransferTest() = default;

  ~BatAdsTransferTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    transfer_ = std::make_unique<Transfer>();
    transfer_->AddObserver(this);
  }

  void TearDown() override {
    transfer_->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnDidTransferAd(const AdInfo& ad) override { transfer_count_++; }

  void OnFailedToTransferAd(const AdInfo& ad) override { transfer_count_--; }

  int GetTransferCount() { return transfer_count_; }

  AdInfo BuildAdForType(const AdType type) {
    AdInfo ad;

    ad.type = type;
    ad.placement_id = "56b604b7-5eeb-4b7f-84cc-bf965556a550";
    ad.creative_instance_id = "c71b357a-89b9-4c4a-b71e-22654d4e557e";
    ad.creative_set_id = "01cd57da-1fa2-460b-a95d-4cc8cbd25e21";
    ad.campaign_id = "579e4e33-8c26-418f-9936-236142e0a697";
    ad.advertiser_id = "9ed47e65-9744-497b-9102-3a6424055f0e";
    ad.segment = "technology & computing";
    ad.target_url = GURL("https://www.brave.com");

    return ad;
  }

  std::unique_ptr<Transfer> transfer_;

 private:
  int transfer_count_ = 0;
};

TEST_F(BatAdsTransferTest, DoNotTransferAdIfUrlIsMissingHTTPOrHTTPSScheme) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kPromotedContentAd);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("brave.com")});
  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(0, GetTransferCount());
}

TEST_F(BatAdsTransferTest,
       DoNotTransferAdIfTheUrlDoesNotMatchTheLastClickedAd) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kNewTabPageAd);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("brave.com")});
  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(0, GetTransferCount());
}

TEST_F(BatAdsTransferTest, DoNotTransferAdIfTheSameAdIsAlreadyTransferring) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kAdNotification);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  // Act
  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});
  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(1, GetTransferCount());
}

TEST_F(BatAdsTransferTest, TransferAdIfAnotherAdIsAlreadyTransferring) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kSearchResultAd);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  transfer_->MaybeTransferAd(1, {GURL("https://foobar.com")});

  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"),
                               /* is_visible */ false,
                               /* is_incognito */ false);

  TabManager::Get()->OnUpdated(2, GURL("https://brave.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  transfer_->MaybeTransferAd(2, {GURL("https://brave.com")});
  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(1, GetTransferCount());
}

TEST_F(BatAdsTransferTest,
       TransferAdIfTheTabIsVisibleAndTheUrlIsTheSameAsTheDomainOrHost) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kNewTabPageAd);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});
  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(1, GetTransferCount());
}

TEST_F(BatAdsTransferTest, FailToTransferAdIfNotVisible) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kAdNotification);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"),
                               /* is_visible */ false,
                               /* is_incognito */ false);

  // Act
  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});
  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(-1, GetTransferCount());
}

TEST_F(BatAdsTransferTest,
       FailToTransferAdIfTheTabUrlIsNotTheSameAsTheDomainOrHost) {
  // Arrange
  const AdInfo ad = BuildAdForType(AdType::kInlineContentAd);
  transfer_->set_last_clicked_ad(ad);

  TabManager::Get()->OnUpdated(1, GURL("https://brave.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  transfer_->MaybeTransferAd(1, {GURL("https://brave.com")});

  // Act
  TabManager::Get()->OnUpdated(1, GURL("https://foobar.com"),
                               /* is_visible */ true,
                               /* is_incognito */ false);

  FastForwardClockBy(base::Seconds(10));

  // Assert
  EXPECT_EQ(-1, GetTransferCount());
}

}  // namespace ads
