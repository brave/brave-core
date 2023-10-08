/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<PurchaseIntentResource>();
  }

  bool LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<PurchaseIntentResource> resource_;
};

TEST_F(BraveAdsPurchaseIntentResourceTest, IsNotInitialized) {
  // Act & Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestPathToTempPath(kInvalidResourceId,
                                             kPurchaseIntentResourceId));

  // Act & Assert
  EXPECT_FALSE(LoadResource(kCountryComponentId));
}

TEST_F(BraveAdsPurchaseIntentResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadFileResource(kPurchaseIntentResourceId,
                                             ::testing::_, ::testing::_))
      .WillByDefault(::testing::Invoke(
          [](const std::string& /*id=*/, const int /*version=*/,
             LoadFileCallback callback) {
            const base::FilePath path =
                GetFileResourcePath().AppendASCII(kMissingResourceId);

            base::File file(path, base::File::Flags::FLAG_OPEN |
                                      base::File::Flags::FLAG_READ);
            std::move(callback).Run(std::move(file));
          }));

  // Act & Assert
  EXPECT_FALSE(LoadResource(kCountryComponentId));
}

TEST_F(BraveAdsPurchaseIntentResourceTest, LoadResourceWhenLocaleDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kCountryComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenLocaleDidChangeIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  OptOutOfNotificationAdsForTesting();
  OptOutOfBraveNewsAdsForTesting();

  ASSERT_FALSE(LoadResource(kCountryComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceWhenLocaleDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kCountryComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       LoadResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kCountryComponentId));

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenOptedInToNotificationAdsPrefDidChangeIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  ASSERT_TRUE(LoadResource(kCountryComponentId));

  OptOutOfNotificationAdsForTesting();
  OptOutOfBraveNewsAdsForTesting();

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kCountryComponentId));

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Act & Assert
  EXPECT_TRUE(LoadResource(kCountryComponentId));
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidCountryComponentId) {
  // Act & Assert
  EXPECT_FALSE(LoadResource(kInvalidCountryComponentId));
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  OptOutOfNotificationAdsForTesting();
  OptOutOfBraveNewsAdsForTesting();

  // Act & Assert
  EXPECT_FALSE(LoadResource(kCountryComponentId));
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceWhenDidUpdateResourceComponent) {
  // Arrange
  ASSERT_TRUE(LoadResource(kCountryComponentId));

  // Act & Assert
  EXPECT_TRUE(LoadResource(kCountryComponentId));
}

}  // namespace brave_ads
