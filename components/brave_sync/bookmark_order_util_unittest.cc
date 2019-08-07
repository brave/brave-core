/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
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
  EXPECT_EQ(result.at(0), 1);
  EXPECT_EQ(result.at(1), 7);
  EXPECT_EQ(result.at(2), 4);
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

TEST(BookmarkOrderUtilTest, ToOrderString) {
  EXPECT_EQ(ToOrderString({}), "");
  EXPECT_EQ(ToOrderString({1}), "1");
  EXPECT_EQ(ToOrderString({1, 2, 3}), "1.2.3");
  EXPECT_EQ(ToOrderString({-1, 2, 3}), "");
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

  EXPECT_TRUE(CompareOrder("2.0.8", "2.0.8.0.1"));
  EXPECT_TRUE(CompareOrder("2.0.8.0.1", "2.0.8.1"));

  EXPECT_TRUE(CompareOrder("2.0.8", "2.0.8.0.0.1"));
  EXPECT_TRUE(CompareOrder("2.0.8.0.0.1", "2.0.8.0.1"));

  EXPECT_TRUE(CompareOrder("2.0.8.10", "2.0.8.10.1"));
  EXPECT_TRUE(CompareOrder("2.0.8.10.1", "2.0.8.11.1"));

  EXPECT_TRUE(CompareOrder("2.0.0.1", "2.0.1"));

  EXPECT_TRUE(CompareOrder("2.5.6.3", "2.5.7.8.2"));
  EXPECT_TRUE(CompareOrder("2.5.6.3", "2.5.6.4"));
  EXPECT_TRUE(CompareOrder("2.5.6.4", "2.5.7.8.2"));

  EXPECT_TRUE(CompareOrder("2.0.8.10", "2.0.8.11"));
  EXPECT_TRUE(CompareOrder("2.0.8.11", "2.0.8.11.1"));
}

TEST(BookmarkOrderUtilTest, GetOrder) {
  // Ported from https://github.com/brave/sync/blob/staging/test/client/bookmarkUtil.js
  EXPECT_EQ(GetOrder("", "2.0.1", ""), "2.0.0.1");

  EXPECT_EQ(GetOrder("", "2.0.9", ""), "2.0.8");
  EXPECT_EQ(GetOrder("2.0.8", "", ""), "2.0.9");
  EXPECT_EQ(GetOrder("2.0.8", "2.0.9", ""), "2.0.8.1");

  EXPECT_EQ(GetOrder("2.0.8", "2.0.8.1", ""), "2.0.8.0.1");
  EXPECT_EQ(GetOrder("2.0.8", "2.0.8.0.1", ""), "2.0.8.0.0.1");
  EXPECT_EQ(GetOrder("2.0.8", "2.0.8.0.0.1", ""), "2.0.8.0.0.0.1");

  EXPECT_EQ(GetOrder("2.0.8.1", "2.0.9", ""), "2.0.8.2");
  EXPECT_EQ(GetOrder("2.0.8.1", "2.0.10", ""), "2.0.8.2");
  EXPECT_EQ(GetOrder("2.0.8.10", "2.0.8.15", ""), "2.0.8.11");

  EXPECT_EQ(GetOrder("2.0.8.10", "2.0.8.15.1", ""), "2.0.8.11");
  EXPECT_EQ(GetOrder("2.0.8.10", "2.0.8.11.1", ""), "2.0.8.11");

  EXPECT_EQ(GetOrder("2.0.8.11", "2.0.8.11.1", ""), "2.0.8.11.0.1");

  EXPECT_EQ(GetOrder("2.0.8.10.0.1", "2.0.8.15.1", ""), "2.0.8.10.0.2");
  EXPECT_EQ(GetOrder("", "", "2.0.9"), "2.0.9.1");

  EXPECT_EQ(GetOrder("2.5.6.3", "2.5.7.8.2", ""), "2.5.6.4");
  EXPECT_EQ(GetOrder("2.5.6.35", "2.5.7.8.2", ""), "2.5.6.36");

  EXPECT_EQ(GetOrder("1.1.1.2", "1.1.1.2.1", ""), "1.1.1.2.0.1");
  EXPECT_EQ(GetOrder("1.1.1.2.1", "1.1.1.3", ""), "1.1.1.2.2");
}

}   // namespace brave_sync
