/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_util.h"
#include "brave/components/brave_ads/core/internal/resources/contextual/text_classification/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/internal/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/resources/resources_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using testing::_;
using testing::Invoke;

class BraveAdsTextClassificationResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<TextClassificationResource>();
  }

  void LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
  }

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationResourceTest, IsNotInitialized) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  CopyFileFromTestPathToTempPath(kInvalidResourceId,
                                 kTextClassificationResourceId);

  // Act
  LoadResource(kLanguageComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadMissingResource) {
  // Arrange
  EXPECT_CALL(ads_client_mock_,
              LoadFileResource(kTextClassificationResourceId, _, _))
      .WillOnce(Invoke([](const std::string& /*id*/, const int /*version*/,
                          LoadFileCallback callback) {
        const base::FilePath path =
            GetFileResourcePath().AppendASCII(kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  // Act
  LoadResource(kLanguageComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenLocaleDidChange) {
  // Arrange
  LoadResource(kLanguageComponentId);

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWhenLocaleDidChangeIfOptedOutOfNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  LoadResource(kLanguageComponentId);

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceWhenLocaleDidChange) {
  // Arrange
  LoadResource(kLanguageComponentId);

  // Act
  NotifyLocaleDidChange(/*locale*/ "en_GB");
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  LoadResource(kLanguageComponentId);

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenOptedInToNotificationAdsPrefDidChangeIfOptedOutOfNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  LoadResource(kLanguageComponentId);

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  LoadResource(kLanguageComponentId);

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);
  task_environment_.RunUntilIdle();

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Arrange

  // Act
  LoadResource(kLanguageComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidLanguageComponentId) {
  // Arrange

  // Act
  LoadResource(kInvalidLanguageComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfOptedOutOfNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  // Act
  LoadResource(kLanguageComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceWhenDidUpdateResourceComponent) {
  // Arrange
  LoadResource(kLanguageComponentId);

  // Act
  LoadResource(kLanguageComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

}  // namespace brave_ads
