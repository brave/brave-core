/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/future.h"

#include <string>
#include <tuple>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ledger {

class FutureTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(FutureTest, ValueSentInFutureTurn) {
  int value = 0;
  MakeReadyFuture(10).Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));
  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 10);
}

TEST_F(FutureTest, CompleteCallbacksExecutedInFutureTurn) {
  Promise<int> promise;
  int value = 0;
  promise.GetFuture().Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));
  promise.SetValue(1);
  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 1);
}

TEST_F(FutureTest, TransformingThen) {
  double value = 0;

  MakeReadyFuture(1)
      .Then(base::BindOnce([](int v) { return static_cast<double>(v) / 2; }))
      .Then(base::BindLambdaForTesting([&value](double v) { value = v; }));

  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 0.5);
}

TEST_F(FutureTest, UnwrappingThen) {
  bool value = false;

  MakeReadyFuture(42)
      .Then(base::BindOnce([](int value) { return MakeReadyFuture(true); }))
      .Then(base::BindLambdaForTesting([&value](bool v) { value = v; }));

  EXPECT_FALSE(value);
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(value);
}

TEST_F(FutureTest, MakeReadyFuture) {
  int value = 0;
  MakeReadyFuture(1).Then(
      base::BindLambdaForTesting([&value](int v) { value = v; }));
  EXPECT_EQ(value, 0);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(value, 1);
}

}  // namespace ledger
