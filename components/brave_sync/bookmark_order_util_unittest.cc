/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/bookmark_order_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_sync {

TEST(BookmarkOrderUtilTest, OrderToIntVect_EmptyString) {
  std::vector<int> result = OrderToIntVect("");
  EXPECT_TRUE(result.empty());
}

TEST(BookmarkOrderUtilTest, OrderToIntVect_SingleValue) {
  std::vector<int> result = OrderToIntVect("1");
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result.at(0), 1);
}

TEST(BookmarkOrderUtilTest, OrderToIntVect_TypicalValue) {
  std::vector<int> result = OrderToIntVect("1.7.4");
  ASSERT_EQ(result.size(), 3u);
  EXPECT_TRUE(result.at(0) == 1);
  EXPECT_TRUE(result.at(1) == 7);
  EXPECT_TRUE(result.at(2) == 4);
}

TEST(BookmarkOrderUtilTest, OrderToIntVect_WrongValue) {
  std::vector<int> result = OrderToIntVect("..");
  EXPECT_TRUE(result.empty());
}

TEST(BookmarkOrderUtilTest, OrderToIntVect_SemiWrongValue) {
  std::vector<int> result = OrderToIntVect(".5.");
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result.at(0), 5);
}

TEST(BookmarkOrderUtilTest, CompareOrder) {
  EXPECT_FALSE(CompareOrder("", ""));
  EXPECT_TRUE(CompareOrder("1", "2"));
  EXPECT_TRUE(CompareOrder("1", "1.1"));
  EXPECT_TRUE(CompareOrder("1.1", "2.234.1"));
  EXPECT_TRUE(CompareOrder("2.234.1", "63.17.1.45.2"));

  EXPECT_FALSE(CompareOrder("2", "1"));
  EXPECT_TRUE(CompareOrder("2", "11"));
  EXPECT_FALSE(CompareOrder("11", "2"));

  EXPECT_TRUE(CompareOrder("1.7.0.1", "1.7.1"));
  EXPECT_TRUE(CompareOrder("1.7.0.1", "1.7.0.2"));
  EXPECT_FALSE(CompareOrder("1.7.0.2", "1.7.0.1"));
}

} // namespace brave_sync
