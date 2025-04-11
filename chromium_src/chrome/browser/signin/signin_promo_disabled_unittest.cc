/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/signin/chrome_signin_pref_names.h"
#include "chrome/browser/signin/identity_test_environment_profile_adaptor.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/signin/signin_promo_util.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "components/autofill/core/browser/test_utils/test_profiles.h"
#include "components/signin/public/base/signin_switches.h"
#include "components/sync/test/mock_sync_service.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/common/extension_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace signin {

class ShowPromoTest : public testing::Test {
 public:
  ShowPromoTest() {
    TestingProfile::Builder profile_builder;
    profile_builder.AddTestingFactory(
        SyncServiceFactory::GetInstance(),
        base::BindRepeating([](content::BrowserContext* context) {
          return static_cast<std::unique_ptr<KeyedService>>(
              std::make_unique<syncer::MockSyncService>());
        }));
    profile_ = IdentityTestEnvironmentProfileAdaptor::
        CreateProfileForIdentityTestEnvironment(profile_builder);

    identity_test_env_adaptor_ =
        std::make_unique<IdentityTestEnvironmentProfileAdaptor>(profile_.get());
  }

  syncer::MockSyncService* sync_service() {
    return static_cast<syncer::MockSyncService*>(
        SyncServiceFactory::GetForProfile(profile()));
  }

  IdentityManager* identity_manager() {
    return identity_test_env_adaptor_->identity_test_env()->identity_manager();
  }

  TestingProfile* profile() { return profile_.get(); }

  const extensions::Extension* CreateExtension(
      extensions::mojom::ManifestLocation location =
          extensions::mojom::ManifestLocation::kInternal) {
    extension_ = extensions::ExtensionBuilder()
                     .SetManifest(base::Value::Dict()
                                      .Set("name", "test")
                                      .Set("manifest_version", 2)
                                      .Set("version", "1.0.0"))
                     .SetLocation(location)
                     .Build();

    return extension_.get();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<IdentityTestEnvironmentProfileAdaptor>
      identity_test_env_adaptor_;
  scoped_refptr<const extensions::Extension> extension_;
};

#if BUILDFLAG(ENABLE_DICE_SUPPORT)

class ShowSigninPromoTestWithFeatureFlagsIsDisabled : public ShowPromoTest {
 public:
  void SetUp() override {
    ShowPromoTest::SetUp();
    feature_list_.InitWithFeatures(
        /*enabled_features=*/
        {switches::kImprovedSigninUIOnDesktop,
         switches::kSyncEnableBookmarksInTransportMode,
         switches::kEnableExtensionsExplicitBrowserSignin},
        /*disabled_features=*/{});
    ON_CALL(*sync_service(), GetDataTypesForTransportOnlyMode())
        .WillByDefault(testing::Return(syncer::DataTypeSet::All()));
  }

  GaiaId gaia_id() {
    return identity_manager()
        ->GetPrimaryAccountInfo(ConsentLevel::kSignin)
        .gaia;
  }

  autofill::AutofillProfile CreateAddress(
      const std::string& country_code = "US") {
    return autofill::test::StandardProfile(AddressCountryCode(country_code));
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled, ShowPromoWithNoAccount) {
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled,
       ShowPromoWithWebSignedInAccount) {
  MakeAccountAvailable(identity_manager(), "test@email.com");
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled,
       ShowPromoWithSignInPendingAccount) {
  AccountInfo info = MakePrimaryAccountAvailable(
      identity_manager(), "test@email.com", ConsentLevel::kSignin);
  signin::SetInvalidRefreshTokenForPrimaryAccount(identity_manager());
  EXPECT_FALSE(ShouldShowPasswordSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled, DoNotShowAddressPromo) {
  ASSERT_FALSE(ShouldShowAddressSignInPromo(*profile(), CreateAddress()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled, DoNotShowBookmarkPromo) {
  ASSERT_FALSE(ShouldShowBookmarkSignInPromo(*profile()));
}

TEST_F(ShowSigninPromoTestWithFeatureFlagsIsDisabled,
       ShowExtensionsPromoWithNoAccount) {
  EXPECT_FALSE(ShouldShowExtensionSignInPromo(*profile(), *CreateExtension()));
}

#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

}  // namespace signin
