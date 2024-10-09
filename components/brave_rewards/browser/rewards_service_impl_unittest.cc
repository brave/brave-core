/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
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

// npm run test -- brave_unit_tests --filter=RewardsServiceTest.*

namespace brave_rewards {

using ::testing::NiceMock;
using ::testing::Return;

class MockRewardsServiceObserver : public RewardsServiceObserver {
 public:
  MOCK_METHOD6(OnReconcileComplete,
               void(RewardsService*,
                    const mojom::Result,
                    const std::string&,
                    const double,
                    const mojom::RewardsType,
                    const mojom::ContributionProcessor));
  MOCK_METHOD2(OnGetRecurringTips,
               void(RewardsService*,
                    std::vector<mojom::PublisherInfoPtr> list));
  MOCK_METHOD2(OnPublisherBanner,
               void(RewardsService*, mojom::PublisherBannerPtr banner));
  MOCK_METHOD4(OnPanelPublisherInfo,
               void(RewardsService*,
                    mojom::Result,
                    const mojom::PublisherInfo*,
                    uint64_t));
};

class RewardsServiceTest : public testing::Test {
 public:
  RewardsServiceTest() = default;
  ~RewardsServiceTest() override = default;

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>("en_US");
    profile_ = CreateBraveRewardsProfile(temp_dir_.GetPath());
    ASSERT_TRUE(profile_.get());

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
    observer_ = std::make_unique<MockRewardsServiceObserver>();
    rewards_service_->AddObserver(observer_.get());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
    rewards_service_->RemoveObserver(observer_.get());
    rewards_service_ = nullptr;
    profile_.reset();
  }

  Profile* profile() { return profile_.get(); }
  RewardsServiceImpl* rewards_service() { return rewards_service_.get(); }
  MockRewardsServiceObserver* observer() { return observer_.get(); }

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
  std::unique_ptr<MockRewardsServiceObserver> observer_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;
  std::unique_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
};

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
TEST_F(RewardsServiceTest, GetExternalWallet) {
  DisableGemini();
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletUphold);
}

TEST_F(RewardsServiceTest, GetExternalWalletMultipleCustodians) {
  EnableGemini();
  EXPECT_EQ(rewards_service()->GetExternalWalletType(), "uphold");

  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   "bad-provider-name");
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletUphold);

  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   internal::constant::kWalletUphold);
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletUphold);

  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   internal::constant::kWalletGemini);
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            internal::constant::kWalletGemini);
}
#endif

}  // namespace brave_rewards
