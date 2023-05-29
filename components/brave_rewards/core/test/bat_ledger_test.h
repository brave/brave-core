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
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::internal {

// Base class for unit tests. |BATLedgerTest| provides a task environment and a
// test implementation of |LedgerClient|.
class BATLedgerTest : public testing::Test {
 public:
  BATLedgerTest();
  ~BATLedgerTest() override;

  void InitializeLedger();

 protected:
  // Adds a mock network response for the specified URL and HTTP method.
  void AddNetworkResultForTesting(const std::string& url,
                                  mojom::UrlMethod method,
                                  mojom::UrlResponsePtr response);

  // Sets a callback that is executed when a message is logged to the client.
  void SetLogCallbackForTesting(TestLedgerClient::LogCallback callback);

  base::test::TaskEnvironment task_environment_;
  TestLedgerClient client_;

 private:
  mojo::AssociatedReceiver<mojom::LedgerClient> client_receiver_{&client_};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_TEST_BAT_LEDGER_TEST_H_
