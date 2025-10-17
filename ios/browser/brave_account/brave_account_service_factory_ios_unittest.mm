/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_service_factory_ios.h"

#include <string>

#include "base/check_deref.h"
#include "base/notreached.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_account/features.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/test/test_profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/test/test_profile_manager_ios.h"
#include "ios/chrome/test/ios_chrome_scoped_testing_local_state.h"
#include "ios/web/public/test/web_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {
namespace {

enum class ProfileKind {
  kRegularOriginal,
  kRegularOTR,
};

}  // namespace

class BraveAccountServiceFactoryIOSTest
    : public testing::TestWithParam<ProfileKind> {
 protected:
  ProfileIOS* GetProfileForKind(ProfileKind kind) {
    TestProfileIOS::Builder builder;
    builder.AddTestingFactory(
        BraveAccountServiceFactoryIOS::GetInstance(),
        BraveAccountServiceFactoryIOS::GetDefaultFactory());

    switch (kind) {
      case ProfileKind::kRegularOriginal:
        return profile_manager_.AddProfileWithBuilder(std::move(builder));
      case ProfileKind::kRegularOTR:
        return CHECK_DEREF(
                   profile_manager_.AddProfileWithBuilder(std::move(builder)))
            .GetOffTheRecordProfile();
    }

    NOTREACHED();
  }

  web::WebTaskEnvironment task_environment_;
  IOSChromeScopedTestingLocalState scoped_testing_local_state_;
  TestProfileManagerIOS profile_manager_;
  base::test::ScopedFeatureList scoped_feature_list_{
      brave_account::features::kBraveAccount};
};

constexpr ProfileKind kTestCases[] = {
    ProfileKind::kRegularOriginal,
    ProfileKind::kRegularOTR,
};

TEST_P(BraveAccountServiceFactoryIOSTest,
       GetFor_ServiceIsRestrictedToRegularOriginalProfile) {
  ProfileKind kind = GetParam();
  ProfileIOS* profile = GetProfileForKind(kind);
  ASSERT_TRUE(profile);
  EXPECT_EQ(BraveAccountServiceFactoryIOS::GetFor(profile) != nullptr,
            kind == ProfileKind::kRegularOriginal);
}

INSTANTIATE_TEST_SUITE_P(BraveAccountServiceFactoryIOSTestCases,
                         BraveAccountServiceFactoryIOSTest,
                         testing::ValuesIn(kTestCases),
                         [](const auto& info) -> std::string {
                           switch (info.param) {
                             case ProfileKind::kRegularOriginal:
                               return "RegularOriginal";
                             case ProfileKind::kRegularOTR:
                               return "RegularOTR";
                           }

                           NOTREACHED();
                         });

}  // namespace brave_account
