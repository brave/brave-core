/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_browser_tests --filter=RewardsFlagBrowserTest.*

namespace rewards_browsertest {

class RewardsFlagBrowserTest : public InProcessBrowserTest {
 public:
  RewardsFlagBrowserTest() {
    observer_ = std::make_unique<RewardsBrowserTestObserver>();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    // Rewards service
    brave::RegisterPathProvider();
    auto* profile = browser()->profile();
    rewards_service_ = static_cast<brave_rewards::RewardsServiceImpl*>(
        brave_rewards::RewardsServiceFactory::GetForProfile(profile));

    // Observer
    observer_->Initialize(rewards_service_);
    if (!rewards_service_->IsWalletInitialized()) {
      observer_->WaitForWalletInitialization();
    }
    rewards_service_->SetLedgerEnvForTesting();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void GetReconcileInterval() {
    rewards_service_->GetReconcileInterval(
        base::Bind(&RewardsFlagBrowserTest::OnGetReconcileInterval,
          base::Unretained(this)));
  }

  void GetShortRetries() {
    rewards_service_->GetShortRetries(
        base::Bind(&RewardsFlagBrowserTest::OnGetShortRetries,
          base::Unretained(this)));
  }

  void GetEnvironment() {
    rewards_service_->GetEnvironment(
        base::Bind(&RewardsFlagBrowserTest::OnGetEnvironment,
          base::Unretained(this)));
  }

  void GetDebug() {
    rewards_service_->GetDebug(
        base::Bind(&RewardsFlagBrowserTest::OnGetDebug,
          base::Unretained(this)));
  }

  MOCK_METHOD1(OnGetEnvironment, void(ledger::Environment));
  MOCK_METHOD1(OnGetDebug, void(bool));
  MOCK_METHOD1(OnGetReconcileInterval, void(int32_t));
  MOCK_METHOD1(OnGetShortRetries, void(bool));

  brave_rewards::RewardsServiceImpl* rewards_service_;
  std::unique_ptr<RewardsBrowserTestObserver> observer_;
};

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsSingleArg) {
  testing::InSequence s;
  // SetEnvironment(ledger::Environment::PRODUCTION)
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::PRODUCTION));
  // Staging - true and 1
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::STAGING)).Times(2);
  // Staging - false and random
  EXPECT_CALL(*this, OnGetEnvironment(
      ledger::Environment::PRODUCTION)).Times(2);

  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Staging - true
  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->HandleFlags("staging=true");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Staging - 1
  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->HandleFlags("staging=1");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Staging - false
  rewards_service_->SetEnvironment(ledger::Environment::STAGING);
  rewards_service_->HandleFlags("staging=false");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Staging - random
  rewards_service_->SetEnvironment(ledger::Environment::STAGING);
  rewards_service_->HandleFlags("staging=werwe");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // SetDebug(true)
  EXPECT_CALL(*this, OnGetDebug(true));
  // Debug - true and 1
  EXPECT_CALL(*this, OnGetDebug(true)).Times(2);
  // Debug - false and random
  EXPECT_CALL(*this, OnGetDebug(false)).Times(2);

  rewards_service_->SetDebug(true);
  GetDebug();
  rewards_browsertest_util::RunUntilIdle();

  // Debug - true
  rewards_service_->SetDebug(false);
  rewards_service_->HandleFlags("debug=true");
  GetDebug();
  rewards_browsertest_util::RunUntilIdle();

  // Debug - 1
  rewards_service_->SetDebug(false);
  rewards_service_->HandleFlags("debug=1");
  GetDebug();
  rewards_browsertest_util::RunUntilIdle();

  // Debug - false
  rewards_service_->SetDebug(true);
  rewards_service_->HandleFlags("debug=false");
  GetDebug();
  rewards_browsertest_util::RunUntilIdle();

  // Debug - random
  rewards_service_->SetDebug(true);
  rewards_service_->HandleFlags("debug=werwe");
  GetDebug();
  rewards_browsertest_util::RunUntilIdle();

  // SetEnvironment(ledger::Environment::PRODUCTION)
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::PRODUCTION));
  // Development - true and 1
  EXPECT_CALL(
      *this,
      OnGetEnvironment(ledger::Environment::DEVELOPMENT)).Times(2);
  // Development - false and random
  EXPECT_CALL(
      *this,
      OnGetEnvironment(ledger::Environment::PRODUCTION)).Times(2);

  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Development - true
  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=true");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Development - 1
  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=1");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Development - false
  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=false");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // Development - random
  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->HandleFlags("development=werwe");
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();

  // positive number
  EXPECT_CALL(*this, OnGetReconcileInterval(10));
  // negative number and string
  EXPECT_CALL(*this, OnGetReconcileInterval(0)).Times(2);

  // Reconcile interval - positive number
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->HandleFlags("reconcile-interval=10");
  GetReconcileInterval();
  rewards_browsertest_util::RunUntilIdle();

  // Reconcile interval - negative number
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->HandleFlags("reconcile-interval=-1");
  GetReconcileInterval();
  rewards_browsertest_util::RunUntilIdle();

  // Reconcile interval - string
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->HandleFlags("reconcile-interval=sdf");
  GetReconcileInterval();
  rewards_browsertest_util::RunUntilIdle();

  EXPECT_CALL(*this, OnGetShortRetries(true));   // on
  EXPECT_CALL(*this, OnGetShortRetries(false));  // off

  // Short retries - on
  rewards_service_->SetShortRetries(false);
  rewards_service_->HandleFlags("short-retries=true");
  GetShortRetries();
  rewards_browsertest_util::RunUntilIdle();

  // Short retries - off
  rewards_service_->SetShortRetries(true);
  rewards_service_->HandleFlags("short-retries=false");
  GetShortRetries();
  rewards_browsertest_util::RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsMultipleFlags) {
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::STAGING));
  EXPECT_CALL(*this, OnGetDebug(true));
  EXPECT_CALL(*this, OnGetReconcileInterval(10));
  EXPECT_CALL(*this, OnGetShortRetries(true));

  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->SetDebug(true);
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->SetShortRetries(false);

  rewards_service_->HandleFlags(
      "staging=true,debug=true,short-retries=true,reconcile-interval=10");

  GetReconcileInterval();
  GetShortRetries();
  GetEnvironment();
  GetDebug();
  rewards_browsertest_util::RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(RewardsFlagBrowserTest, HandleFlagsWrongInput) {
  EXPECT_CALL(*this, OnGetEnvironment(ledger::Environment::PRODUCTION));
  EXPECT_CALL(*this, OnGetDebug(false));
  EXPECT_CALL(*this, OnGetReconcileInterval(0));
  EXPECT_CALL(*this, OnGetShortRetries(false));

  rewards_service_->SetEnvironment(ledger::Environment::PRODUCTION);
  rewards_service_->SetDebug(false);
  rewards_service_->SetReconcileInterval(0);
  rewards_service_->SetShortRetries(false);

  rewards_service_->HandleFlags(
      "staging=,debug=,shortretries=true,reconcile-interval");

  GetReconcileInterval();
  GetShortRetries();
  GetDebug();
  GetEnvironment();
  rewards_browsertest_util::RunUntilIdle();
}

}  // namespace rewards_browsertest
