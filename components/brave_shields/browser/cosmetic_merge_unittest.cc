/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/json/json_reader.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

using ::testing::_;

class CosmeticResourceMergeTest : public testing::Test {
 public:
  CosmeticResourceMergeTest() {}
  ~CosmeticResourceMergeTest() override {}

  void CompareMergeFromStrings(
          const std::string& a,
          const std::string& b,
          bool force_hide,
          const std::string& expected) {
    base::Optional<base::Value> a_val = base::JSONReader::Read(a);
    ASSERT_TRUE(a_val);

    base::Optional<base::Value> b_val = base::JSONReader::Read(b);
    ASSERT_TRUE(b_val);

    const base::Optional<base::Value> expected_val =
        base::JSONReader::Read(expected);
    ASSERT_TRUE(expected_val);

    MergeResourcesInto(&*a_val, &*b_val, force_hide);

    ASSERT_EQ(*a_val, *expected_val);
  }

 protected:
  void SetUp() override {}

  void TearDown() override {}
};

const char EMPTY_RESOURCES[] = "{"
    "\"hide_selectors\": [], "
    "\"style_selectors\": {}, "
    "\"exceptions\": [], "
    "\"injected_script\": \"\""
"}";

const char NONEMPTY_RESOURCES[] = "{"
    "\"hide_selectors\": [\"a\", \"b\"], "
    "\"style_selectors\": {\"c\": \"color: #fff\", \"d\": \"color: #000\"}, "
    "\"exceptions\": [\"e\", \"f\"], "
    "\"injected_script\": \"console.log('g')\""
"}";

TEST_F(CosmeticResourceMergeTest, MergeTwoEmptyResources) {
  const std::string a = EMPTY_RESOURCES;
  const std::string b = EMPTY_RESOURCES;

  // Same as EMPTY_RESOURCES, but with an additional newline in the
  // injected_script
  const std::string expected = "{"
      "\"hide_selectors\": [], "
      "\"style_selectors\": {}, "
      "\"exceptions\": [], "
      "\"injected_script\": \"\n\""
  "}";

  CompareMergeFromStrings(a, b, false, expected);
}

TEST_F(CosmeticResourceMergeTest, MergeEmptyIntoNonEmpty) {
  const std::string a = NONEMPTY_RESOURCES;
  const std::string b = EMPTY_RESOURCES;

  // Same as a, but with an additional newline at the end of the
  // injected_script
  const std::string expected = "{"
      "\"hide_selectors\": [\"a\", \"b\"], "
      "\"style_selectors\": {\"c\": \"color: #fff\", \"d\": \"color: #000\"}, "
      "\"exceptions\": [\"e\", \"f\"], "
      "\"injected_script\": \"console.log('g')\n\""
  "}";

  CompareMergeFromStrings(a, b, false, expected);
}

TEST_F(CosmeticResourceMergeTest, MergeNonEmptyIntoEmpty) {
  const std::string a = EMPTY_RESOURCES;
  const std::string b = NONEMPTY_RESOURCES;

  // Same as b, but with an additional newline at the beginning of the
  // injected_script
  const std::string expected = "{"
      "\"hide_selectors\": [\"a\", \"b\"],"
      "\"style_selectors\": {\"c\": \"color: #fff\", \"d\": \"color: #000\"}, "
      "\"exceptions\": [\"e\", \"f\"], "
      "\"injected_script\": \"\nconsole.log('g')\""
  "}";

  CompareMergeFromStrings(a, b, false, expected);
}

TEST_F(CosmeticResourceMergeTest, MergeNonEmptyIntoNonEmpty) {
  const std::string a = NONEMPTY_RESOURCES;
  const std::string b = "{"
      "\"hide_selectors\": [\"h\", \"i\"], "
      "\"style_selectors\": {\"j\": \"color: #eee\", \"k\": \"color: #111\"}, "
      "\"exceptions\": [\"l\", \"m\"], "
      "\"injected_script\": \"console.log('n')\""
  "}";

  const std::string expected = "{"
      "\"hide_selectors\": [\"a\", \"b\", \"h\", \"i\"], "
      "\"style_selectors\": {"
          "\"c\": \"color: #fff\", "
          "\"d\": \"color: #000\", "
          "\"j\": \"color: #eee\", "
          "\"k\": \"color: #111\""
      "}, "
      "\"exceptions\": [\"e\", \"f\", \"l\", \"m\"], "
      "\"injected_script\": \"console.log('g')\nconsole.log('n')\""
  "}";

  CompareMergeFromStrings(a, b, false, expected);
}

TEST_F(CosmeticResourceMergeTest, MergeEmptyForceHide) {
  const std::string a = EMPTY_RESOURCES;
  const std::string b = EMPTY_RESOURCES;

  // Same as EMPTY_RESOURCES, but with an additional newline in the
  // injected_script and a new empty `force_hide_selectors` array
  const std::string expected = "{"
      "\"hide_selectors\": [], "
      "\"style_selectors\": {}, "
      "\"exceptions\": [], "
      "\"injected_script\": \"\n\","
      "\"force_hide_selectors\": []"
  "}";

  CompareMergeFromStrings(a, b, true, expected);
}

TEST_F(CosmeticResourceMergeTest, MergeNonEmptyForceHide) {
  const std::string a = NONEMPTY_RESOURCES;
  const std::string b = "{"
      "\"hide_selectors\": [\"h\", \"i\"], "
      "\"style_selectors\": {\"j\": \"color: #eee\", \"k\": \"color: #111\"}, "
      "\"exceptions\": [\"l\", \"m\"], "
      "\"injected_script\": \"console.log('n')\""
  "}";

  const std::string expected = "{"
      "\"hide_selectors\": [\"a\", \"b\"], "
      "\"style_selectors\": {"
          "\"c\": \"color: #fff\", "
          "\"d\": \"color: #000\", "
          "\"j\": \"color: #eee\", "
          "\"k\": \"color: #111\""
      "}, "
      "\"exceptions\": [\"e\", \"f\", \"l\", \"m\"], "
      "\"injected_script\": \"console.log('g')\nconsole.log('n')\","
      "\"force_hide_selectors\": [\"h\", \"i\"]"
  "}";

  CompareMergeFromStrings(a, b, true, expected);
}


}  // namespace brave_shields
