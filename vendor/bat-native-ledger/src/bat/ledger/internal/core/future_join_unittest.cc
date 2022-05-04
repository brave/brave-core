/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/future_join.h"

#include <string>
#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ledger {

class FutureJoinTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(FutureJoinTest, JoinFutures) {
  auto future1 = MakeReadyFuture(10);
  auto future2 = MakeReadyFuture<std::string>("joiner");
  auto future3 = MakeReadyFuture(true);

  std::tuple<int, std::string, bool> result;

  JoinFutures(std::move(future1), std::move(future2), std::move(future3))
      .Then(base::BindLambdaForTesting(
          [&result](decltype(result) t) { result = std::move(t); }));

  task_environment_.RunUntilIdle();

  EXPECT_EQ(std::get<0>(result), 10);
  EXPECT_EQ(std::get<1>(result), "joiner");
  EXPECT_TRUE(std::get<2>(result));
}

TEST_F(FutureJoinTest, JoinFutureVector) {
  std::vector<Future<int>> futures;
  futures.push_back(MakeReadyFuture(10));
  futures.push_back(MakeReadyFuture(20));
  futures.push_back(MakeReadyFuture(30));

  std::vector<int> results;

  JoinFutures(std::move(futures))
      .Then(base::BindLambdaForTesting(
          [&results](decltype(results) r) { results = std::move(r); }));

  task_environment_.RunUntilIdle();

  ASSERT_EQ(results.size(), 3ul);
  EXPECT_EQ(results[0], 10);
  EXPECT_EQ(results[1], 20);
  EXPECT_EQ(results[2], 30);

  futures.clear();

  JoinFutures(std::move(futures))
      .Then(base::BindLambdaForTesting(
          [&results](decltype(results) r) { results = std::move(r); }));

  task_environment_.RunUntilIdle();

  EXPECT_TRUE(results.empty());
}

}  // namespace ledger
