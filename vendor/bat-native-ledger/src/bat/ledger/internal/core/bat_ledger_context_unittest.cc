/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_context.h"

#include <string>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/core/future.h"

namespace ledger {

class BATLedgerContextTest : public BATLedgerTest {};

class TestComponent : public BATLedgerContext::Object {
 public:
  inline static const char kContextKey[] = "test-component";

  TestComponent() = default;
  ~TestComponent() override = default;

  BATLedgerContext& GetContext() { return context(); }
};

TEST_F(BATLedgerContextTest, Get) {
  BATLedgerContext context(GetTestLedgerClient());
  auto& component = context.Get<TestComponent>();
  EXPECT_EQ(&component.GetContext(), &context);
  EXPECT_EQ(&context.Get<TestComponent>(), &component);
}

TEST_F(BATLedgerContextTest, StartJob) {
  class Job : public BATLedgerContext::Object {
   public:
    Future<int> GetFuture() { return promise_.GetFuture(); }
    void Start(int n) { promise_.SetValue(n); }

   private:
    Promise<int> promise_;
  };

  int value = 0;
  context().StartJob<Job>(100).Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));

  task_environment()->RunUntilIdle();
  EXPECT_EQ(value, 100);
}

TEST_F(BATLedgerContextTest, Logging) {
  std::string last_message;
  auto on_log = [&](const std::string& message) { last_message = message; };

  SetLogCallbackForTesting(base::BindLambdaForTesting(on_log));

  context().LogError(FROM_HERE) << "error" << 1;
  ASSERT_EQ(last_message, "error1");

  context().LogInfo(FROM_HERE) << "info" << 2;
  ASSERT_EQ(last_message, "info2");

  context().LogVerbose(FROM_HERE) << "verbose" << 3;
  ASSERT_EQ(last_message, "verbose3");

  {
    // Moved-from streams do not log.
    auto stream1 = context().LogError(FROM_HERE);
    BATLedgerContext::LogStream stream2(std::move(stream1));
    last_message = "";
    stream1 << "1";
  }
  ASSERT_EQ(last_message, "");
}

}  // namespace ledger
