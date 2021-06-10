/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_transfer/ad_transfer.h"

#include <memory>

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdTransferTest : public AdTransferObserver, public UnitTestBase {
 protected:
  BatAdsAdTransferTest() : ad_transfer_(std::make_unique<AdTransfer>()) {
    ad_transfer_->AddObserver(this);
  }

  ~BatAdsAdTransferTest() override { ad_transfer_->RemoveObserver(this); }

  void OnAdTransfer(const AdInfo& ad) override { ad_transfer_count_++; }

  void OnAdTransferFailed(const AdInfo& ad) override { ad_transfer_count_--; }

  int GetAdTransferCount() { return ad_transfer_count_; }

  AdInfo GetAdForType(const AdType type) {
    AdInfo ad;

    ad.type = type;
    ad.creative_instance_id = "c71b357a-89b9-4c4a-b71e-22654d4e557e";
    ad.creative_set_id = "01cd57da-1fa2-460b-a95d-4cc8cbd25e21";
    ad.campaign_id = "579e4e33-8c26-418f-9936-236142e0a697";
    ad.advertiser_id = "9ed47e65-9744-497b-9102-3a6424055f0e";
    ad.segment = "Technology & Computing";
    ad.target_url = "https://www.brave.com";

    return ad;
  }

  std::unique_ptr<AdTransfer> ad_transfer_;

 private:
  int ad_transfer_count_ = 0;
};

TEST_F(BatAdsAdTransferTest, DoNotTransferAdIfUrlIsMissingHTTPOrHTTPSScheme) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kPromotedContentAd);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://brave.com", /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  ad_transfer_->MaybeTransferAd(1, "brave.com");
  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(0, GetAdTransferCount());
}

TEST_F(BatAdsAdTransferTest,
       DoNotTransferAdIfTheUrlDoesNotMatchTheLastClickedAd) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kNewTabPageAd);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://foobar.com", /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  ad_transfer_->MaybeTransferAd(1, "brave.com");
  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(0, GetAdTransferCount());
}

TEST_F(BatAdsAdTransferTest, DoNotTransferAdIfTheSameAdIsAlreadyTransferring) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kAdNotification);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://brave.com", /* is_visible */ true,
                               /* is_incognito */ false);

  ad_transfer_->MaybeTransferAd(1, "https://brave.com");

  // Act
  ad_transfer_->MaybeTransferAd(1, "https://brave.com");
  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(1, GetAdTransferCount());
}

TEST_F(BatAdsAdTransferTest, TransferAdIfAnotherAdIsAlreadyTransferring) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kPromotedContentAd);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://foobar.com", /* is_visible */ true,
                               /* is_incognito */ false);

  ad_transfer_->MaybeTransferAd(1, "https://foobar.com");

  TabManager::Get()->OnUpdated(1, "https://foobar.com", /* is_visible */ false,
                               /* is_incognito */ false);

  TabManager::Get()->OnUpdated(2, "https://brave.com", /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  ad_transfer_->MaybeTransferAd(2, "https://brave.com");
  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(1, GetAdTransferCount());
}

TEST_F(BatAdsAdTransferTest,
       TransferAdIfTheTabIsVisibleAndTheUrlIsTheSameAsTheDomainOrHost) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kNewTabPageAd);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://brave.com", /* is_visible */ true,
                               /* is_incognito */ false);

  // Act
  ad_transfer_->MaybeTransferAd(1, "https://brave.com");
  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(1, GetAdTransferCount());
}

TEST_F(BatAdsAdTransferTest, FailToTransferAdIfNotVisible) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kAdNotification);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://brave.com", /* is_visible */ false,
                               /* is_incognito */ false);

  // Act
  ad_transfer_->MaybeTransferAd(1, "https://brave.com");
  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(-1, GetAdTransferCount());
}

TEST_F(BatAdsAdTransferTest,
       FailToTransferAdIfTheTabUrlIsNotTheSameAsTheDomainOrHost) {
  // Arrange
  const AdInfo ad = GetAdForType(AdType::kInlineContentAd);
  ad_transfer_->SetLastClickedAd(ad);

  TabManager::Get()->OnUpdated(1, "https://brave.com", /* is_visible */ true,
                               /* is_incognito */ false);

  ad_transfer_->MaybeTransferAd(1, "https://brave.com");

  // Act
  TabManager::Get()->OnUpdated(1, "https://foobar.com", /* is_visible */ true,
                               /* is_incognito */ false);

  FastForwardClockBy(base::TimeDelta::FromSeconds(10));

  // Assert
  EXPECT_EQ(-1, GetAdTransferCount());
}

}  // namespace ads
