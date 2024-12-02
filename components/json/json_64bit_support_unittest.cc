/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <limits>
#include <optional>

#include "base/containers/span.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/json/schema.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
constexpr char kSampleJson[] =
    R"a({"boolean":true,"maxint":9223372036854775807,"maxuint":18446744073709551615,"minint":-9223372036854775808,"minuint":0,"name":"whatever","price":1.5,"simpleint":2147483647})a";
}

using ::testing::Optional;

TEST(Json64BitSupportTest, Write64BitValues) {
  base::Value::Dict dict;
  dict.Set("maxuint", base::Value(base::as_byte_span(base::NumberToString(
                          std::numeric_limits<uint64_t>::max()))));
  dict.Set("minuint", base::Value(base::as_byte_span(base::NumberToString(
                          std::numeric_limits<uint64_t>::min()))));
  dict.Set("maxint", base::Value(base::as_byte_span(base::NumberToString(
                         std::numeric_limits<int64_t>::max()))));
  dict.Set("minint", base::Value(base::as_byte_span(base::NumberToString(
                         std::numeric_limits<int64_t>::min()))));
  dict.Set("price", 1.5);
  dict.Set("boolean", true);
  dict.Set("name", "whatever");
  dict.Set("simpleint", std::numeric_limits<int>::max());
  EXPECT_THAT(
      base::WriteJsonWithOptions(dict, base::OPTIONS_SERIALISE_64BIT_NUMBERS),
      kSampleJson);
}

TEST(Json64BitSupportTest, Read64BitValues) {
  auto parsed =
      base::JSONReader::ReadDict(kSampleJson, base::JSON_ALLOW_64BIT_NUMBERS);
  ASSERT_TRUE(parsed);
  ASSERT_TRUE(parsed->Find("boolean"));
  ASSERT_TRUE(parsed->Find("name"));
  ASSERT_TRUE(parsed->Find("price"));
  ASSERT_TRUE(parsed->Find("simpleint"));
  ASSERT_TRUE(parsed->Find("maxint"));
  ASSERT_TRUE(parsed->Find("maxuint"));
  ASSERT_TRUE(parsed->Find("minint"));
  ASSERT_TRUE(parsed->Find("minuint"));
  ASSERT_TRUE(parsed->Find("maxint")->is_blob());
  ASSERT_TRUE(parsed->Find("maxuint")->is_blob());
  ASSERT_TRUE(parsed->Find("minint")->is_blob());
  ASSERT_TRUE(parsed->Find("minuint")->is_int())
      << "This value should stay as an int, as it fits into it";

  EXPECT_EQ(*parsed->FindBool("boolean"), true);
  EXPECT_EQ(*parsed->FindString("name"), "whatever");
  EXPECT_EQ(*parsed->FindDouble("price"), 1.5);
  EXPECT_EQ(*parsed->FindInt("simpleint"), std::numeric_limits<int>::max());
  EXPECT_EQ(base::as_string_view(*parsed->FindBlob("maxint")),
            base::NumberToString(std::numeric_limits<int64_t>::max()));
  EXPECT_EQ(base::as_string_view(*parsed->FindBlob("maxuint")),
            base::NumberToString(std::numeric_limits<uint64_t>::max()));
  EXPECT_EQ(base::as_string_view(*parsed->FindBlob("minint")),
            base::NumberToString(std::numeric_limits<int64_t>::min()));
  EXPECT_EQ(*parsed->FindInt("minuint"), 0);
}

TEST(Json64BitSupportTest, SchemaCompilerSupport) {
  json::schema::Example example;
  example.id = 1;
  example.some_signal = 10.5;
  example.some_boolean = true;
  example.some_string = "hello";
  example.some_large_value = std::numeric_limits<int64_t>::max();
  example.some_large_unsigned_value = std::numeric_limits<uint64_t>::max();
  example.another_value = -10;
  example.some_other_value = 1000;

  auto json = base::WriteJsonWithOptions(example.ToValue(),
                                         base::OPTIONS_SERIALISE_64BIT_NUMBERS);
  EXPECT_THAT(
      json,
      R"a({"anotherValue":-10,"id":1,"someBoolean":true,"someLargeUnsignedValue":18446744073709551615,"someLargeValue":9223372036854775807,"someOtherValue":1000,"someSignal":10.5,"someString":"hello"})a");

  auto parsed =
      base::JSONReader::ReadDict(*json, base::JSON_ALLOW_64BIT_NUMBERS);
  ASSERT_TRUE(parsed);

  auto parsed_example = json::schema::Example::FromValue(*parsed);
  ASSERT_TRUE(parsed_example);
  EXPECT_EQ(parsed_example->ToValue(), example.ToValue());
}

}  // namespace brave_wallet
