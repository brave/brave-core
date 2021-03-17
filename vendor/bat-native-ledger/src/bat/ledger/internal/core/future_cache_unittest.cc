/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/future_cache.h"

#include <vector>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ledger {

class FutureCacheTest : public testing::Test {
 protected:
  auto MakeValueGenerator(int* calls) {
    return [calls]() {
      ++(*calls);
      return Future<int>::Completed(*calls);
    };
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};

TEST_F(FutureCacheTest, ReturnsPendingRequests) {
  FutureCache<int> cache;

  int generate_calls = 0;
  auto generate_value = MakeValueGenerator(&generate_calls);

  std::vector<int> values;
  auto gather_values = [&values](int value) { values.push_back(value); };

  cache.GetFuture(generate_value)
      .Then(base::BindLambdaForTesting(gather_values));
  cache.GetFuture(generate_value)
      .Then(base::BindLambdaForTesting(gather_values));
  cache.GetFuture(generate_value)
      .Then(base::BindLambdaForTesting(gather_values));

  EXPECT_EQ(generate_calls, 1);

  task_environment_.RunUntilIdle();

  cache.GetFuture(generate_value)
      .Then(base::BindLambdaForTesting(gather_values));

  task_environment_.RunUntilIdle();

  EXPECT_EQ(generate_calls, 2);

  ASSERT_EQ(values.size(), 4ul);
  EXPECT_EQ(values[0], 1);
  EXPECT_EQ(values[1], 1);
  EXPECT_EQ(values[2], 1);
  EXPECT_EQ(values[3], 2);
}

TEST_F(FutureCacheTest, ReturnsFreshResults) {
  FutureCache<int> cache;

  int generate_calls = 0;
  auto generate_value = MakeValueGenerator(&generate_calls);

  auto generate_cache_value = [&generate_value]() {
    return generate_value().Map(base::BindOnce(
        [](int value) { return std::make_pair(value, base::Seconds(10)); }));
  };

  cache.GetFuture(generate_cache_value);
  EXPECT_EQ(generate_calls, 1);

  task_environment_.FastForwardBy(base::Seconds(2));

  cache.GetFuture(generate_cache_value);
  EXPECT_EQ(generate_calls, 1);

  task_environment_.FastForwardBy(base::Seconds(8));

  cache.GetFuture(generate_cache_value);
  EXPECT_EQ(generate_calls, 2);
}

TEST_F(FutureCacheTest, SupportsUniqueKeys) {
  FutureCache<int, int> cache;

  int generate_calls = 0;
  auto generate_value = MakeValueGenerator(&generate_calls);

  cache.GetFuture(1, generate_value);
  cache.GetFuture(2, generate_value);

  EXPECT_EQ(generate_calls, 2);
}

}  // namespace ledger
