// /* Copyright (c) 2020 The Brave Authors. All rights reserved.
//  * This Source Code Form is subject to the terms of the Mozilla Public
//  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
//  * You can obtain one at http://mozilla.org/MPL/2.0/. */

// TODO(tmancey): Implement

// #include
// "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications.h"

// #include <memory>

// #include "base/strings/string_number_conversions.h"
// #include
// "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
// #include "bat/ads/internal/container_util.h"
// #include
// "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
// #include "bat/ads/internal/unittest_base.h"
// #include "bat/ads/internal/unittest_util.h"

// // npm run test -- brave_unit_tests --filter=BatAds*

// namespace ads {
// namespace ad_notifications {

// class BatAdsEligibleAdNotificationsTest : public UnitTestBase {
//  protected:
//   BatAdsEligibleAdNotificationsTest()
//       : subdivision_targeting_(
//             std::make_unique<ad_targeting::geographic::SubdivisionTargeting>()),
//         anti_targeting_(std::make_unique<resource::AntiTargeting>()),
//         eligible_ads_(
//             std::make_unique<EligibleAds>(subdivision_targeting_.get(),
//                                           anti_targeting_.get())) {}

//   ~BatAdsEligibleAdNotificationsTest() override = default;

//   CreativeAdNotificationList GetAds(const int count) {
//     CreativeAdNotificationList ads;

//     for (int i = 0; i < count; i++) {
//       CreativeAdNotificationInfo ad;

//       const int creative_instance_id = i + 1;
//       ad.creative_instance_id = base::NumberToString(creative_instance_id);

//       ad.daily_cap = 1;

//       const int advertiser_id = 1 + (i / 2);
//       ad.advertiser_id = base::NumberToString(advertiser_id);

//       ad.per_day = 1;
//       ad.per_week = 1;
//       ad.per_month = 1;
//       ad.total_max = 1;

//       ads.push_back(ad);
//     }

//     return ads;
//   }

//   std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
//       subdivision_targeting_;
//   std::unique_ptr<resource::AntiTargeting> anti_targeting_;
//   std::unique_ptr<EligibleAds> eligible_ads_;
// };

// TEST_F(BatAdsEligibleAdNotificationsTest, NoSeenAdvertisersOrAds) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   eligible_ads_->GetForSegments(ads, last_delivered_ad, {}, {});

//   // Assert
//   const CreativeAdNotificationList expected_ads = ads;
//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, EligibleAd) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdNotification("1");
//   Client::Get()->UpdateSeenAdNotification("2");
//   Client::Get()->UpdateSeenAdNotification("3");
//   Client::Get()->UpdateSeenAdNotification("4");
//   Client::Get()->UpdateSeenAdNotification("5");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   CreativeAdNotificationInfo ad;
//   ad.creative_instance_id = "6";
//   ad.advertiser_id = "3";

//   const CreativeAdNotificationList expected_ads = {ad};

//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, EligibleAds) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdNotification("1");
//   Client::Get()->UpdateSeenAdNotification("2");
//   Client::Get()->UpdateSeenAdNotification("4");
//   Client::Get()->UpdateSeenAdNotification("5");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   CreativeAdNotificationInfo ad_1;
//   ad_1.creative_instance_id = "3";
//   ad_1.advertiser_id = "2";

//   CreativeAdNotificationInfo ad_2;
//   ad_2.creative_instance_id = "6";
//   ad_2.advertiser_id = "3";

//   const CreativeAdNotificationList expected_ads = {ad_1, ad_2};

//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, EligibleAdsRoundRobin) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdNotification("1");
//   Client::Get()->UpdateSeenAdNotification("2");
//   Client::Get()->UpdateSeenAdNotification("3");
//   Client::Get()->UpdateSeenAdNotification("4");
//   Client::Get()->UpdateSeenAdNotification("5");
//   Client::Get()->UpdateSeenAdNotification("6");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   const CreativeAdNotificationList expected_ads = ads;
//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, EligibleAdvertiser) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdvertiser("1");
//   Client::Get()->UpdateSeenAdvertiser("2");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   CreativeAdNotificationInfo ad_1;
//   ad_1.creative_instance_id = "5";
//   ad_1.advertiser_id = "3";

//   CreativeAdNotificationInfo ad_2;
//   ad_2.creative_instance_id = "6";
//   ad_2.advertiser_id = "3";

//   const CreativeAdNotificationList expected_ads = {ad_1, ad_2};

//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, EligibleAdvertisers) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdvertiser("1");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   CreativeAdNotificationInfo ad_1;
//   ad_1.creative_instance_id = "3";
//   ad_1.advertiser_id = "2";

//   CreativeAdNotificationInfo ad_2;
//   ad_2.creative_instance_id = "4";
//   ad_2.advertiser_id = "2";

//   CreativeAdNotificationInfo ad_3;
//   ad_3.creative_instance_id = "5";
//   ad_3.advertiser_id = "3";

//   CreativeAdNotificationInfo ad_4;
//   ad_4.creative_instance_id = "6";
//   ad_4.advertiser_id = "3";

//   const CreativeAdNotificationList expected_ads = {ad_1, ad_2, ad_3, ad_4};

//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, EligibleAdvertisersRoundRobin) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdvertiser("1");
//   Client::Get()->UpdateSeenAdvertiser("2");
//   Client::Get()->UpdateSeenAdvertiser("3");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   const CreativeAdNotificationList expected_ads = ads;
//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, RoundRobin) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(6);

//   const CreativeAdInfo last_delivered_ad;

//   Client::Get()->UpdateSeenAdNotification("1");
//   Client::Get()->UpdateSeenAdNotification("2");
//   Client::Get()->UpdateSeenAdvertiser("1");
//   Client::Get()->UpdateSeenAdNotification("3");
//   Client::Get()->UpdateSeenAdNotification("4");
//   Client::Get()->UpdateSeenAdvertiser("2");
//   Client::Get()->UpdateSeenAdNotification("5");
//   Client::Get()->UpdateSeenAdNotification("6");
//   Client::Get()->UpdateSeenAdvertiser("3");

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   const CreativeAdNotificationList expected_ads = ads;
//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, LastDeliveredAd) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(2);

//   CreativeAdInfo last_delivered_ad;
//   last_delivered_ad.advertiser_id = "1";
//   last_delivered_ad.creative_instance_id = "1";

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   CreativeAdNotificationInfo ad;
//   ad.creative_instance_id = "2";
//   ad.advertiser_id = "1";

//   const CreativeAdNotificationList expected_ads = {ad};

//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// TEST_F(BatAdsEligibleAdNotificationsTest, LastDeliveredAdForSingleAd) {
//   // Arrange
//   const CreativeAdNotificationList ads = GetAds(2);

//   CreativeAdInfo last_delivered_ad;
//   last_delivered_ad.advertiser_id = "1";
//   last_delivered_ad.creative_instance_id = "1";

//   // Act
//   const CreativeAdNotificationList eligible_ads =
//       eligible_ads_->Get(ads, last_delivered_ad, {}, {});

//   // Assert
//   CreativeAdNotificationInfo ad;
//   ad.creative_instance_id = "1";
//   ad.advertiser_id = "1";

//   const CreativeAdNotificationList expected_ads = {ad};

//   EXPECT_TRUE(CompareAsSets(expected_ads, eligible_ads));
// }

// }  // namespace ad_notifications
// }  // namespace ads
