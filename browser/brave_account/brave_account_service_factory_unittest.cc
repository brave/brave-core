/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/brave_account_service_factory.h"

#include <string>

#include "base/check_deref.h"
#include "base/notreached.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_account/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {
namespace {

enum class ProfileKind {
  kRegularOriginal,
  kRegularOTR,
  kGuestOriginal,
  kGuestOTR,
#if !BUILDFLAG(IS_ANDROID)
  kSystemOriginal,
  kSystemOTR,
#endif  // !BUILDFLAG(IS_ANDROID)
};

}  // namespace

class BraveAccountServiceFactoryTest
    : public testing::TestWithParam<ProfileKind> {
 protected:
  void SetUp() override { ASSERT_TRUE(manager_.SetUp()); }

  void TearDown() override { manager_.DeleteAllTestingProfiles(); }

  Profile* GetProfileForKind(ProfileKind kind) {
    switch (kind) {
      case ProfileKind::kRegularOriginal:
        return manager_.CreateTestingProfile(
            "testing", TestingProfile::TestingFactory(
                           BraveAccountServiceFactory::GetInstance(),
                           BraveAccountServiceFactory::GetDefaultFactory()));
      case ProfileKind::kRegularOTR:
        return CHECK_DEREF(manager_.CreateTestingProfile("testing"))
            .GetPrimaryOTRProfile(true);
      case ProfileKind::kGuestOriginal:
        return manager_.CreateGuestProfile();
      case ProfileKind::kGuestOTR:
        return CHECK_DEREF(manager_.CreateGuestProfile())
            .GetPrimaryOTRProfile(true);
#if !BUILDFLAG(IS_ANDROID)
      case ProfileKind::kSystemOriginal:
        return manager_.CreateSystemProfile();
      case ProfileKind::kSystemOTR:
        return CHECK_DEREF(manager_.CreateSystemProfile())
            .GetPrimaryOTRProfile(true);
#endif  // !BUILDFLAG(IS_ANDROID)
    }

    NOTREACHED();
  }

  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager manager_{TestingBrowserProcess::GetGlobal()};
  base::test::ScopedFeatureList scoped_feature_list_{
      brave_account::features::kBraveAccount};
};

constexpr ProfileKind kTestCases[] = {
    ProfileKind::kRegularOriginal, ProfileKind::kRegularOTR,
    ProfileKind::kGuestOriginal,   ProfileKind::kGuestOTR,
#if !BUILDFLAG(IS_ANDROID)
    ProfileKind::kSystemOriginal,  ProfileKind::kSystemOTR,
#endif
};

TEST_P(BraveAccountServiceFactoryTest,
       GetFor_ServiceIsRestrictedToRegularOriginalProfile) {
  ProfileKind kind = GetParam();
  Profile* profile = GetProfileForKind(kind);
  ASSERT_TRUE(profile);
  EXPECT_EQ(BraveAccountServiceFactory::GetFor(profile) != nullptr,
            kind == ProfileKind::kRegularOriginal);
}

INSTANTIATE_TEST_SUITE_P(BraveAccountServiceFactoryTestCases,
                         BraveAccountServiceFactoryTest,
                         testing::ValuesIn(kTestCases),
                         [](const auto& info) -> std::string {
                           switch (info.param) {
                             case ProfileKind::kRegularOriginal:
                               return "RegularOriginal";
                             case ProfileKind::kRegularOTR:
                               return "RegularOTR";
                             case ProfileKind::kGuestOriginal:
                               return "GuestOriginal";
                             case ProfileKind::kGuestOTR:
                               return "GuestOTR";
#if !BUILDFLAG(IS_ANDROID)
                             case ProfileKind::kSystemOriginal:
                               return "SystemOriginal";
                             case ProfileKind::kSystemOTR:
                               return "SystemOTR";
#endif  // !BUILDFLAG(IS_ANDROID)
                           }

                           NOTREACHED();
                         });

}  // namespace brave_account
