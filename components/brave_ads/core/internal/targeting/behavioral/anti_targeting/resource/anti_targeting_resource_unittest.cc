/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/test/run_until.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_load_state_types.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/resource_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAntiTargetingResourceTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<AntiTargetingResource>();
  }

  std::unique_ptr<AntiTargetingResource> resource_;
};

TEST_F(BraveAdsAntiTargetingResourceTest, IsResourceNotLoaded) {
  // Act & Assert
  EXPECT_FALSE(resource_->GetManifestVersion());
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
  EXPECT_NE(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest, LoadResource) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));
  EXPECT_NE(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest, DoNotLoadMalformedResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kAntiTargetingResourceId));

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(resource_->GetManifestVersion());

  // Act & Assert
  ASSERT_TRUE(
      base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad; }));
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest, DoNotFlagFailureForUnsupportedVersion) {
  // Arrange: cause a genuine failure first so `GetLoadState()` starts at
  // `kFailedToLoad`, giving the assertion below an actual transition to wait
  // for.
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kAntiTargetingResourceId));
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(
      base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad; }));

  // Act
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kUnsupportedVersionAntiTargetingResourceId,
      /*to_path=*/kAntiTargetingResourceId));
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersionUpdate, test::kCountryComponentId);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] {
    return resource_->GetLoadState() != ResourceLoadStateType::kFailedToLoad;
  }));
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadResourceComponent(kAntiTargetingResourceId,
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
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  ASSERT_TRUE(
      base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad; }));
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       DoNotFlagFailureForUnregisteredResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadResourceComponent(kAntiTargetingResourceId,
                                                  /*version=*/::testing::_,
                                                  /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadResourceComponentCallback callback) {
        std::move(callback).Run(/*file=*/{}, /*exists=*/false);
      });

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
  EXPECT_NE(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       ResetFailureToLoadWhenNoLongerRequired) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kAntiTargetingResourceId));

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(
      base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kFailedToLoad; }));

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, false);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kFailedToLoad, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       DoNotLoadResourceWithInvalidCountryComponentId) {
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kInvalidCountryComponentId);

  // Act & Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       DoNotLoadResourceIfAllAdsAreDisabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       LoadResourceWhenNewTabPageAdsAreEnabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());

  // Act
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(prefs::kSponsoredEnabled, true);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       DoNotResetResourceIfNewTabPageAdsAlreadyEnabled) {
  // Arrange
  test::DisableNotificationAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));

  // Act
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       LoadResourceWhenNotificationAdsAreEnabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());

  // Act
  SetProfileBooleanPref(prefs::kNotificationsEnabled, true);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       DoNotResetResourceIfNotificationAdsAlreadyEnabled) {
  // Arrange
  test::DisableSponsoredAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));

  // Act
  SetProfileBooleanPref(prefs::kNotificationsEnabled, true);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsAntiTargetingResourceTest,
    DoNotLoadResourceWhenSponsoredAdsAreEnabledAndNewTabPageBackgroundImagesAreDisabled) {
  // Arrange
  test::DisableAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());

  // Act
  SetProfileBooleanPref(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsAntiTargetingResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithInvalidCountryComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kInvalidCountryComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsAntiTargetingResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithExistingManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsAntiTargetingResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithNewManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));
  ASSERT_EQ(test::kCountryComponentManifestVersion,
            resource_->GetManifestVersion());

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersionUpdate, test::kCountryComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
  EXPECT_EQ(test::kCountryComponentManifestVersionUpdate,
            resource_->GetManifestVersion());
}

TEST_F(BraveAdsAntiTargetingResourceTest,
       ResetResourceForOnNotifyDidUnregisterResourceComponent) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kCountryComponentId);

  // Assert
  EXPECT_NE(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

TEST_F(
    BraveAdsAntiTargetingResourceTest,
    DoNotResetResourceForOnNotifyDidUnregisterResourceComponentWithInvalidCountryComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->GetLoadState() == ResourceLoadStateType::kLoaded; }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kInvalidCountryComponentId);

  // Assert
  EXPECT_EQ(ResourceLoadStateType::kLoaded, resource_->GetLoadState());
}

}  // namespace brave_ads
