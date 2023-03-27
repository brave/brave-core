/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/database/database_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=DatabaseUtil.*

namespace brave_rewards::core {
namespace database {

class DatabaseUtil : public testing::Test {};

TEST(DatabaseUtil, GenerateStringInCase) {
  // empty list
  std::string result = GenerateStringInCase({});
  ASSERT_EQ(result, "");

  // one item
  result = GenerateStringInCase({"id_1"});
  ASSERT_EQ(result, "'id_1'");

  // multiple items
  result = GenerateStringInCase({"id_1", "id_2", "id_3"});
  ASSERT_EQ(result, "'id_1', 'id_2', 'id_3'");
}

}  // namespace database
}  // namespace brave_rewards::core
