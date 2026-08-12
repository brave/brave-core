/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/json_array_of.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/check_deref.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/test_endpoint_bodies_equality.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client {

template <typename T>
bool operator==(const JSONArrayOf<T>& lhs, const JSONArrayOf<T>& rhs) {
  return lhs.items == rhs.items;
}

namespace {

using Array = JSONArrayOf<JSONSuccessBody>;

// { "success": <value> }
base::DictValue Element(std::string_view value) {
  return base::DictValue().Set("success", value);
}

JSONSuccessBody ParsedElement(std::string_view value) {
  JSONSuccessBody body;
  body.success = value;
  return body;
}

struct TestCase {
  std::string test_name;
  base::Value value;
  std::optional<Array> expected;
};

const TestCase* NotAList() {
  static const base::NoDestructor<TestCase> kTestCase(
      {.test_name = "not_a_list",
       .value = base::Value(Element("one")),
       .expected = std::nullopt});
  return kTestCase.get();
}

const TestCase* EmptyArray() {
  static const base::NoDestructor<TestCase> kTestCase(
      {.test_name = "empty_array",
       .value = base::Value(base::ListValue()),
       .expected = Array()});
  return kTestCase.get();
}

const TestCase* Elements() {
  static const base::NoDestructor<TestCase> kTestCase(
      {.test_name = "elements",
       .value = base::Value(
           base::ListValue().Append(Element("one")).Append(Element("two"))),
       .expected = [] {
         Array array;
         array.items.push_back(ParsedElement("one"));
         array.items.push_back(ParsedElement("two"));
         return array;
       }()});
  return kTestCase.get();
}

const TestCase* InvalidElement() {
  static const base::NoDestructor<TestCase> kTestCase(
      {.test_name = "invalid_element",
       .value = base::Value(base::ListValue().Append(Element("one")).Append(2)),
       .expected = std::nullopt});
  return kTestCase.get();
}

using JSONArrayOfTest = testing::TestWithParam<const TestCase*>;

}  // namespace

TEST_P(JSONArrayOfTest, FromValue) {
  const TestCase& test_case = CHECK_DEREF(GetParam());
  EXPECT_EQ(Array::FromValue(test_case.value), test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    JSONArrayOfTestCases,
    JSONArrayOfTest,
    testing::Values(NotAList(), EmptyArray(), Elements(), InvalidElement()),
    [](const auto& info) { return CHECK_DEREF(info.param).test_name; });

TEST(JSONArrayOfNestingTest, FromValue) {
  std::optional<JSONArrayOf<Array>> nested = JSONArrayOf<Array>::FromValue(
      base::Value(base::ListValue()
                      .Append(base::ListValue().Append(Element("one")))
                      .Append(base::ListValue())));
  ASSERT_TRUE(nested);
  ASSERT_EQ(nested->items.size(), 2u);
  ASSERT_EQ(nested->items[0].items.size(), 1u);
  EXPECT_EQ(nested->items[0].items[0].success, "one");
  EXPECT_TRUE(nested->items[1].items.empty());
}

}  // namespace brave_account::endpoint_client
