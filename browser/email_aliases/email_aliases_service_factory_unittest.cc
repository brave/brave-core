/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/email_aliases/email_aliases_service_factory.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/email_aliases/features.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace email_aliases {

class FakeEmailAliasesServiceObserver
    : public mojom::EmailAliasesServiceObserver {
 public:
  FakeEmailAliasesServiceObserver() = default;
  ~FakeEmailAliasesServiceObserver() override = default;
};

class EmailAliasesServiceFactoryTest : public ::testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};

  void SetUp() override { ASSERT_TRUE(profile_manager_.SetUp()); }
};

TEST_F(EmailAliasesServiceFactoryTest,
       NoServiceWhenBraveAccountFeatureDisabled) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitWithFeatures(
      {features::kEmailAliases}, {brave_account::features::kBraveAccount});
  auto* profile = profile_manager_.CreateTestingProfile("test");
  auto* service = EmailAliasesServiceFactory::GetServiceForProfile(profile);
  EXPECT_EQ(service, nullptr);
}

TEST_F(EmailAliasesServiceFactoryTest, NoServiceWhenFeatureDisabled) {
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitWithFeatures(
      {brave_account::features::kBraveAccount}, {features::kEmailAliases});
  auto* profile = profile_manager_.CreateTestingProfile("test");
  auto* service = EmailAliasesServiceFactory::GetServiceForProfile(profile);
  EXPECT_EQ(service, nullptr);
}

#if !BUILDFLAG(IS_ANDROID)
TEST_F(EmailAliasesServiceFactoryTest, NoServiceForGuestOrSystemProfile) {
  scoped_feature_list_.InitWithFeatures(
      {brave_account::features::kBraveAccount, features::kEmailAliases}, {});
  auto* guest_profile = profile_manager_.CreateGuestProfile();
  auto* system_profile = profile_manager_.CreateSystemProfile();
  auto* service_guest =
      EmailAliasesServiceFactory::GetServiceForProfile(guest_profile);
  auto* service_system =
      EmailAliasesServiceFactory::GetServiceForProfile(system_profile);
  EXPECT_EQ(service_guest, nullptr);
  EXPECT_EQ(service_system, nullptr);
}
#endif

TEST_F(EmailAliasesServiceFactoryTest, SameServiceForRegularAndIncognito) {
  scoped_feature_list_.InitWithFeatures(
      {brave_account::features::kBraveAccount, features::kEmailAliases}, {});
  [[maybe_unused]] auto* profile =
      profile_manager_.CreateTestingProfile("test");
  [[maybe_unused]] auto* incognito =
      profile->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  auto* service_regular =
      EmailAliasesServiceFactory::GetServiceForProfile(profile);
  auto* service_incognito =
      EmailAliasesServiceFactory::GetServiceForProfile(incognito);
  EXPECT_EQ(service_regular, service_incognito);
}

}  // namespace email_aliases
