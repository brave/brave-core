/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"
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
  MOCK_METHOD3(OnFetchPromotions,
               void(RewardsService*,
                    const mojom::Result result,
                    const std::vector<mojom::PromotionPtr>& list));
  MOCK_METHOD3(OnPromotionFinished,
               void(RewardsService*, const mojom::Result, mojom::PromotionPtr));
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
  MOCK_METHOD2(OnAdsEnabled, void(RewardsService*, bool));
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
#if BUILDFLAG(ENABLE_GREASELION)
    auto* rewards_ = new RewardsServiceImpl(profile(), nullptr);
#else
    auto* rewards_ = new RewardsServiceImpl(profile());
#endif
    RewardsServiceFactory::SetServiceForTesting(std::move(rewards_));
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile()));
    ASSERT_TRUE(RewardsServiceFactory::GetInstance());
    ASSERT_TRUE(rewards_service());
    observer_ = std::make_unique<MockRewardsServiceObserver>();
    rewards_service_->AddObserver(observer_.get());
  }

  void TearDown() override {
    TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);
    rewards_service_->RemoveObserver(observer_.get());
    delete rewards_service_;
    profile_.reset();
  }

  Profile* profile() { return profile_.get(); }
  RewardsServiceImpl* rewards_service() { return rewards_service_; }
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
  raw_ptr<RewardsServiceImpl> rewards_service_ = nullptr;
  std::unique_ptr<MockRewardsServiceObserver> observer_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;
};

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
TEST_F(RewardsServiceTest, GetExternalWallet) {
  DisableGemini();
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            core::constant::kWalletUphold);
}

TEST_F(RewardsServiceTest, GetExternalWalletMultipleCustodians) {
  EnableGemini();
  EXPECT_EQ(rewards_service()->GetExternalWalletType(), "uphold");

  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   "bad-provider-name");
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            core::constant::kWalletUphold);

  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   core::constant::kWalletUphold);
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            core::constant::kWalletUphold);

  profile()->GetPrefs()->SetString(prefs::kExternalWalletType,
                                   core::constant::kWalletGemini);
  EXPECT_EQ(rewards_service()->GetExternalWalletType(),
            core::constant::kWalletGemini);
}
#endif

}  // namespace brave_rewards
