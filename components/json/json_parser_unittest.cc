/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/json/rs/src/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(JsonParser, ConvertUint64ToString) {
  std::string json = "{\"a\": " + std::to_string(UINT64_MAX) + "}";
  EXPECT_EQ(std::string(json::convert_i64_to_string("/a", json)),
            R"({"a":"18446744073709551615"})");

  // UINT64_MAX + 1
  json = "{\"a\": 18446744073709551616 }";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  // INT64_MIN
  json = "{\"a\": " + std::to_string(INT64_MIN) + "}";
  EXPECT_EQ(std::string(json::convert_i64_to_string("/a", json)),
            R"({"a":"-9223372036854775808"})");

  // INT64_MIN - 1
  json = "{\"a\": -9223372036854775809 }";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());

  json = R"({"a": { "b": 1 }})";
  EXPECT_EQ(std::string(json::convert_i64_to_string("/a/b", json)),
            R"({"a":{"b":"1"}})");

  json = R"({"a":[{"b":1}]})";
  EXPECT_EQ(std::string(json::convert_i64_to_string("/a/0/b", json)),
            R"({"a":[{"b":"1"}]})");

  json = R"({"a":[1]})";
  EXPECT_EQ(std::string(json::convert_i64_to_string("/a/0", json)),
            R"({"a":["1"]})");

  json = R"({"a": 0 })";
  EXPECT_EQ(std::string(json::convert_i64_to_string("/a", json)),
            R"({"a":"0"})");

  json = R"({"a": 0.1 })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"a": "1" })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(std::string(json::convert_i64_to_string("/a", json)).empty());
}

TEST(JsonParser, ConvertStringToUint64) {
  // UINT64_MAX
  std::string json = R"({"a":"18446744073709551615"})";
  EXPECT_EQ(std::string(json::convert_string_to_i64("/a", json)),
            R"({"a":18446744073709551615})");
  // INT64_MIN
  json = R"({"a":"-9223372036854775808"})";
  EXPECT_EQ(std::string(json::convert_string_to_i64("/a", json)),
            R"({"a":-9223372036854775808})");
  // UINT64_MAX + 1
  json = R"({\"a\":18446744073709551616})";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  // INT64_MIN - 1
  json = R"({"a":"-9223372036854775809"})";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());

  json = R"({"a": { "b": "1" }})";
  EXPECT_EQ(std::string(json::convert_string_to_i64("/a/b", json)),
            R"({"a":{"b":1}})");

  json = R"({"a":[{"b":"1"}]})";
  EXPECT_EQ(std::string(json::convert_string_to_i64("/a/0/b", json)),
            R"({"a":[{"b":1}]})");

  json = R"({"a":["1"]})";
  EXPECT_EQ(std::string(json::convert_string_to_i64("/a/0", json)),
            R"({"a":[1]})");

  json = R"({"a": "0" })";
  EXPECT_EQ(std::string(json::convert_string_to_i64("/a", json)), R"({"a":0})");

  json = R"({"a": 1 })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"a": 0.1 })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(std::string(json::convert_string_to_i64("/a", json)).empty());
}

}  // namespace brave_wallet
