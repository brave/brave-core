/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"

#include <memory>
#include <utility>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/test/run_until.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/test/resource_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/file_path_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentResourceTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<PurchaseIntentResource>();
  }

  std::unique_ptr<PurchaseIntentResource> resource_;
};

TEST_F(BraveAdsPurchaseIntentResourceTest, IsResourceNotLoaded) {
  // Act & Assert
  EXPECT_FALSE(resource_->GetManifestVersion());
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsPurchaseIntentResourceTest, LoadResource) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));
}

TEST_F(BraveAdsPurchaseIntentResourceTest, DoNotLoadMalformedResource) {
  // Arrange
  ASSERT_TRUE(CopyFileFromTestDataPathToProfilePath(
      /*from_path=*/test::kMalformedResourceId,
      /*to_path=*/kPurchaseIntentResourceId));

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(resource_->GetManifestVersion());

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsPurchaseIntentResourceTest, DoNotLoadMissingResource) {
  // Arrange
  ON_CALL(ads_client_mock_, LoadResourceComponent(kPurchaseIntentResourceId,
                                                  /*version=*/::testing::_,
                                                  /*callback=*/::testing::_))
      .WillByDefault([](const std::string& /*id*/, int /*version*/,
                        LoadFileCallback callback) {
        const base::FilePath path =
            test::ResourceComponentsDataPath().AppendASCII(
                test::kMissingResourceId);

        base::File file(
            path, base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
        std::move(callback).Run(std::move(file));
      });

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotLoadResourceWithInvalidCountryComponentId) {
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kInvalidCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotLoadResourceIfOptedOutOfAllAds) {
  // Arrange
  test::OptOutOfAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Act & Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotLoadResourceWhenOptingInToNewTabPageAds) {
  // Arrange
  test::OptOutOfAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
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

TEST_F(BraveAdsPurchaseIntentResourceTest,
       LoadResourceWhenOptingInToNotificationAds) {
  // Arrange
  test::OptOutOfAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotResetResourceIfAlreadyOptedInToNotificationAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();
  test::OptOutOfSearchResultAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       DoNotLoadResourceWhenOptingInToSearchResultAds) {
  // Arrange
  test::OptOutOfAllAds();

  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_FALSE(resource_->IsLoaded());

  // Act
  SetProfileBooleanPref(prefs::kOptedInToSearchResultAds, true);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithInvalidCountryComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kInvalidCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithExistingManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotResetResourceForOnResourceComponentDidChangeWithNewManifestVersion) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));
  ASSERT_EQ(test::kCountryComponentManifestVersion,
            resource_->GetManifestVersion());

  // Act
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersionUpdate, test::kCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
  EXPECT_EQ(test::kCountryComponentManifestVersionUpdate,
            resource_->GetManifestVersion());
}

TEST_F(BraveAdsPurchaseIntentResourceTest,
       ResetResourceForOnNotifyDidUnregisterResourceComponent) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kCountryComponentId);

  // Assert
  EXPECT_FALSE(resource_->IsLoaded());
}

TEST_F(
    BraveAdsPurchaseIntentResourceTest,
    DoNotResetResourceForOnNotifyDidUnregisterResourceComponentWithInvalidCountryComponentId) {
  // Arrange
  ads_client_notifier_.NotifyResourceComponentDidChange(
      test::kCountryComponentManifestVersion, test::kCountryComponentId);
  ASSERT_TRUE(base::test::RunUntil([this] { return resource_->IsLoaded(); }));

  // Act
  ads_client_notifier_.NotifyDidUnregisterResourceComponent(
      test::kInvalidCountryComponentId);

  // Assert
  EXPECT_TRUE(resource_->IsLoaded());
}

}  // namespace brave_ads
