/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/json/rs/src/lib.rs.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(JsonParser, ConvertUint64ToString) {
  std::string json = "{\"a\": " + std::to_string(UINT64_MAX) + "}";
  EXPECT_EQ(std::string(json::convert_uint64_value_to_string("/a", json)),
            R"({"a":"18446744073709551615"})");

  // UINT64_MAX + 1
  json = "{\"a\": 18446744073709551616 }";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());

  // INT64_MIN
  json = "{\"a\": " + std::to_string(INT64_MIN) + "}";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());

  json = R"({"a": { "b": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_uint64_value_to_string("/a/b", json)),
            R"({"a":{"b":"1","c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":1}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_uint64_value_to_string("/a/b/0/e", json)),
            R"({"a":{"b":[{"e":"1"}],"c":2},"d":"string"})");

  json = R"({"a":[{"b":1}]})";
  EXPECT_EQ(std::string(json::convert_uint64_value_to_string("/a/0/b", json)),
            R"({"a":[{"b":"1"}]})");

  json = R"({"a":[1]})";
  EXPECT_EQ(std::string(json::convert_uint64_value_to_string("/a/0", json)),
            R"({"a":["1"]})");

  json = R"({"a": 0 })";
  EXPECT_EQ(std::string(json::convert_uint64_value_to_string("/a", json)),
            R"({"a":"0"})");

  json = R"({"a": 0.1 })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"a": "1" })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(
      std::string(json::convert_uint64_value_to_string("/a", json)).empty());
}

TEST(JsonParser, ConvertInt64ToString) {
  std::string json = "{\"a\": 18446744073709551615 }";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());

  // INT64_MIN
  json = "{\"a\": " + std::to_string(INT64_MIN) + "}";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a", json)),
            R"({"a":"-9223372036854775808"})");

  // INT64_MIN - 1
  json = "{\"a\": -9223372036854775809 }";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());

  // INT64_MAX
  json = "{\"a\": " + std::to_string(INT64_MAX) + "}";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a", json)),
            R"({"a":"9223372036854775807"})");

  json = R"({"a": { "b": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a/b", json)),
            R"({"a":{"b":"1","c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":1}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a/b/0/e", json)),
            R"({"a":{"b":[{"e":"1"}],"c":2},"d":"string"})");

  json = R"({"a":[{"b":1}]})";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a/0/b", json)),
            R"({"a":[{"b":"1"}]})");

  json = R"({"a":[1]})";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a/0", json)),
            R"({"a":["1"]})");

  json = R"({"a": 0 })";
  EXPECT_EQ(std::string(json::convert_int64_value_to_string("/a", json)),
            R"({"a":"0"})");

  json = R"({"a": 0.1 })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"a": "1" })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(
      std::string(json::convert_int64_value_to_string("/a", json)).empty());
}

TEST(JsonParser, ConvertStringToUint64) {
  // UINT64_MAX
  std::string json = R"({"a":"18446744073709551615"})";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a", json)),
            R"({"a":18446744073709551615})");

  json = R"({"a":"-1"})";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  // UINT64_MAX + 1
  json = R"({\"a\":18446744073709551616})";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());

  json = R"({"a": { "b": "1", "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a/b", json)),
            R"({"a":{"b":1,"c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":"1"}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a/b/0/e", json)),
            R"({"a":{"b":[{"e":1}],"c":2},"d":"string"})");

  json = R"({"a": { "b": "1" }})";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a/b", json)),
            R"({"a":{"b":1}})");

  json = R"({"a":[{"b":"1"}]})";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a/0/b", json)),
            R"({"a":[{"b":1}]})");

  json = R"({"a":["1"]})";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a/0", json)),
            R"({"a":[1]})");

  json = R"({"a": "0" })";
  EXPECT_EQ(std::string(json::convert_string_value_to_uint64("/a", json)),
            R"({"a":0})");

  json = R"({"a": 1 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"a": 0.1 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_uint64("/a", json)).empty());
}

TEST(JsonParser, ConvertStringToInt64) {
  // INT64_MIN
  std::string json = R"({"a":"-9223372036854775808"})";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a", json)),
            R"({"a":-9223372036854775808})");
  // INT64_MIN - 1
  json = R"({"a":"-9223372036854775809"})";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());

  // INT64_MAX
  json = "{\"a\": \"" + std::to_string(INT64_MAX) + "\"}";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a", json)),
            R"({"a":9223372036854775807})");
  // INT64_MAX + 1
  json = "{\"a\": \"9223372036854775808\"}";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());

  json = R"({"a": { "b": "1", "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a/b", json)),
            R"({"a":{"b":1,"c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":"1"}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a/b/0/e", json)),
            R"({"a":{"b":[{"e":1}],"c":2},"d":"string"})");

  json = R"({"a": { "b": "1" }})";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a/b", json)),
            R"({"a":{"b":1}})");

  json = R"({"a":[{"b":"1"}]})";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a/0/b", json)),
            R"({"a":[{"b":1}]})");

  json = R"({"a":["1"]})";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a/0", json)),
            R"({"a":[1]})");

  json = R"({"a": "0" })";
  EXPECT_EQ(std::string(json::convert_string_value_to_int64("/a", json)),
            R"({"a":0})");

  json = R"({"a": 1 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"a": 0.1 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(
      std::string(json::convert_string_value_to_int64("/a", json)).empty());
}

}  // namespace brave_wallet
