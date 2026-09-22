/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/test/run_until.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_load_state_types.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/resource_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextClassificationResourceTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<TextClassificationResource>();
  }

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationResourceTest, IsResourceNotLoaded) {
  // Act & Assert
  EXPECT_FALSE(resource_->GetManifestVersion());
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest, LoadResource) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadMalformedResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kTextClassificationResourceId));

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(resource_->GetManifestVersion());

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad;
  }));
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadResourceComponent(kTextClassificationResourceId,
                                                  /*version=*/::testing::_,
                                                  /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadResourceComponentCallback callback) {
        const base::FilePath path =
            test::ResourceComponentsDataPath().AppendASCII(
                test::kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file), /*exists=*/true);
      });

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  EXPECT_EQ(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotFlagFailureForUnregisteredResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadResourceComponent(kTextClassificationResourceId,
                                                  /*version=*/::testing::_,
                                                  /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadResourceComponentCallback callback) {
        std::move(callback).Run(/*file=*/{}, /*exists=*/false);
      });

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  EXPECT_EQ(ResourceLoadStateType::kNotLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       ResetFailureToLoadWhenNoLongerRequired) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kTextClassificationResourceId));

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad;
  }));

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, false);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWithInvalidLanguageComponentId) {
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion,
      test::kInvalidLanguageComponentId);

  // Act & Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceIfAllAdsAreDisabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Act & Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWhenNewTabPageAdsAreEnabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());

  // Act
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenNotificationAdsAreEnabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());

  // Act
  SetProfileBooleanPref(prefs::kNotificationsEnabled, true);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceIfNotificationAdsAlreadyEnabled) {
  // Arrange
  test::DisableSponsoredAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  SetProfileBooleanPref(prefs::kNotificationsEnabled, true);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotLoadResourceWhenSponsoredAdsAreEnabledAndNewTabPageBackgroundImagesAreDisabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());

  // Act
  SetProfileBooleanPref(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithInvalidLanguageComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion,
      test::kInvalidLanguageComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithExistingManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithNewManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));
  ASSERT_EQ(test::kLanguageComponentManifestVersion,
            resource_->GetManifestVersion());

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersionUpdate,
      test::kLanguageComponentId);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));
  EXPECT_EQ(test::kLanguageComponentManifestVersionUpdate,
            resource_->GetManifestVersion());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       ResetResourceForOnNotifyDidUnregisterResourceComponent) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kLanguageComponentId);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnNotifyDidUnregisterResourceComponentWithInvalidLanguageComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kLanguageComponentManifestVersion, test::kLanguageComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() == ResourceLoadStateType::kLoaded;
  }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kInvalidLanguageComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

}  // namespace brave_ads
