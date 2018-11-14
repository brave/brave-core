/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/files/scoped_temp_dir.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=RewardsServiceTest.*

using namespace brave_rewards;

class RewardsServiceTest : public testing::Test {
 public:
  RewardsServiceTest() :
      thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
  }
  ~RewardsServiceTest() override {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    profile_ = CreateBraveRewardsProfile(temp_dir_.GetPath());
    ASSERT_TRUE(profile_.get() != NULL);


    rewards_service_ = static_cast<RewardsServiceImpl*>(
        RewardsServiceFactory::GetInstance()->GetForProfile(profile()));

    ASSERT_TRUE(rewards_service_ != NULL);
  }

  void TearDown() override {
    profile_.reset();
  }


  Profile* profile() { return profile_.get(); }
  RewardsServiceImpl* rewards_service() { return rewards_service_; }

 private:
  std::unique_ptr<Profile> profile_;
  content::TestBrowserThreadBundle thread_bundle_;
  RewardsServiceImpl* rewards_service_;
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
  rewards_service()->HandleFlags("staging=1");
  ASSERT_FALSE(ledger::is_production);

  // Staging - false
  ledger::is_production = true;
  ASSERT_TRUE(ledger::is_production);
  rewards_service()->HandleFlags("staging=false");
  ASSERT_TRUE(ledger::is_production);

  // Staging - random
  ledger::is_production = true;
  ASSERT_TRUE(ledger::is_production);
  rewards_service()->HandleFlags("staging=werwe");
  ASSERT_TRUE(ledger::is_production);

  // Reconcile interval - positive number
  ledger::reconcile_time = 0;
  ASSERT_EQ(ledger::reconcile_time, 0);
  rewards_service()->HandleFlags("reconcile-interval=10");
  ASSERT_EQ(ledger::reconcile_time, 10);

  // Reconcile interval - negative number
  ledger::reconcile_time = 0;
  ASSERT_EQ(ledger::reconcile_time, 0);
  rewards_service()->HandleFlags("reconcile-interval=-1");
  ASSERT_EQ(ledger::reconcile_time, 0);

  // Reconcile interval - string
  ledger::reconcile_time = 0;
  ASSERT_EQ(ledger::reconcile_time, 0);
  rewards_service()->HandleFlags("reconcile-interval=sdf");
  ASSERT_EQ(ledger::reconcile_time, 0);

  // Short retries - on
  ledger::short_retries = false;
  ASSERT_FALSE(ledger::short_retries);
  rewards_service()->HandleFlags("short-retries=true");
  ASSERT_TRUE(ledger::short_retries);

  // Short retries - off
  ledger::short_retries = true;
  ASSERT_TRUE(ledger::short_retries);
  rewards_service()->HandleFlags("short-retries=false");
  ASSERT_FALSE(ledger::short_retries);

  // Mixture of flags
  ASSERT_FALSE(ledger::short_retries);
  ASSERT_TRUE(ledger::is_production);
  ASSERT_EQ(ledger::reconcile_time, 0);
  rewards_service()->HandleFlags(
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
  rewards_service()->HandleFlags(
      "staging=,shortretries=true,reconcile-interval");
  ASSERT_FALSE(ledger::short_retries);
  ASSERT_TRUE(ledger::is_production);
  ASSERT_EQ(ledger::reconcile_time, 0);
}


// add test for strange entries