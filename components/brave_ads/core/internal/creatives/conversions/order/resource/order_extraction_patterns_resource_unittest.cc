/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource.h"

#include <memory>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_load_state_types.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kValidConversionConfig[] = R"JSON({
  "version": 1,
  "conversions": {
    "order": {}
  }
})JSON";

constexpr char kMalformedConversionConfig[] = R"JSON({
  "version": 1,
  "conversions": {
    "order": {
      "id": {
        "regex": {
          "patterns": {
            "en": 123
          }
        }
      }
    }
  }
})JSON";

}  // namespace

class BraveAdsOrderExtractionPatternsResourceTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<OrderExtractionPatternsResource>();
  }

  std::unique_ptr<OrderExtractionPatternsResource> resource_;
};

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       ReturnDefaultPatternsIfResourceNotLoaded) {
  // Act & Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
  const OrderExtractionPatternsInfo& patterns = resource_->GetPatterns();
  EXPECT_THAT(patterns.id,
              ::testing::Field(&ComponentOrderIdExtractionPatternsInfo::regexes,
                               ::testing::Not(::testing::IsEmpty())));
  EXPECT_THAT(patterns.value,
              ::testing::Field(
                  &ComponentOrderValueExtractionPatternsInfo::currency_regexes,
                  ::testing::Not(::testing::IsEmpty())));
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest, LoadResource) {
  // Act
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(kValidConversionConfig));

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoesNotLoadMalformedResource) {
  // Act
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(kMalformedConversionConfig));

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoesNotLoadResourceWithUnsupportedVersion) {
  // Act
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(R"JSON({
        "version": 999,
        "conversions": {
          "order": {}
        }
      })JSON"));

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       PersistLoadedResourceToPref) {
  // Act
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(kValidConversionConfig));

  // Assert
  std::optional<base::Value> pref_value =
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig);
  ASSERT_TRUE(pref_value);
  ASSERT_TRUE(pref_value->is_dict());
  EXPECT_EQ(1, pref_value->GetDict().FindInt("version"));
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoesNotPersistMalformedResourceToPref) {
  // Arrange
  test::SetProfileDictPrefValue(prefs::kOrderExtractionPatternsConfig,
                                base::DictValue().Set("version", 1));

  // Act
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(kMalformedConversionConfig));

  // Assert: the previously-persisted value is untouched.
  std::optional<base::Value> pref_value =
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig);
  ASSERT_TRUE(pref_value);
  ASSERT_TRUE(pref_value->is_dict());
  EXPECT_EQ(1, pref_value->GetDict().FindInt("version"));
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       ClearPrefWhenContentNoLongerAvailable) {
  // Arrange
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(kValidConversionConfig));
  ASSERT_TRUE(
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig));

  // Act
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(std::nullopt);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
  std::optional<base::Value> pref_value =
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig);
  ASSERT_TRUE(pref_value);
  ASSERT_TRUE(pref_value->is_dict());
  EXPECT_FALSE(pref_value->GetDict().FindInt("version"));
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       ClearPrefWhenComponentIsUnregistered) {
  // Arrange
  ads_client_notifier_.NotifySponsoredContentComponentDidChange(
      base::test::ParseJsonDict(kValidConversionConfig));
  ASSERT_TRUE(
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig));

  // Act
  ads_client_notifier_.NotifyDidUnregisterSponsoredContentComponent();

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
  std::optional<base::Value> pref_value =
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig);
  ASSERT_TRUE(pref_value);
  ASSERT_TRUE(pref_value->is_dict());
  EXPECT_FALSE(pref_value->GetDict().FindInt("version"));
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       LoadPatternsFromPrefAtConstruction) {
  // Arrange
  test::SetProfileDictPrefValue(prefs::kOrderExtractionPatternsConfig,
                                base::DictValue().Set("version", 1));

  // Act
  const OrderExtractionPatternsResource resource_from_pref;

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_from_pref.GetLoadState());
}

TEST_F(BraveAdsOrderExtractionPatternsResourceTest,
       DoesNotLoadPatternsFromPrefWithUnsupportedVersion) {
  // Arrange
  test::SetProfileDictPrefValue(prefs::kOrderExtractionPatternsConfig,
                                base::DictValue().Set("version", 999));

  // Act
  const OrderExtractionPatternsResource resource_from_pref;

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_from_pref.GetLoadState());
}

}  // namespace brave_ads
