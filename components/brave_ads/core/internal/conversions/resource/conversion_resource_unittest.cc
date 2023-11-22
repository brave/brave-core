/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/resource/conversion_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/conversions/resource/conversion_resource_constants.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<ConversionResource>();
  }

  bool LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<ConversionResource> resource_;
};

TEST_F(BraveAdsConversionResourceTest, IsNotInitialized) {
  // Act & Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestPathToTempPath(kInvalidResourceId,
                                             kConversionResourceId));

  // Act & Assert
  EXPECT_FALSE(LoadResource(kCountryComponentId));
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_,
          LoadFileResource(kConversionResourceId, ::testing::_, ::testing::_))
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

TEST_F(BraveAdsConversionResourceTest, LoadResourceWhenLocaleDidChange) {
  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsConversionResourceTest,
    LoadResourceWhenLocaleDidChangeIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  test::OptOutOfNotificationAds();
  test::OptOutOfBraveNewsAds();

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsConversionResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Act & Assert
  EXPECT_TRUE(LoadResource(kCountryComponentId));
}

TEST_F(
    BraveAdsConversionResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidCountryComponentId) {
  // Act & Assert
  EXPECT_FALSE(LoadResource(kInvalidCountryComponentId));
}

TEST_F(
    BraveAdsConversionResourceTest,
    LoadResourceWhenDidUpdateResourceComponentIfNotificationAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  test::OptOutOfNotificationAds();
  test::OptOutOfBraveNewsAds();

  // Act & Assert
  EXPECT_TRUE(LoadResource(kCountryComponentId));
}

}  // namespace brave_ads
