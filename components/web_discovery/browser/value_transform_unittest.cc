/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/value_transform.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

namespace {

// Helper function to create a value transform from a JSON string definition
std::unique_ptr<ValueTransform> CreateTransform(const std::string& json_def) {
  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(
      json_def, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!parsed_json.has_value() || !parsed_json->is_list()) {
    return nullptr;
  }
  return CreateValueTransform(parsed_json->GetList());
}

}  // namespace

TEST(WebDiscoveryValueTransformTest, DecodeURIComponentTransform) {
  // Test regular decodeURIComponent
  auto transform = CreateTransform(R"(["decodeURIComponent"])");
  ASSERT_TRUE(transform);

  // Test successful decoding
  auto result = transform->Process("Hello%20World");
  EXPECT_EQ(result, "Hello World");

  // Test already decoded string
  result = transform->Process("Hello World");
  EXPECT_EQ(result, "Hello World");

  // Test complex encoding
  result = transform->Process("Hello%20%26%20Goodbye");
  EXPECT_EQ(result, "Hello & Goodbye");

  // Test double encoding (%25 -> %)
  result = transform->Process("Hello%2520World");
  EXPECT_EQ(result, "Hello%20World");

  result = transform->Process("Hello%25%ZZWorld");
  EXPECT_EQ(result, std::nullopt);

  // Test invalid encoding (should fail in regular mode)
  result = transform->Process("Hello%ZZWorld");
  EXPECT_EQ(result, std::nullopt);
}

TEST(WebDiscoveryValueTransformTest, TryDecodeURIComponentTransform) {
  // Test tryDecodeURIComponent
  auto transform = CreateTransform(R"(["tryDecodeURIComponent"])");
  ASSERT_TRUE(transform);

  // Test successful decoding
  auto result = transform->Process("Hello%20World");
  EXPECT_EQ(result, "Hello World");

  // Test invalid encoding (should return original in try mode)
  result = transform->Process("Hello%ZZWorld");
  EXPECT_EQ(result, "Hello%ZZWorld");

  // Test already decoded string
  result = transform->Process("Hello World");
  EXPECT_EQ(result, "Hello World");
}

TEST(WebDiscoveryValueTransformTest, FilterExactTransform) {
  // Test filterExact with allowed strings
  auto transform =
      CreateTransform(R"(["filterExact", ["apple", "banana", "cherry"]])");
  ASSERT_TRUE(transform);

  // Test allowed values
  auto result = transform->Process("apple");
  EXPECT_EQ(result, "apple");

  result = transform->Process("banana");
  EXPECT_EQ(result, "banana");

  result = transform->Process("cherry");
  EXPECT_EQ(result, "cherry");

  // Test disallowed values
  result = transform->Process("orange");
  EXPECT_EQ(result, std::nullopt);

  result = transform->Process("Apple");  // Case sensitive
  EXPECT_EQ(result, std::nullopt);

  result = transform->Process("");
  EXPECT_EQ(result, std::nullopt);

  // Test invalid definition (non-string in list)
  auto invalid_transform =
      CreateTransform(R"(["filterExact", ["apple", 123, "cherry"]])");
  EXPECT_FALSE(invalid_transform);
}

TEST(WebDiscoveryValueTransformTest, RemoveParamsTransform) {
  // Test removeParams
  auto transform = CreateTransform(
      R"(["removeParams", ["utm_source", "utm_medium", "fbclid"]])");
  ASSERT_TRUE(transform);

  // Test URL with params to remove
  auto result = transform->Process(
      "https://"
      "example.com?utm_source=google&param1=value1&utm_medium=cpc&param2="
      "value2");
  EXPECT_EQ(result, "https://example.com/?param1=value1&param2=value2");

  // Test URL with no query params
  result = transform->Process("https://example.com");
  EXPECT_EQ(result, "https://example.com");

  // Test URL with all params removed
  result = transform->Process(
      "https://example.com?utm_source=google&utm_medium=cpc");
  EXPECT_EQ(result, "https://example.com/");

  // Test invalid URL
  result = transform->Process("not-a-url");
  EXPECT_EQ(result, std::nullopt);

  // Test URL with no matching params
  result = transform->Process("https://example.com?other_param=value");
  EXPECT_EQ(result, "https://example.com/?other_param=value");
}

TEST(WebDiscoveryValueTransformTest, MaskUTransform) {
  // Test regular maskU
  auto transform = CreateTransform(R"(["maskU"])");
  ASSERT_TRUE(transform);

  // Test URL that needs masking (has fragment) - should mask to scheme://host/
  // (PROTECTED)
  auto result =
      transform->Process("https://example.com/path?param=value#fragment");
  EXPECT_EQ(result, "https://example.com/ (PROTECTED)");

  // Test simple URL that doesn't need masking (no fragment, query, etc.)
  result = transform->Process("https://example.com/");
  EXPECT_EQ(result, "https://example.com/");

  // Test invalid URL - should be dropped
  result = transform->Process("not-a-url");
  EXPECT_EQ(result, std::nullopt);

  // Test URL with username/password - should be dropped
  result = transform->Process("https://user:pass@example.com/");
  EXPECT_EQ(result, std::nullopt);

  // Test URL with non-standard port - should be dropped
  result = transform->Process("https://example.com:8080/");
  EXPECT_EQ(result, std::nullopt);
}

TEST(WebDiscoveryValueTransformTest, RelaxedMaskUTransform) {
  // Test relaxedMaskU
  auto transform = CreateTransform(R"(["relaxedMaskU"])");
  ASSERT_TRUE(transform);

  // Test URL with query params - relaxed mode should try to preserve path
  auto result = transform->Process(
      "https://example.com/safe/path?param=sensitive#fragment");
  // Should preserve path and add (PROTECTED) suffix
  EXPECT_EQ(result, "https://example.com/safe/path (PROTECTED)");

  // Test URL with fragment - should be masked
  result = transform->Process("https://example.com/path#fragment");
  EXPECT_EQ(result, "https://example.com/path (PROTECTED)");

  // Test simple URL without sensitive parts
  result = transform->Process("https://example.com/");
  EXPECT_EQ(result, "https://example.com/");

  // Test invalid URL - should be dropped
  result = transform->Process("not-a-url");
  EXPECT_EQ(result, std::nullopt);

  // Test URL with username/password - should be dropped even in relaxed mode
  result = transform->Process("https://user:pass@example.com/");
  EXPECT_EQ(result, std::nullopt);
}

TEST(WebDiscoveryValueTransformTest, SplitTransform) {
  // Test regular split
  auto transform = CreateTransform(R"(["split", ",", 1])");
  ASSERT_TRUE(transform);

  // Test successful split
  auto result = transform->Process("apple,banana,cherry");
  EXPECT_EQ(result, "banana");

  // Test split at index 0
  transform = CreateTransform(R"(["split", ",", 0])");
  ASSERT_TRUE(transform);
  result = transform->Process("apple,banana,cherry");
  EXPECT_EQ(result, "apple");

  // Test valid index with 2 parts
  result = transform->Process(
      "apple,banana");  // 2 parts, asking for index 0 (first part)
  EXPECT_EQ(result, "apple");

  result = transform->Process("apple");
  // Should fail because no split occurred
  EXPECT_EQ(result, std::nullopt);

  result = transform->Process("apple");  // Only 1 part, asking for index 0
  EXPECT_EQ(result, std::nullopt);  // Should fail because no split occurred

  // Test index out of bounds
  transform = CreateTransform(R"(["split", ",", 2])");  // Asking for index 2
  ASSERT_TRUE(transform);
  result = transform->Process(
      "apple,banana");  // Only 2 parts [0,1], index 2 is out of bounds
  EXPECT_EQ(result, std::nullopt);

  // Test string that doesn't contain delimiter
  transform = CreateTransform(R"(["split", ",", 1])");
  ASSERT_TRUE(transform);
  result = transform->Process("apple");
  EXPECT_EQ(result, std::nullopt);
}

TEST(WebDiscoveryValueTransformTest, TrySplitTransform) {
  // Test trySplit
  auto transform = CreateTransform(R"(["trySplit", ",", 1])");
  ASSERT_TRUE(transform);

  // Test successful split
  auto result = transform->Process("apple,banana,cherry");
  EXPECT_EQ(result, "banana");

  // Test index out of bounds (should return original in try mode)
  result = transform->Process("apple");
  EXPECT_EQ(result, "apple");

  // Test string that doesn't contain delimiter (should return original)
  result = transform->Process("apple");
  EXPECT_EQ(result, "apple");
}

TEST(WebDiscoveryValueTransformTest, TrimTransform) {
  // Test trim
  auto transform = CreateTransform(R"(["trim"])");
  ASSERT_TRUE(transform);

  // Test leading and trailing whitespace
  auto result = transform->Process("  hello world  ");
  EXPECT_EQ(result, "hello world");

  // Test only leading whitespace
  result = transform->Process("  hello world");
  EXPECT_EQ(result, "hello world");

  // Test only trailing whitespace
  result = transform->Process("hello world  ");
  EXPECT_EQ(result, "hello world");

  // Test no whitespace
  result = transform->Process("hello world");
  EXPECT_EQ(result, "hello world");

  // Test only whitespace
  result = transform->Process("   ");
  EXPECT_EQ(result, "");

  // Test tabs and newlines
  result = transform->Process("\t\nhello world\n\t");
  EXPECT_EQ(result, "hello world");
}

TEST(WebDiscoveryValueTransformTest, JsonTransform) {
  // Test basic json extraction
  auto transform = CreateTransform(R"(["json", "name"])");
  ASSERT_TRUE(transform);

  // Test string value extraction
  auto result = transform->Process(R"({"name": "John", "age": 30})");
  EXPECT_EQ(result, "John");

  // Test nested path
  transform = CreateTransform(R"(["json", "user.name"])");
  ASSERT_TRUE(transform);
  result = transform->Process(R"({"user": {"name": "Jane", "age": 25}})");
  EXPECT_EQ(result, "Jane");

  // Test integer value
  transform = CreateTransform(R"(["json", "age"])");
  ASSERT_TRUE(transform);
  result = transform->Process(R"({"name": "John", "age": 30})");
  EXPECT_EQ(result, "30");

  // Test boolean value
  transform = CreateTransform(R"(["json", "active"])");
  ASSERT_TRUE(transform);
  result = transform->Process(R"({"active": true})");
  EXPECT_EQ(result, "true");

  result = transform->Process(R"({"active": false})");
  EXPECT_EQ(result, "false");

  // Test missing path
  transform = CreateTransform(R"(["json", "missing"])");
  ASSERT_TRUE(transform);
  result = transform->Process(R"({"name": "John"})");
  EXPECT_EQ(result, "");

  // Test invalid JSON
  result = transform->Process("invalid json");
  EXPECT_EQ(result, "");

  // Test object extraction with extract_objects flag
  transform = CreateTransform(R"(["json", "user", true])");
  ASSERT_TRUE(transform);
  result = transform->Process(R"({"user": {"name": "Jane", "age": 25}})");
  EXPECT_NE(result, std::nullopt);
  // Should return JSON representation of the object
  EXPECT_TRUE(result->find("name") != std::string::npos);
  EXPECT_TRUE(result->find("Jane") != std::string::npos);
}

TEST(WebDiscoveryValueTransformTest, QueryParamTransform) {
  // Test queryParam
  auto transform = CreateTransform(R"(["queryParam", "q"])");
  ASSERT_TRUE(transform);

  // Test simple query parameter extraction
  auto result = transform->Process("q=hello%20world&other=value");
  EXPECT_EQ(result, "hello world");

  // Test parameter not found
  result = transform->Process("other=value&another=test");
  EXPECT_EQ(result, std::nullopt);

  // Test parameter with no value
  result = transform->Process("q=&other=value");
  EXPECT_EQ(result, "");

  // Test parameter at end
  result = transform->Process("other=value&q=test");
  EXPECT_EQ(result, "test");
}

TEST(WebDiscoveryValueTransformTest, RequireURLTransform) {
  // Test requireURL
  auto transform = CreateTransform(R"(["requireURL"])");
  ASSERT_TRUE(transform);

  // Test valid URLs
  auto result = transform->Process("https://example.com");
  EXPECT_EQ(result, "https://example.com");

  result = transform->Process("http://test.com/path?param=value");
  EXPECT_EQ(result, "http://test.com/path?param=value");

  // Test invalid URLs
  result = transform->Process("not-a-url");
  EXPECT_EQ(result, std::nullopt);

  result = transform->Process(
      "ftp://example.com");  // Might be valid depending on GURL implementation
  // Result depends on GURL's validation rules

  result = transform->Process("");
  EXPECT_EQ(result, std::nullopt);
}

TEST(WebDiscoveryValueTransformTest, TransformSequence) {
  // Test a simple sequence of transforms: trim -> split
  std::vector<std::unique_ptr<ValueTransform>> transforms;

  transforms.push_back(CreateTransform(R"(["trim"])"));
  transforms.push_back(CreateTransform(R"(["split", ",", 1])"));

  ASSERT_TRUE(transforms[0]);
  ASSERT_TRUE(transforms[1]);

  // Test successful sequence: trim whitespace then split and get second element
  auto result = ApplyTransforms(transforms, "  apple,banana,cherry  ");
  EXPECT_EQ(result, "banana");

  // Test sequence with no split occurring (should fail at split step)
  result = ApplyTransforms(transforms, "  just-one-element  ");
  EXPECT_EQ(result, std::nullopt);
}

// Test invalid transform definitions
TEST(WebDiscoveryValueTransformTest, InvalidTransformDefinitions) {
  // Test empty definition
  auto transform = CreateTransform("[]");
  EXPECT_FALSE(transform);

  // Test non-string transform name
  transform = CreateTransform("[123]");
  EXPECT_FALSE(transform);

  // Test unknown transform name
  transform = CreateTransform(R"(["unknownTransform"])");
  EXPECT_FALSE(transform);

  // Test invalid split parameters
  transform = CreateTransform(R"(["split", "", 0])");  // Empty delimiter
  EXPECT_FALSE(transform);

  transform = CreateTransform(R"(["split", ",", -1])");  // Negative index
  EXPECT_FALSE(transform);

  // Test invalid filterExact parameters
  transform = CreateTransform(R"(["filterExact", "not-a-list"])");
  EXPECT_FALSE(transform);

  // Test invalid removeParams parameters
  transform = CreateTransform(R"(["removeParams", "not-a-list"])");
  EXPECT_FALSE(transform);
}

}  // namespace web_discovery
