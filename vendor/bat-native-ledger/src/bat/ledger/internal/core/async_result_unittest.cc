/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/async_result.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ledger {

class AsyncResultTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(AsyncResultTest, CompleteResultSentInFutureTurn) {
  AsyncResult<int>::Resolver resolver;
  resolver.Complete(10);
  int value = 0;
  resolver.result().Then(
      base::BindLambdaForTesting([&value](const int& v) { value = v; }));
  ASSERT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  ASSERT_EQ(value, 10);
}

TEST_F(AsyncResultTest, CompleteCallbacksExecutedInFutureTurn) {
  AsyncResult<int>::Resolver resolver;
  int value = 0;
  resolver.result().Then(
      base::BindLambdaForTesting([&value](const int& v) { value = v; }));
  resolver.Complete(1);
  ASSERT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  ASSERT_EQ(value, 1);
}

}  // namespace ledger
