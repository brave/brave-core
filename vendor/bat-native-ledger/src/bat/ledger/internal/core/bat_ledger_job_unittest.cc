/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/bat_ledger_job.h"

#include "base/bind.h"
#include "bat/ledger/internal/core/bat_ledger_test.h"

namespace ledger {

class BATLedgerJobTest : public BATLedgerTest {};

TEST_F(BATLedgerJobTest, StartJob) {
  class Job : public BATLedgerJob<bool> {
   public:
    void Start(int n) {
      MakeReadyFuture(n).Then(ContinueWith(this, &Job::OnDone));
    }

   private:
    void OnDone(int n) { Complete(static_cast<bool>(n)); }
  };

  bool value = false;
  context().StartJob<Job>(10).Then(
      base::BindLambdaForTesting([&value](bool v) { value = v; }));

  task_environment()->RunUntilIdle();
  EXPECT_TRUE(value);
}

TEST_F(BATLedgerJobTest, ContinueWithLambda) {
  class Job : public BATLedgerJob<int> {
   public:
    void Start() { StdFunctionApi(ContinueWithLambda(this, &Job::Callback)); }

   private:
    void Callback(int value) { Complete(value); }

    static void StdFunctionApi(std::function<void(int)> callback) {
      MakeReadyFuture(true).Then(base::BindLambdaForTesting(
          [callback = std::move(callback)](bool) { callback(42); }));
    }
  };

  int value = 0;
  context().StartJob<Job>().Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));

  task_environment()->RunUntilIdle();
  EXPECT_EQ(value, 42);
}

TEST_F(BATLedgerJobTest, CompleteWithFuture) {
  class Job : public BATLedgerJob<int> {
   public:
    void Start() { CompleteWithFuture(MakeReadyFuture(42)); }
  };

  int value = 0;
  context().StartJob<Job>().Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));

  task_environment()->RunUntilIdle();
  EXPECT_EQ(value, 42);
}

}  // namespace ledger
