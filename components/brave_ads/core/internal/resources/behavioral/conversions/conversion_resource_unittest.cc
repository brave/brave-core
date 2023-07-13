/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversion_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/conversions/conversion_resource_constants.h"
#include "brave/components/brave_ads/core/internal/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/resources/resources_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using testing::_;
using testing::Invoke;

class BraveAdsConversionResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<ConversionResource>();
  }

  void LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
  }

  std::unique_ptr<ConversionResource> resource_;
};

TEST_F(BraveAdsConversionResourceTest, IsNotInitialized) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidResourceId, kConversionResourceId);

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadMissingResource) {
  // Arrange
  EXPECT_CALL(ads_client_mock_, LoadFileResource(kConversionResourceId, _, _))
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

TEST_F(BraveAdsConversionResourceTest, LoadResourceWhenLocaleDidChange) {
  // Arrange

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsConversionResourceTest,
    LoadResourceWhenLocaleDidChangeIfBravePrivateAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBravePrivateAds();
  DisableBraveNewsAds();

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsConversionResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Arrange

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsConversionResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidCountryComponentId) {
  // Arrange

  // Act
  LoadResource(kInvalidCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsConversionResourceTest,
    LoadResourceWhenDidUpdateResourceComponentIfBravePrivateAdsAndBraveNewsAdsAreDisabled) {
  // Arrange
  DisableBravePrivateAds();
  DisableBraveNewsAds();

  // Act
  LoadResource(kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

}  // namespace brave_ads
