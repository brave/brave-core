/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_internals/ads_internals_util.h"

#include <optional>

#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/test/confirmation_queue_item_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/test/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/test/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/test/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/test/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/test/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/test/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/test/creative_notification_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAdsAdsInternalsUtil*

namespace brave_ads {

class BraveAdsAdsInternalsUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdsInternalsUtilTest,
       BuildAdsInternalsContainsCreativeSetConversionJsonKeys) {
  // Arrange
  test::BuildAndSaveCreativeSetConversion(
      /*creative_set_id=*/"creative-set-id",
      /*url_pattern=*/"https://www.brave.com/*",
      /*observation_window=*/base::Days(7));

  AdInfo ad =
      test::BuildAd(mojom::AdType::kNotificationAd, /*use_random_uuids=*/false);
  ad.creative_set_id = "creative-set-id";
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const conversions =
      internals->FindList("creativeSetConversions");
  ASSERT_TRUE(conversions);
  ASSERT_EQ(1u, conversions->size());
  const base::DictValue* const conversion = (*conversions)[0].GetIfDict();
  ASSERT_TRUE(conversion);
  EXPECT_TRUE(conversion->Find("URL Pattern"));
  EXPECT_TRUE(conversion->Find("Expires At"));
}

TEST_F(BraveAdsAdsInternalsUtilTest, BuildAdsInternalsContainsAdEventJsonKeys) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(mojom::AdType::kNotificationAd, /*use_random_uuids=*/false);
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const ad_events = internals->FindList("adEvents");
  ASSERT_TRUE(ad_events);
  ASSERT_EQ(1u, ad_events->size());
  const base::DictValue* const ad_event = (*ad_events)[0].GetIfDict();
  ASSERT_TRUE(ad_event);
  EXPECT_TRUE(ad_event->Find("Creative Instance ID"));
  EXPECT_TRUE(ad_event->Find("Target URL"));
  EXPECT_TRUE(ad_event->Find("Ad Type"));
  EXPECT_TRUE(ad_event->Find("Event Type"));
  EXPECT_TRUE(ad_event->Find("Created At"));
}

TEST_F(BraveAdsAdsInternalsUtilTest, BuildAdsInternalsContainsPaymentTokens) {
  // Arrange
  const PaymentTokenInfo payment_token = test::BuildPaymentToken();
  database::table::PaymentTokens database_table;
  base::test::TestFuture<bool> save_future;
  database_table.Save({payment_token}, save_future.GetCallback());
  ASSERT_TRUE(save_future.Get());

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const payment_tokens =
      internals->FindList("paymentTokens");
  ASSERT_TRUE(payment_tokens);
  EXPECT_EQ(1u, payment_tokens->size());
}

TEST_F(BraveAdsAdsInternalsUtilTest, BuildAdsInternalsContainsTransactions) {
  // Arrange
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/1.0, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/base::Time(),
      /*use_random_uuids=*/false);
  database::table::Transactions database_table;
  base::test::TestFuture<bool> save_future;
  database_table.Save({transaction}, save_future.GetCallback());
  ASSERT_TRUE(save_future.Get());

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const transactions =
      internals->FindList("transactions");
  ASSERT_TRUE(transactions);
  EXPECT_EQ(1u, transactions->size());
}

TEST_F(BraveAdsAdsInternalsUtilTest,
       BuildAdsInternalsContainsConditionMatchers) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad = test::BuildCreativeNewTabPageAd(
      CreativeNewTabPageAdWallpaperType::kImage, /*use_random_uuids=*/false);
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  EXPECT_TRUE(internals->FindList("conditionMatchers"));
}

TEST_F(BraveAdsAdsInternalsUtilTest,
       BuildAdsInternalsContainsConfirmationQueue) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmationWithoutDynamicUserData(
          /*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  test::BuildAndSaveConfirmationQueueItems(*confirmation, /*count=*/1);

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const confirmation_queue =
      internals->FindList("confirmationQueue");
  ASSERT_TRUE(confirmation_queue);
  EXPECT_EQ(1u, confirmation_queue->size());
}

TEST_F(BraveAdsAdsInternalsUtilTest,
       BuildAdsInternalsContainsActiveNotificationAdCampaigns) {
  // Arrange
  database::SaveCreativeNotificationAds(
      test::BuildCreativeNotificationAds(/*count=*/1));

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const campaigns =
      internals->FindList("activeNotificationAdCampaigns");
  ASSERT_TRUE(campaigns);
  EXPECT_EQ(1u, campaigns->size());
  EXPECT_EQ(1, internals->FindInt("activeNotificationAdCount"));
}

TEST_F(BraveAdsAdsInternalsUtilTest,
       BuildAdsInternalsContainsActiveNewTabPageAdCampaigns) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad = test::BuildCreativeNewTabPageAd(
      CreativeNewTabPageAdWallpaperType::kImage, /*use_random_uuids=*/false);
  test::SaveCreativeNewTabPageAds({creative_ad});

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const campaigns =
      internals->FindList("activeNewTabPageAdCampaigns");
  ASSERT_TRUE(campaigns);
  EXPECT_EQ(1u, campaigns->size());
  EXPECT_EQ(1, internals->FindInt("activeNewTabPageAdCount"));
}

}  // namespace brave_ads
