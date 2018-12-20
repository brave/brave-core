/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsServiceTest.*

using namespace brave_rewards;
using ::testing::_;

class MockRewardsServiceObserver : public RewardsServiceObserver {
 public:
  MockRewardsServiceObserver() {}
  MOCK_METHOD2(OnWalletInitialized, void(RewardsService*, int));
  MOCK_METHOD3(OnWalletProperties, void(RewardsService*,
      int,
      std::unique_ptr<brave_rewards::WalletProperties>));
  MOCK_METHOD3(OnGrant,
      void(RewardsService*, unsigned int, brave_rewards::Grant));
  MOCK_METHOD3(OnGrantCaptcha,
      void(RewardsService*, std::string, std::string));
  MOCK_METHOD4(OnRecoverWallet, void(RewardsService*,
                                     unsigned int,
                                     double,
                                     std::vector<brave_rewards::Grant>));
  MOCK_METHOD3(OnGrantFinish,
      void(RewardsService*, unsigned int, brave_rewards::Grant));
  MOCK_METHOD1(OnContentSiteUpdated, void(RewardsService*));
  MOCK_METHOD2(OnExcludedSitesChanged, void(RewardsService*, std::string));
  MOCK_METHOD5(OnReconcileComplete, void(RewardsService*,
                                         unsigned int,
                                         const std::string&,
                                         const std::string&,
                                         const std::string&));
  MOCK_METHOD2(OnRecurringDonationUpdated,
      void(RewardsService*, brave_rewards::ContentSiteList));
  MOCK_METHOD2(OnCurrentTips,
      void(RewardsService*, brave_rewards::ContentSiteList));
  MOCK_METHOD2(OnPublisherBanner,
      void(RewardsService*, const brave_rewards::PublisherBanner));
  MOCK_METHOD4(OnGetPublisherActivityFromUrl,
      void(RewardsService*, int, ledger::PublisherInfo*, uint64_t));
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
        RewardsServiceFactory::GetInstance()->GetForProfile(profile()));
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
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<Profile> profile_;
  RewardsServiceImpl* rewards_service_;
  std::unique_ptr<MockRewardsServiceObserver> observer_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(RewardsServiceTest, HandleFlags) {
  // Staging - true
  ledger::is_production = true;
  ASSERT_TRUE(ledger::is_production);
  rewards_service()->HandleFlags("staging=true");
  ASSERT_FALSE(ledger::is_production);

  // Staging - 1
  ledger::is_production = true;
  ASSERT_TRUE(ledger::is_production);
  RewardsServiceImpl::HandleFlags("staging=1");
  ASSERT_FALSE(ledger::is_production);

  // Staging - false
  ledger::is_production = true;
  ASSERT_TRUE(ledger::is_production);
  RewardsServiceImpl::HandleFlags("staging=false");
  ASSERT_TRUE(ledger::is_production);

  // Staging - random
  ledger::is_production = true;
  ASSERT_TRUE(ledger::is_production);
  RewardsServiceImpl::HandleFlags("staging=werwe");
  ASSERT_TRUE(ledger::is_production);

  // Reconcile interval - positive number
  ledger::reconcile_time = 0;
  ASSERT_EQ(ledger::reconcile_time, 0);
  RewardsServiceImpl::HandleFlags("reconcile-interval=10");
  ASSERT_EQ(ledger::reconcile_time, 10);

  // Reconcile interval - negative number
  ledger::reconcile_time = 0;
  ASSERT_EQ(ledger::reconcile_time, 0);
  RewardsServiceImpl::HandleFlags("reconcile-interval=-1");
  ASSERT_EQ(ledger::reconcile_time, 0);

  // Reconcile interval - string
  ledger::reconcile_time = 0;
  ASSERT_EQ(ledger::reconcile_time, 0);
  RewardsServiceImpl::HandleFlags("reconcile-interval=sdf");
  ASSERT_EQ(ledger::reconcile_time, 0);

  // Short retries - on
  ledger::short_retries = false;
  ASSERT_FALSE(ledger::short_retries);
  RewardsServiceImpl::HandleFlags("short-retries=true");
  ASSERT_TRUE(ledger::short_retries);

  // Short retries - off
  ledger::short_retries = true;
  ASSERT_TRUE(ledger::short_retries);
  RewardsServiceImpl::HandleFlags("short-retries=false");
  ASSERT_FALSE(ledger::short_retries);

  // Mixture of flags
  ASSERT_FALSE(ledger::short_retries);
  ASSERT_TRUE(ledger::is_production);
  ASSERT_EQ(ledger::reconcile_time, 0);
  RewardsServiceImpl::HandleFlags(
      "staging=true,short-retries=true,reconcile-interval=10");
  ASSERT_TRUE(ledger::short_retries);
  ASSERT_FALSE(ledger::is_production);
  ASSERT_EQ(ledger::reconcile_time, 10);

  // Wrong input
  ledger::short_retries = false;
  ledger::reconcile_time = 0;
  ledger::is_production = true;
  ASSERT_FALSE(ledger::short_retries);
  ASSERT_TRUE(ledger::is_production);
  ASSERT_EQ(ledger::reconcile_time, 0);
  RewardsServiceImpl::HandleFlags(
      "staging=,shortretries=true,reconcile-interval");
  ASSERT_FALSE(ledger::short_retries);
  ASSERT_TRUE(ledger::is_production);
  ASSERT_EQ(ledger::reconcile_time, 0);
}

TEST_F(RewardsServiceTest, OnWalletProperties) {
  // We always need to call observer as we report errors back even when we have null pointer
  EXPECT_CALL(*observer(), OnWalletProperties(_, 1, _)).Times(1);
  rewards_service()->OnWalletProperties(ledger::Result::LEDGER_ERROR, nullptr);
}

// add test for strange entries
