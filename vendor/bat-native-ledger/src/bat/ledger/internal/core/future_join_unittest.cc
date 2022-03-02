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

// npm run test -- brave_unit_tests --filter=FutureJoinTest.*

namespace ledger {

class FutureJoinTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(FutureJoinTest, JoinFutures) {
  auto future1 = MakeFuture(10);
  auto future2 = MakeFuture<std::string>("joiner");
  auto future3 = MakeFuture(true);

  int int_result = 0;
  std::string string_result = "";
  bool bool_result = false;

  JoinFutures(std::move(future1), std::move(future2), std::move(future3))
      .Then(base::BindLambdaForTesting([&](int ir, std::string sr, bool br) {
        int_result = ir;
        string_result = std::move(sr);
        bool_result = br;
      }));

  task_environment_.RunUntilIdle();

  EXPECT_EQ(int_result, 10);
  EXPECT_EQ(string_result, "joiner");
  EXPECT_TRUE(bool_result);
}

TEST_F(FutureJoinTest, JoinFutureVector) {
  std::vector<Future<int>> futures;
  futures.push_back(MakeFuture(10));
  futures.push_back(MakeFuture(20));
  futures.push_back(MakeFuture(30));

  std::vector<int> results;

  JoinFutures(std::move(futures))
      .Then(base::BindLambdaForTesting(
          [&results](decltype(results) r) { results = std::move(r); }));

  task_environment_.RunUntilIdle();

  EXPECT_EQ(results[0], 10);
  EXPECT_EQ(results[1], 20);
  EXPECT_EQ(results[2], 30);
  EXPECT_EQ(results.size(), 3ul);
}

}  // namespace ledger
