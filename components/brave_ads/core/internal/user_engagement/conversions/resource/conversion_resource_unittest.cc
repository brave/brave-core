/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_file_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_constants.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionResourceTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<ConversionResource>();
  }

  std::unique_ptr<ConversionResource> resource_;
};

TEST_F(BraveAdsConversionResourceTest, IsResourceNotLoaded) {
  // Act & Assert
  EXPECT_FALSE(resource_->GetManifestVersion());
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, LoadResource) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  // Act & Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadMalformedResource) {
  // Arrange
  ASSERT_TRUE(
      CopyFileFromTestPathToTempPath(/*from_path=*/test::kMalformedResourceId,
                                     /*to_path=*/kConversionResourceId));

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadComponentResource(kConversionResourceId,
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

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       DoNotLoadResourceWithInvalidCountryComponentId) {
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kInvalidCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadResourceForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, DoNotLoadResourceIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, LoadResourceForOnLocaleDidChange) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       DoNotLoadResourceForOnLocaleDidChangeIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  NotifyLocaleDidChange(/*locale=*/"en_GB");

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest, LoadResourceWhenOptingInToBraveNewsAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       DoNotResetResourceIfAlreadyOptedInToBraveNewsAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();
  test::OptOutOfNotificationAds();
  test::OptOutOfSearchResultAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       LoadResourceWhenOptingInToNewTabPageAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage,
                        true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       DoNotResetResourceIfAlreadyOptedInToNewTabPageAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNotificationAds();
  test::OptOutOfSearchResultAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage,
                        true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       LoadResourceWhenOptingInToNotificationAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       DoNotResetResourceIfAlreadyOptedInToNotificationAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNewTabPageAds();
  test::OptOutOfSearchResultAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       LoadResourceWhenOptingInToSearchResultAds) {
  // Arrange
  test::OptOutOfAllAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsConversionResourceTest,
       DoNotResetResourceIfAlreadyOptedInToSearchResultAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNewTabPageAds();
  test::OptOutOfNotificationAds();

  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsConversionResourceTest,
    DoNotResetResourceForOnDidUpdateResourceComponentWithInvalidCountryComponentId) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kInvalidCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsConversionResourceTest,
    DoNotResetResourceForOnDidUpdateResourceComponentWithExistingManifestVersion) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsConversionResourceTest,
    DoNotResetResourceForOnDidUpdateResourceComponentWithNewManifestVersion) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());
  ASSERT_EQ(test::kCountryComponentManifestVersion,
            resource_->GetManifestVersion());

  // Act
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersionUpdate,
                                   test::kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
  EXPECT_EQ(test::kCountryComponentManifestVersionUpdate,
            resource_->GetManifestVersion());
}

TEST_F(BraveAdsConversionResourceTest,
       ResetResourceForOnNotifyDidUnregisterResourceComponent) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUnregisterResourceComponent(test::kCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsConversionResourceTest,
    DoNotResetResourceForOnNotifyDidUnregisterResourceComponentWithInvalidCountryComponentId) {
  // Arrange
  NotifyDidUpdateResourceComponent(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act
  NotifyDidUnregisterResourceComponent(test::kInvalidCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

}  // namespace brave_ads
