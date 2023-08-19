/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using testing::_;
using testing::Invoke;

class BraveAdsPurchaseIntentResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<PurchaseIntentResource>();
  }

  void LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
  }

  std::unique_ptr<PurchaseIntentResource> resource_;
};

TEST_F(BraveAdsPurchaseIntentResourceTest, IsNotInitialized) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestPathToTempPath(kInvalidResourceId,
                                             kPurchaseIntentResourceId));

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest, DoNotLoadMissingResource) {
  // Arrange
  EXPECT_CALL(ads_client_mock_,
              LoadFileResource(kPurchaseIntentResourceId, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest, LoadResourceWhenLocaleDidChange) {
  // Arrange
  LoadResource(kCountryComponentId);

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenLocaleDidChangeIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  DisableNotificationAdsForTesting();
  DisableBraveNewsAdsForTesting();

  LoadResource(kCountryComponentId);

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceWhenLocaleDidChange) {
  // Arrange
  LoadResource(kCountryComponentId);

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       LoadResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  LoadResource(kCountryComponentId);

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenOptedInToNotificationAdsPrefDidChangeIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  DisableNotificationAdsForTesting();
  DisableBraveNewsAdsForTesting();

  LoadResource(kCountryComponentId);

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  LoadResource(kCountryComponentId);

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Arrange

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidCountryComponentId) {
  // Arrange

  // Act
  LoadResource(kInvalidCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  DisableNotificationAdsForTesting();
  DisableBraveNewsAdsForTesting();

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceWhenDidUpdateResourceComponent) {
  // Arrange
  LoadResource(kCountryComponentId);

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

}  // namespace brave_ads
