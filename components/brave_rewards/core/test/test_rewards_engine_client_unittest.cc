/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "sql/statement.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=TestRewardsEngineClientTest.*

namespace brave_rewards::internal {

class TestRewardsEngineClientTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  TestRewardsEngineClient client_;
};

TEST_F(TestRewardsEngineClientTest, CanAccessDatabaseDirectly) {
  sql::Database& db = client_.database().GetInternalDatabaseForTesting();
  ASSERT_TRUE(db.Execute("CREATE TABLE test_table (num INTEGER);"));
  ASSERT_TRUE(db.Execute("INSERT INTO test_table VALUES (42);"));
  sql::Statement s(db.GetUniqueStatement("SELECT num FROM test_table"));
  ASSERT_TRUE(s.Step());
  ASSERT_EQ(s.ColumnInt(0), 42);
}

}  // namespace brave_rewards::internal
