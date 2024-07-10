/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTextClassificationResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<TextClassificationResource>();
  }

  std::unique_ptr<TextClassificationResource> resource_;
};

TEST_F(BraveAdsTextClassificationResourceTest, IsResourceNotLoaded) {
  // Act & Assert
  EXPECT_FALSE(resource_->GetManifestVersion());
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest, LoadResource) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);

  // Act & Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadMalformedResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestPathToTempPath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kTextClassificationResourceId));

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadComponentResource(kTextClassificationResourceId,
                                                  /*version=*/::testing::_,
                                                  /*callback=*/::testing::_))
      .WillByDefault(::testing::Invoke([](const std::string& /*id*/,
                                          const int /*version*/,
                                          LoadFileCallback callback) {
        const base::FilePath path =
            ComponentResourcesTestDataPath().AppendASCII(
                test::kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      }));

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWithInvalidLanguageComponentId) {
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kInvalidLanguageComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceForOnLocaleDidChange) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceForOnLocaleDidChangeIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWhenOptingInToBraveNewsAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWhenOptingInToNewTabPageAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage,
                        true);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       LoadResourceWhenOptingInToNotificationAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotResetResourceIfAlreadyOptedInToNotificationAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNewTabPageAds();
  test::OptOutOfSearchResultAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       DoNotLoadResourceWhenOptingInToSearchResultAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnDidUpdateResourceComponentWithInvalidLanguageComponentId) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kInvalidLanguageComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnDidUpdateResourceComponentWithExistingManifestVersion) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnDidUpdateResourceComponentWithNewManifestVersion) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());
  ASSERT_EQ(test::kLanguageComponentManifestVersion,
            resource_->GetManifestVersion());

  // Act
  NotifyDidUpdateResourceComponent(
      test::kLanguageComponentManifestVersionUpdate,
      test::kLanguageComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
  EXPECT_EQ(test::kLanguageComponentManifestVersionUpdate,
            resource_->GetManifestVersion());
}

TEST_F(BraveAdsTextClassificationResourceTest,
       ResetResourceForOnNotifyDidUnregisterResourceComponent) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUnregisterResourceComponent(test::kLanguageComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsTextClassificationResourceTest,
    DoNotResetResourceForOnNotifyDidUnregisterResourceComponentWithInvalidLanguageComponentId) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kLanguageComponentManifestVersion,
                                   test::kLanguageComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUnregisterResourceComponent(test::kInvalidLanguageComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

}  // namespace brave_ads
