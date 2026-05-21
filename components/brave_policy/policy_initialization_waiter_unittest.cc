/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_policy/policy_initialization_waiter.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "components/policy/core/common/mock_policy_service.h"
#include "components/policy/core/common/policy_service.h"
#include "components/policy/core/common/policy_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_policy {

using ::testing::_;
using ::testing::Return;

class PolicyInitializationWaiterTest : public ::testing::Test {
 protected:
  // Captures the registered Observer so the test can drive
  // `OnPolicyServiceInitialized` directly.
  void CaptureObserver() {
    EXPECT_CALL(mock_, AddObserver(policy::POLICY_DOMAIN_CHROME, _))
        .WillOnce([this](policy::PolicyDomain,
                         policy::PolicyService::Observer* observer) {
          observer_ = observer;
        });
    EXPECT_CALL(mock_, RemoveObserver(policy::POLICY_DOMAIN_CHROME, _))
        .WillOnce([this](policy::PolicyDomain,
                         policy::PolicyService::Observer* observer) {
          EXPECT_EQ(observer, observer_);
          observer_ = nullptr;
        });
  }

  base::test::TaskEnvironment task_environment_;
  policy::MockPolicyService mock_;
  raw_ptr<policy::PolicyService::Observer> observer_ = nullptr;
};

TEST_F(PolicyInitializationWaiterTest, NullServiceFiresSynchronously) {
  bool fired = false;
  PolicyInitializationWaiter waiter(/*policy_service=*/nullptr);
  waiter.Wait(base::BindOnce([](bool* fired) { *fired = true; }, &fired));
  EXPECT_TRUE(fired);
}

TEST_F(PolicyInitializationWaiterTest, AlreadyInitializedFiresSynchronously) {
  EXPECT_CALL(mock_, IsInitializationComplete(policy::POLICY_DOMAIN_CHROME))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_, AddObserver(_, _)).Times(0);

  PolicyInitializationWaiter waiter(&mock_);
  bool fired = false;
  waiter.Wait(base::BindOnce([](bool* fired) { *fired = true; }, &fired));
  EXPECT_TRUE(fired);
}

TEST_F(PolicyInitializationWaiterTest, DefersUntilInitialized) {
  EXPECT_CALL(mock_, IsInitializationComplete(policy::POLICY_DOMAIN_CHROME))
      .WillOnce(Return(false));
  CaptureObserver();

  PolicyInitializationWaiter waiter(&mock_);
  bool fired = false;
  waiter.Wait(base::BindOnce([](bool* fired) { *fired = true; }, &fired));
  EXPECT_FALSE(fired);

  ASSERT_TRUE(observer_);
  observer_->OnPolicyServiceInitialized(policy::POLICY_DOMAIN_CHROME);
  EXPECT_FALSE(fired);
  EXPECT_TRUE(base::test::RunUntil([&] { return fired; }));
}

TEST_F(PolicyInitializationWaiterTest, IgnoresWrongDomain) {
  EXPECT_CALL(mock_, IsInitializationComplete(policy::POLICY_DOMAIN_CHROME))
      .WillOnce(Return(false));
  CaptureObserver();

  PolicyInitializationWaiter waiter(&mock_);
  bool fired = false;
  waiter.Wait(base::BindOnce([](bool* fired) { *fired = true; }, &fired));

  ASSERT_TRUE(observer_);
  observer_->OnPolicyServiceInitialized(policy::POLICY_DOMAIN_EXTENSIONS);
  EXPECT_FALSE(fired);
}

TEST_F(PolicyInitializationWaiterTest, DestructionCancelsPendingCallback) {
  EXPECT_CALL(mock_, IsInitializationComplete(policy::POLICY_DOMAIN_CHROME))
      .WillOnce(Return(false));
  CaptureObserver();

  bool fired = false;
  {
    PolicyInitializationWaiter waiter(&mock_);
    waiter.Wait(base::BindOnce([](bool* fired) { *fired = true; }, &fired));
  }
  EXPECT_FALSE(fired);
}

TEST_F(PolicyInitializationWaiterTest, SecondWaitReplacesCallback) {
  EXPECT_CALL(mock_, IsInitializationComplete(policy::POLICY_DOMAIN_CHROME))
      .WillOnce(Return(false));
  CaptureObserver();

  PolicyInitializationWaiter waiter(&mock_);
  bool first_fired = false;
  bool second_fired = false;
  waiter.Wait(base::BindOnce([](bool* fired) { *fired = true; }, &first_fired));
  waiter.Wait(
      base::BindOnce([](bool* fired) { *fired = true; }, &second_fired));

  ASSERT_TRUE(observer_);
  observer_->OnPolicyServiceInitialized(policy::POLICY_DOMAIN_CHROME);
  EXPECT_FALSE(first_fired);
  EXPECT_FALSE(second_fired);
  EXPECT_TRUE(base::test::RunUntil([&] { return second_fired; }));
  EXPECT_FALSE(first_fired);
}

}  // namespace brave_policy
