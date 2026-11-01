/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/has_to_value.h"

#include <string>

#include "base/strings/strcat.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class DictValue;
}  // namespace base

namespace brave_account::endpoint_client::detail {

namespace {

struct NoToValue {
  static constexpr char kName[] = "NoToValue";
};

struct StaticToValue {
  static constexpr char kName[] = "StaticToValue";
  static base::DictValue ToValue();
};

struct NonConstToValue {
  static constexpr char kName[] = "NonConstToValue";
  base::DictValue ToValue();
};

struct ToValueWithWrongReturnType {
  static constexpr char kName[] = "ToValueWithWrongReturnType";
  std::string ToValue() const;
};

struct ToValueWithWrongParameterType {
  static constexpr char kName[] = "ToValueWithWrongParameterType";
  base::DictValue ToValue(int) const;
};

struct OverloadedToValue {
  static constexpr char kName[] = "OverloadedToValue";
  base::DictValue ToValue() const;
  base::DictValue ToValue(int) const;
};

struct ValidToValue {
  static constexpr char kName[] = "ValidToValue";
  base::DictValue ToValue() const;
};

template <typename T, bool SatisfiesConcept>
struct HasToValueTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using HasToValueTestCases =
    testing::Types<HasToValueTestCase<NoToValue, false>,
                   HasToValueTestCase<StaticToValue, false>,
                   HasToValueTestCase<NonConstToValue, false>,
                   HasToValueTestCase<ToValueWithWrongReturnType, false>,
                   HasToValueTestCase<ToValueWithWrongParameterType, false>,
                   HasToValueTestCase<OverloadedToValue, false>,
                   HasToValueTestCase<ValidToValue, true>>;

struct HasToValueTestCaseName {
  template <typename HasToValueTestCase>
  static std::string GetName(int) {
    return base::StrCat(
        {HasToValueTestCase::Type::kName, "_does",
         (HasToValueTestCase::kSatisfiesConcept ? "" : "_not")});
  }
};

template <typename>
struct HasToValueTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(HasToValueTest, HasToValueTestCases, HasToValueTestCaseName);

TYPED_TEST(HasToValueTest, SatisfyConcept) {
  EXPECT_EQ(HasToValue<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::detail
