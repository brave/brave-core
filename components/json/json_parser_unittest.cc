/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "brave/components/json/json_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(JsonParser, ConvertUint64ToString) {
  std::string json =
      "{\"a\": " + base::NumberToString(std::numeric_limits<uint64_t>::max()) +
      "}";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a", json, false),
            R"({"a":"18446744073709551615"})");

  // std::numeric_limits<uint64_t>::max + 1
  json = "{\"a\": 18446744073709551616 }";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, true).empty());

  // std::numeric_limits<int64_t>::min()
  json =
      "{\"a\": " + base::NumberToString(std::numeric_limits<int64_t>::min()) +
      "}";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, true).empty());

  json = R"({"a": { "b/a": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a/b~1a", json, false),
            R"({"a":{"b/a":"1","c":2},"d":"string"})");

  json = R"({"a": { "b~a": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a/b~0a", json, false),
            R"({"a":{"b~a":"1","c":2},"d":"string"})");

  json = R"({"a": { "b": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a/b", json, false),
            R"({"a":{"b":"1","c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":1}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a/b/0/e", json, false),
            R"({"a":{"b":[{"e":"1"}],"c":2},"d":"string"})");

  json = R"({"a":[{"b":1}]})";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a/0/b", json, false),
            R"({"a":[{"b":"1"}]})");

  json = R"({"a":[1]})";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a/0", json, false),
            R"({"a":["1"]})");

  json = R"({"a": 0 })";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a", json, false),
            R"({"a":"0"})");

  json = R"({"a": 0.1 })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"a": "1" })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, false).empty());

  // Optional, unchanged if path not found or value is null.
  json = R"({"b": 1 })";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a", json, true), json);
  json = R"({"a": null })";
  EXPECT_EQ(json::convert_uint64_value_to_string("/a", json, true), json);
  // Wrong value type should still fail.
  json = R"({"a": "1" })";
  EXPECT_TRUE(json::convert_uint64_value_to_string("/a", json, true).empty());
}

TEST(JsonParser, ConvertInt64ToString) {
  std::string json = "{\"a\": 18446744073709551615 }";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());

  // std::numeric_limits<int64_t>::min()
  json =
      "{\"a\": " + base::NumberToString(std::numeric_limits<int64_t>::min()) +
      "}";
  EXPECT_EQ(json::convert_int64_value_to_string("/a", json, false),
            R"({"a":"-9223372036854775808"})");

  // std::numeric_limits<int64_t>::min() - 1
  json = "{\"a\": -9223372036854775809 }";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, true).empty());

  // std::numeric_limits<int64_t>::max()
  json =
      "{\"a\": " + base::NumberToString(std::numeric_limits<int64_t>::max()) +
      "}";
  EXPECT_EQ(json::convert_int64_value_to_string("/a", json, false),
            R"({"a":"9223372036854775807"})");

  json = R"({"a": { "b/a": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_int64_value_to_string("/a/b~1a", json, false),
            R"({"a":{"b/a":"1","c":2},"d":"string"})");

  json = R"({"a": { "b~a": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_int64_value_to_string("/a/b~0a", json, false),
            R"({"a":{"b~a":"1","c":2},"d":"string"})");

  json = R"({"a": { "b": 1, "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_int64_value_to_string("/a/b", json, false),
            R"({"a":{"b":"1","c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":1}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_int64_value_to_string("/a/b/0/e", json, false),
            R"({"a":{"b":[{"e":"1"}],"c":2},"d":"string"})");

  json = R"({"a":[{"b":1}]})";
  EXPECT_EQ(json::convert_int64_value_to_string("/a/0/b", json, false),
            R"({"a":[{"b":"1"}]})");

  json = R"({"a":[1]})";
  EXPECT_EQ(json::convert_int64_value_to_string("/a/0", json, false),
            R"({"a":["1"]})");

  json = R"({"a": 0 })";
  EXPECT_EQ(json::convert_int64_value_to_string("/a", json, false),
            R"({"a":"0"})");

  json = R"({"a": 0.1 })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"a": "1" })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, false).empty());

  // Optional, unchanged if path not found or value is null.
  json = R"({"b": 1 })";
  EXPECT_EQ(json::convert_int64_value_to_string("/a", json, true), json);
  json = R"({"a": null })";
  EXPECT_EQ(json::convert_int64_value_to_string("/a", json, true), json);

  // Wrong value type should still fail.
  json = R"({"a": "1" })";
  EXPECT_TRUE(json::convert_int64_value_to_string("/a", json, true).empty());
}

TEST(JsonParser, ConvertStringToUint64) {
  // std::numeric_limits<uint64_t>::max
  std::string json = R"({"a":"18446744073709551615"})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a", json, false),
            R"({"a":18446744073709551615})");

  json = R"({"a":"-1"})";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, true).empty());

  // std::numeric_limits<uint64_t>::max + 1
  json = R"({\"a\":18446744073709551616})";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, true).empty());

  json = R"({"a": { "b": "1", "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a/b", json, false),
            R"({"a":{"b":1,"c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":"1"}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a/b/0/e", json, false),
            R"({"a":{"b":[{"e":1}],"c":2},"d":"string"})");

  json = R"({"a~c": { "b": "1", "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a~0c/b", json, false),
            R"({"a~c":{"b":1,"c":2},"d":"string"})");

  json = R"({"a/d": { "b": [{"e":"1"}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a~1d/b/0/e", json, false),
            R"({"a/d":{"b":[{"e":1}],"c":2},"d":"string"})");

  json = R"({"a": { "b": "1" }})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a/b", json, false),
            R"({"a":{"b":1}})");

  json = R"({"a":[{"b":"1"}]})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a/0/b", json, false),
            R"({"a":[{"b":1}]})");

  json = R"({"a":["1"]})";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a/0", json, false),
            R"({"a":[1]})");

  json = R"({"a": "0" })";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a", json, false),
            R"({"a":0})");

  json = R"({"a": 1 })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"a": 0.1 })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, false).empty());

  // Optional, unchanged if path not found or value is null.
  json = R"({"b": "1" })";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a", json, true), json);
  json = R"({"a": null })";
  EXPECT_EQ(json::convert_string_value_to_uint64("/a", json, true), json);

  // Wrong value type should still fail.
  json = R"({"a": 1 })";
  EXPECT_TRUE(json::convert_string_value_to_uint64("/a", json, true).empty());
}

TEST(JsonParser, ConvertStringToInt64) {
  // std::numeric_limits<int64_t>::min()
  std::string json = R"({"a":"-9223372036854775808"})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a", json, false),
            R"({"a":-9223372036854775808})");
  // std::numeric_limits<int64_t>::min() - 1
  json = R"({"a":"-9223372036854775809"})";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, true).empty());

  // std::numeric_limits<int64_t>::max()
  json = "{\"a\": \"" +
         base::NumberToString(std::numeric_limits<int64_t>::max()) + "\"}";
  EXPECT_EQ(json::convert_string_value_to_int64("/a", json, false),
            R"({"a":9223372036854775807})");
  // std::numeric_limits<int64_t>::max() + 1
  json = "{\"a\": \"9223372036854775808\"}";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = "{\"a\": \"9223372036854775808\"}";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, true).empty());

  json = R"({"a": { "b": "1", "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a/b", json, false),
            R"({"a":{"b":1,"c":2},"d":"string"})");

  json = R"({"a": { "b": [{"e":"1"}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a/b/0/e", json, false),
            R"({"a":{"b":[{"e":1}],"c":2},"d":"string"})");

  json = R"({"a~e": { "b": "1", "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a~0e/b", json, false),
            R"({"a~e":{"b":1,"c":2},"d":"string"})");

  json = R"({"a/e": { "b": [{"e":"1"}], "c": 2 }, "d": "string"})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a~1e/b/0/e", json, false),
            R"({"a/e":{"b":[{"e":1}],"c":2},"d":"string"})");

  json = R"({"a": { "b": "1" }})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a/b", json, false),
            R"({"a":{"b":1}})");

  json = R"({"a":[{"b":"1"}]})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a/0/b", json, false),
            R"({"a":[{"b":1}]})");

  json = R"({"a":["1"]})";
  EXPECT_EQ(json::convert_string_value_to_int64("/a/0", json, false),
            R"({"a":[1]})");

  json = R"({"a": "0" })";
  EXPECT_EQ(json::convert_string_value_to_int64("/a", json, false),
            R"({"a":0})");

  json = R"({"a": 1 })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"a": 0.1 })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"a": "" })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"a": -1.0 })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"a": "a" })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"b": 1 })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"a": [] })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());
  json = R"({"a": {} })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, false).empty());

  // Optional, unchanged if path not found or value is null.
  json = R"({"b": "1" })";
  EXPECT_EQ(json::convert_string_value_to_int64("/a", json, true), json);
  json = R"({"a": null })";
  EXPECT_EQ(json::convert_string_value_to_int64("/a", json, true), json);

  // Wrong value type should still fail.
  json = R"({"a": 1 })";
  EXPECT_TRUE(json::convert_string_value_to_int64("/a", json, true).empty());
}

TEST(JsonParser, ConvertUint64InObjectArrayToString) {
  std::string json(
      R"({"a":[{"key":18446744073709551615},{"key":2},{"key":3}]})");
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "", "key", json),
      R"({"a":[{"key":"18446744073709551615"},{"key":"2"},{"key":"3"}]})");

  json = R"({"a":{"b":[{"key":18446744073709551615},{"key":2}]}})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a/b", "", "key", json),
      R"({"a":{"b":[{"key":"18446744073709551615"},{"key":"2"}]}})");

  // Null value support.
  json = R"({"a":[{"key":18446744073709551615},{"key":null}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "", "key", json),
      R"({"a":[{"key":"18446744073709551615"},{"key":null}]})");

  json = R"({"a":[{"b":{"key":18446744073709551615}},{"b":{"key":null}}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "/b", "key", json),
      R"({"a":[{"b":{"key":"18446744073709551615"}},{"b":{"key":null}}]})");

  // Empty object array, nothing to convert.
  json = R"({"a":[]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "", "key", json),
      json);

  // Unchanged when path is not found.
  json = R"({"b":[{"key":1},{"key":2}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "", "key", json),
      json);
  json = R"({"b":[{"c": {"key":1}},{"c": {"key":2}}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "", "key", json),
      json);

  // When key is not found in some of the objects in the array, no need to
  // convert those objects.
  json = R"({"a":[{"key":1},{"diff-key":1},{"key":2}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "", "key", json),
      R"({"a":[{"key":"1"},{"diff-key":1},{"key":"2"}]})");

  json = R"({"a":[{"b":{"key":1}},{"b":{"diff-key":1}},{"b":{"key":2}}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "/b", "key", json),
      R"({"a":[{"b":{"key":"1"}},{"b":{"diff-key":1}},{"b":{"key":"2"}}]})");

  std::vector<std::string> invalid_cases = {
      // Value at path isn't an array.
      R"({"a":{[{"key":1},{"key":2}}})",
      // Value at path isn't an object array.
      R"({"a":[{"key":1}, [], {"key":2}})",
      // Value at key is not uint64 or null.
      R"({"a":[{"key":"1"}]})",
      // std::numeric_limits<uint64_t>::max + 1
      R"("{a":[{"key":18446744073709551616}]})",
      // std::numeric_limits<int64_t>::min()
      R"("{a":[{"key":)" +
          base::NumberToString(std::numeric_limits<int64_t>::min()) + "}]}"};
  for (const auto& invalid_case : invalid_cases) {
    EXPECT_EQ("", json::convert_uint64_in_object_array_to_string(
                      "/a", "", "key", invalid_case))
        << invalid_case;
  }

  invalid_cases = {
      // Value at path isn't an array.
      R"({"a":{[{"key":1},{"key":2}}})",
      // Value at path isn't an object array.
      R"({"a":[{"key":1}, [], {"key":2}})",
      // Value at key is not uint64 or null.
      R"({"a":[{"b": {"key":"1"}}]})",
      // std::numeric_limits<uint64_t>::max + 1
      R"("{a":[{"b": {"key":18446744073709551616}}]})",
      // std::numeric_limits<int32_t>::min()
      R"("{a":[{"b": {"key":)" +
          base::NumberToString(std::numeric_limits<int32_t>::min()) + "}}]}"};
  for (const auto& invalid_case : invalid_cases) {
    EXPECT_EQ("", json::convert_uint64_in_object_array_to_string(
                      "/a", "/b", "key", invalid_case))
        << invalid_case;
  }

  // Object array where key is nested
  json =
      R"({"a":[{"b":{"key":18446744073709551615}},{"b":{"key":2}},{"b":{"key":3}}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "/b", "key", json),
      R"({"a":[{"b":{"key":"18446744073709551615"}},{"b":{"key":"2"}},{"b":{"key":"3"}}]})");

  // Object array where key is nested deeper
  json =
      R"({"a":[{"b":{"c":{"key":18446744073709551615}}},{"b":{"c":{"key":2}}}]})";
  EXPECT_EQ(
      json::convert_uint64_in_object_array_to_string("/a", "/b/c", "key", json),
      R"({"a":[{"b":{"c":{"key":"18446744073709551615"}}},{"b":{"c":{"key":"2"}}}]})");
}

TEST(JsonParser, ConvertAllNumbersToString) {
  // OK: convert u64, f64, and i64 values to string
  std::string json(
      R"({"a":[{"key":18446744073709551615},{"key":-2},{"key":3.14}]})");
  EXPECT_EQ(
      json::convert_all_numbers_to_string(json, ""),
      R"({"a":[{"key":"18446744073709551615"},{"key":"-2"},{"key":"3.14"}]})");

  // OK: convert deeply nested value to string
  json = R"({"some":[{"deeply":{"nested":[{"path":123}]}}]})";
  EXPECT_EQ(json::convert_all_numbers_to_string(json, ""),
            R"({"some":[{"deeply":{"nested":[{"path":"123"}]}}]})");

  // OK: values other than u64/f64/i64 are unchanged
  json = R"({"a":[{"key":18446744073709551615},{"key":null},{"key":true}]})";
  EXPECT_EQ(
      json::convert_all_numbers_to_string(json, ""),
      R"({"a":[{"key":"18446744073709551615"},{"key":null},{"key":true}]})");

  // OK: empty object array, nothing to convert
  json = R"({"a":[]})";
  EXPECT_EQ(json::convert_all_numbers_to_string(json, ""), json);

  // OK: empty array json, nothing to convert
  json = R"([])";
  EXPECT_EQ(json::convert_all_numbers_to_string(json, ""), json);

  // OK: floating point values in scientific notation are unchanged
  json = R"({"a": 1.196568750220778e-7})";
  EXPECT_EQ(json::convert_all_numbers_to_string(json, ""),
            R"({"a":"1.196568750220778e-7"})");

  // OK: convert under specified JSON path only
  json = R"({"a":1,"outer":{"inner": 2}})";
  EXPECT_EQ(json::convert_all_numbers_to_string(json, "/outer"),
            R"({"a":1,"outer":{"inner":"2"}})");
  EXPECT_EQ(json::convert_all_numbers_to_string(json, "/a"),
            R"({"a":"1","outer":{"inner":2}})");

  // KO: invalid path has no effect on the JSON
  json = R"({"a":1,"outer":{"inner":2}})";
  EXPECT_EQ(json::convert_all_numbers_to_string(json, "/invalid"), json);
  EXPECT_EQ(json::convert_all_numbers_to_string(json, "/"), json);

  // KO: invalid cases
  std::vector<std::string> invalid_cases = {
      // invalid json
      R"({"a": hello})",
      // std::numeric_limits<uint64_t>::max + 1
      R"("{a":[{"key":18446744073709551616}]})",
      // std::numeric_limits<int64_t>::min()
      R"("{a":[{"key":)" +
          base::NumberToString(std::numeric_limits<int64_t>::min()) + "}]}",
      // std::numeric_limits<double>::min()
      R"("{a":[{"key":)" +
          base::NumberToString(std::numeric_limits<double>::min()) + "}]}",
      // std::numeric_limits<double>::max()
      R"("{a":[{"key":)" +
          base::NumberToString(std::numeric_limits<double>::max() + 1) + "}]}"};
  for (const auto& invalid_case : invalid_cases) {
    EXPECT_EQ("", json::convert_all_numbers_to_string(invalid_case, ""))
        << invalid_case;
  }
}

TEST(JsonParser, ConvertAllNumbersToStringAndRemoveNullValues) {
  // OK: convert u64, f64, and i64 values to string
  std::string json(
      R"({"a":[{"key":18446744073709551615},{"key":-2},{"key":3.14},
      {"key":null}]})");
  EXPECT_EQ(
      json::convert_all_numbers_to_string_and_remove_null_values(json, ""),
      R"({"a":[{"key":"18446744073709551615"},{"key":"-2"},{"key":"3.14"}]})");

  json =
      R"({"some":[{"deeply":{"nested":[{"path":123, "nullprop1": null}]}}],
       "nullprop2": null})";
  EXPECT_EQ(
      json::convert_all_numbers_to_string_and_remove_null_values(json, ""),
      R"({"some":[{"deeply":{"nested":[{"path":"123"}]}}]})");

  // OK: remove null values, empty strings, arrays, null values in array
  json = R"({"a":1,"outer":{"inner":2,"nullprop1":null,"arr_with_nulls":[null],
    "empty_string":"","empty_array":[]},"nulprop2":null})";
  EXPECT_EQ(
      json::convert_all_numbers_to_string_and_remove_null_values(json, ""),
      R"({"a":"1","outer":{"inner":"2"}})");

  // OK: convert under specified JSON path only
  json = R"({"a":1,"outer":{"inner": 2, "nullprop1": null}, "nulprop2": null})";

  EXPECT_EQ(json::convert_all_numbers_to_string_and_remove_null_values(
                json, "/outer"),
            R"({"a":1,"nulprop2":null,"outer":{"inner":"2"}})");

  // OK: invalid path has no effect on the JSON
  json = R"({"a":1,"outer":{"inner":2}})";
  EXPECT_EQ(json::convert_all_numbers_to_string_and_remove_null_values(
                json, "/invalid"),
            json);
  EXPECT_EQ(
      json::convert_all_numbers_to_string_and_remove_null_values(json, "/"),
      json);
}

}  // namespace brave_wallet
