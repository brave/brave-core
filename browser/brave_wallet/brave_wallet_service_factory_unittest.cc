/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"

#include <memory>

#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/browser/tor/tor_profile_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class BraveWalletServiceFactoryTest : public testing::Test {
 public:
  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
  CreatePrefsService() {
    auto pref_service =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(pref_service->registry());
    return pref_service;
  }

  std::unique_ptr<TestingProfile> CreateProfile(
      std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
          prefs_service) {
    TestingProfile::Builder profile_builder;
    profile_builder.SetPrefService(std::move(prefs_service));
    return profile_builder.Build();
  }

  std::unique_ptr<TestingProfile> CreateGuestProfile() {
    TestingProfile::Builder profile_builder;
    profile_builder.SetGuestSession();
    return profile_builder.Build();
  }

  std::unique_ptr<TestingProfile> CreateProfile() {
    return TestingProfile::Builder().Build();
  }

 private:
  BraveWalletServiceFactory::NotNullForTesting
      brave_wallet_service_factory_not_null_for_testing_;
  content::BrowserTaskEnvironment task_environment_;

 private:
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveWalletServiceFactoryTest, ServiceCreatedByDefault) {
  auto profile = CreateProfile();

  EXPECT_TRUE(BraveWalletServiceFactory::GetServiceForContext(profile.get()));
  EXPECT_TRUE(IsBraveWalletServiceAvailable(profile.get()));
}

#if BUILDFLAG(ENABLE_TOR)
TEST_F(BraveWalletServiceFactoryTest, NoServiceForTor) {
  auto profile = CreateProfile();
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile.get());

  EXPECT_FALSE(BraveWalletServiceFactory::GetServiceForContext(tor_profile));
  EXPECT_FALSE(IsBraveWalletServiceAvailable(tor_profile));
}

TEST_F(BraveWalletServiceFactoryTest, NoServiceForTorEvenWithIncognitoPref) {
  auto prefs_service = CreatePrefsService();
  prefs_service->SetBoolean(kBraveWalletPrivateWindowsEnabled, true);
  auto profile = CreateProfile(std::move(prefs_service));
  Profile* tor_profile =
      TorProfileManager::GetInstance().GetTorProfile(profile.get());

  EXPECT_FALSE(BraveWalletServiceFactory::GetServiceForContext(tor_profile));
  EXPECT_FALSE(IsBraveWalletServiceAvailable(tor_profile));
}
#endif  // BUILDFLAG(ENABLE_TOR)

TEST_F(BraveWalletServiceFactoryTest, NoServiceForIncognito) {
  auto profile = CreateProfile();
  Profile* otr_profile = profile->GetPrimaryOTRProfile(true);

  EXPECT_FALSE(BraveWalletServiceFactory::GetServiceForContext(otr_profile));
  EXPECT_FALSE(IsBraveWalletServiceAvailable(otr_profile));
}

TEST_F(BraveWalletServiceFactoryTest, ServiceForIncognitoWithPref) {
  auto prefs_service = CreatePrefsService();
  prefs_service->SetBoolean(kBraveWalletPrivateWindowsEnabled, true);
  auto profile = CreateProfile(std::move(prefs_service));
  Profile* otr_profile = profile->GetPrimaryOTRProfile(true);

  EXPECT_TRUE(BraveWalletServiceFactory::GetServiceForContext(otr_profile));
  EXPECT_TRUE(IsBraveWalletServiceAvailable(otr_profile));

  EXPECT_TRUE(BraveWalletServiceFactory::GetServiceForContext(profile.get()));
  EXPECT_TRUE(IsBraveWalletServiceAvailable(profile.get()));

  EXPECT_NE(BraveWalletServiceFactory::GetServiceForContext(otr_profile),
            BraveWalletServiceFactory::GetServiceForContext(profile.get()));
}

TEST_F(BraveWalletServiceFactoryTest, NoServiceWhenDisabledByPolicyIsTrue) {
  auto prefs_service = CreatePrefsService();
  prefs_service->SetManagedPref(brave_wallet::kBraveWalletDisabledByPolicy,
                                base::Value(true));

  auto profile = CreateProfile(std::move(prefs_service));

  EXPECT_FALSE(BraveWalletServiceFactory::GetServiceForContext(profile.get()));
  EXPECT_FALSE(IsBraveWalletServiceAvailable(profile.get()));
}

TEST_F(BraveWalletServiceFactoryTest,
       ServiceAvailableWhenDisabledByPolicyIsFalse) {
  auto prefs_service = CreatePrefsService();
  prefs_service->SetManagedPref(brave_wallet::kBraveWalletDisabledByPolicy,
                                base::Value(false));

  auto profile = CreateProfile(std::move(prefs_service));

  EXPECT_TRUE(BraveWalletServiceFactory::GetServiceForContext(profile.get()));
  EXPECT_TRUE(IsBraveWalletServiceAvailable(profile.get()));
}

TEST_F(BraveWalletServiceFactoryTest,
       ServiceStillAvailableAfterDisabledByPolicy) {
  auto profile = CreateProfile();

  EXPECT_TRUE(BraveWalletServiceFactory::GetServiceForContext(profile.get()));

  profile->GetTestingPrefService()->SetManagedPref(
      brave_wallet::kBraveWalletDisabledByPolicy, base::Value(true));
  EXPECT_TRUE(BraveWalletServiceFactory::GetServiceForContext(profile.get()));
  EXPECT_TRUE(IsBraveWalletServiceAvailable(profile.get()));
}

TEST_F(BraveWalletServiceFactoryTest, NoServiceForGuestProfile) {
  auto guest_profile = CreateGuestProfile();

  EXPECT_FALSE(
      BraveWalletServiceFactory::GetServiceForContext(guest_profile.get()));
  EXPECT_FALSE(IsBraveWalletServiceAvailable(guest_profile.get()));
}

}  // namespace brave_wallet
