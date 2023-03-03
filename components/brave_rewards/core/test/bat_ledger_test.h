/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_BAT_LEDGER_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_BAT_LEDGER_TEST_H_

#include <string>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/test/test_ledger_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ledger {

// Base class for unit tests. |BATLedgerTest| provides a task environment and a
// test implementation of |LedgerClient|.
class BATLedgerTest : public testing::Test {
 public:
  BATLedgerTest();
  ~BATLedgerTest() override;

 protected:
  // Returns the |TaskEnvironment| for this test.
  base::test::TaskEnvironment* task_environment() { return &task_environment_; }

  // Returns the |TestLedgerClient| instance for this test.
  TestLedgerClient* GetTestLedgerClient() { return &client_; }

  // Returns the |LedgerImpl| instance for this test.
  LedgerImpl* GetLedgerImpl() { return &ledger_; }

  // Adds a mock network response for the specified URL and HTTP method.
  void AddNetworkResultForTesting(const std::string& url,
                                  mojom::UrlMethod method,
                                  mojom::UrlResponsePtr response);

  // Sets a callback that is executed when a message is logged to the client.
  void SetLogCallbackForTesting(TestLedgerClient::LogCallback callback);

 private:
  base::test::TaskEnvironment task_environment_;
  TestLedgerClient client_;
  LedgerImpl ledger_{&client_};
};

}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_BAT_LEDGER_TEST_H_
