/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/scoped_temp_dir.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsServiceTest.*

namespace brave_rewards {

using ::testing::_;

class MockRewardsServiceObserver : public RewardsServiceObserver {
 public:
  MOCK_METHOD2(OnWalletInitialized,
      void(RewardsService*, const ledger::type::Result));
  MOCK_METHOD3(OnFetchPromotions, void(RewardsService*,
      const ledger::type::Result result,
      const ledger::type::PromotionList& list));
  MOCK_METHOD2(OnRecoverWallet,
      void(RewardsService*, const ledger::type::Result));
  MOCK_METHOD3(OnPromotionFinished, void(
      RewardsService*,
      const ledger::type::Result,
      ledger::type::PromotionPtr));
  MOCK_METHOD6(OnReconcileComplete, void(
      RewardsService*,
      const ledger::type::Result,
      const std::string&,
      const double,
      const ledger::type::RewardsType,
      const ledger::type::ContributionProcessor));
  MOCK_METHOD2(OnGetRecurringTips,
      void(RewardsService*, ledger::type::PublisherInfoList list));
  MOCK_METHOD2(OnPublisherBanner,
      void(RewardsService*, ledger::type::PublisherBannerPtr banner));
  MOCK_METHOD4(OnPanelPublisherInfo,
      void(RewardsService*, int, ledger::type::PublisherInfoPtr, uint64_t));
  MOCK_METHOD2(OnAdsEnabled, void(RewardsService*, bool));
};

class RewardsServiceTest : public testing::Test {
 public:
  RewardsServiceTest() {}
  ~RewardsServiceTest() override {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    profile_ = CreateBraveRewardsProfile(temp_dir_.GetPath());
    ASSERT_TRUE(profile_.get() != NULL);
    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetForProfile(profile()));
    ASSERT_TRUE(RewardsServiceFactory::GetInstance() != NULL);
    ASSERT_TRUE(rewards_service() != NULL);
    observer_.reset(new MockRewardsServiceObserver);
    rewards_service_->AddObserver(observer_.get());
  }

  void TearDown() override {
    rewards_service_->RemoveObserver(observer_.get());
    profile_.reset();
  }

  Profile* profile() { return profile_.get(); }
  RewardsServiceImpl* rewards_service() { return rewards_service_; }
  MockRewardsServiceObserver* observer() { return observer_.get(); }

 private:
  // Need this as a very first member to run tests in UI thread
  // When this is set, class should not install any other MessageLoops, like
  // base::test::ScopedTaskEnvironment
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<Profile> profile_;
  RewardsServiceImpl* rewards_service_;
  std::unique_ptr<MockRewardsServiceObserver> observer_;
  base::ScopedTempDir temp_dir_;
};

// add test for strange entries

}  // namespace brave_rewards
