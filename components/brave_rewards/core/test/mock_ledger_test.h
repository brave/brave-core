/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_MOCK_LEDGER_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_MOCK_LEDGER_TEST_H_

#include <memory>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {

class MockLedgerTest : public testing::Test {
 protected:
  MockLedgerTest() {
    ledger(std::make_unique<MockLedgerImpl>()).SetTesting(true);
  }

  MockLedgerImpl& mock_ledger() {
    return static_cast<MockLedgerImpl&>(ledger());
  }

  base::test::TaskEnvironment task_environment_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_MOCK_LEDGER_TEST_H_
