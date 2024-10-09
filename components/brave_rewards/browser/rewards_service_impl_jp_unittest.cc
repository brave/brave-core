/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsServiceJPTest.*

namespace brave_rewards {

using ::testing::NiceMock;
using ::testing::Return;

class RewardsServiceJPTest : public testing::Test {
 public:
  RewardsServiceJPTest() = default;
  ~RewardsServiceJPTest() override = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>("ja_JP");
    profile_ = CreateBraveRewardsProfile(temp_dir_.GetPath());
    ASSERT_TRUE(profile_);
    rewards_service_ = std::make_unique<RewardsServiceImpl>(
        profile()->GetPrefs(), profile()->GetPath(), nullptr,
        base::RepeatingCallback<int(
            const GURL& url, base::OnceCallback<void(const SkBitmap& bitmap)>,
            const net::NetworkTrafficAnnotationTag& traffic_annotation)>(),
        base::RepeatingCallback<void(int)>(),
        profile()->GetDefaultStoragePartition(),
#if BUILDFLAG(ENABLE_GREASELION)
        nullptr,
#endif
        nullptr);
    ASSERT_TRUE(rewards_service());

    profile()->GetPrefs()->SetString(prefs::kDeclaredGeo, "JP");
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
    rewards_service_ = nullptr;
    profile_.reset();
  }

  Profile* profile() { return profile_.get(); }
  RewardsServiceImpl* rewards_service() { return rewards_service_.get(); }

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
  void EnableGemini() {
    feature_list_.InitAndEnableFeature(features::kGeminiFeature);
  }

  void DisableGemini() {
    feature_list_.InitAndDisableFeature(features::kGeminiFeature);
  }
#endif

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<Profile> profile_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;
  std::unique_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
};

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
TEST_F(RewardsServiceJPTest, GetExternalWallet) {
  DisableGemini();
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletBitflyer);
}

TEST_F(RewardsServiceJPTest, GetExternalWalletMultipleCustodians) {
  EnableGemini();
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletBitflyer);
  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   internal::constant::kWalletUphold);
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletBitflyer);
  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   internal::constant::kWalletGemini);
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletBitflyer);
}
#endif

}  // namespace brave_rewards
