/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/resource/text_embedding_resource.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resources_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/resource/text_embedding_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextEmbeddingResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<TextEmbeddingResource>();
  }

  bool LoadResource(const std::string& id) {
    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion, id);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<TextEmbeddingResource> resource_;
};

TEST_F(BraveAdsTextEmbeddingResourceTest, IsNotInitialized) {
  // Act & Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest, DoNotLoadInvalidResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestPathToTempPath(kInvalidResourceId,
                                             kTextEmbeddingResourceId));

  // Act & Assert
  EXPECT_FALSE(LoadResource(kLanguageComponentId));
}

TEST_F(BraveAdsTextEmbeddingResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadFileResource(kTextEmbeddingResourceId,
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
  EXPECT_FALSE(LoadResource(kLanguageComponentId));
}

TEST_F(BraveAdsTextEmbeddingResourceTest, LoadResourceWhenLocaleDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest,
       DoNotLoadResourceWhenLocaleDidChangeIfOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  ASSERT_FALSE(LoadResource(kLanguageComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest,
       DoNotResetResourceWhenLocaleDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest,
       LoadResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(
    BraveAdsTextEmbeddingResourceTest,
    DoNotLoadResourceWhenOptedInToNotificationAdsPrefDidChangeIfOptedOutOfNotificationAds) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  test::OptOutOfNotificationAds();

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_FALSE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest,
       ResetResourceWhenOptedInToNotificationAdsPrefDidChange) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act
  NotifyPrefDidChange(prefs::kOptedInToNotificationAds);

  // Assert
  EXPECT_TRUE(resource_->IsInitialized());
}

TEST_F(BraveAdsTextEmbeddingResourceTest,
       LoadResourceWhenDidUpdateResourceComponent) {
  // Act & Assert
  EXPECT_TRUE(LoadResource(kLanguageComponentId));
}

TEST_F(
    BraveAdsTextEmbeddingResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfInvalidLanguageComponentId) {
  // Act & Assert
  EXPECT_FALSE(LoadResource(kInvalidLanguageComponentId));
}

TEST_F(
    BraveAdsTextEmbeddingResourceTest,
    DoNotLoadResourceWhenDidUpdateResourceComponentIfOptedOutOfNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  // Act & Assert
  EXPECT_FALSE(LoadResource(kLanguageComponentId));
}

TEST_F(BraveAdsTextEmbeddingResourceTest,
       DoNotResetResourceWhenDidUpdateResourceComponent) {
  // Arrange
  ASSERT_TRUE(LoadResource(kLanguageComponentId));

  // Act & Assert
  EXPECT_TRUE(LoadResource(kLanguageComponentId));
}

}  // namespace brave_ads
