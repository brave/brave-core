/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_request_body.h"

#include <string>

#include "base/strings/strcat.h"
#include "google/protobuf/message_lite.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class DictValue;
}  // namespace base

namespace brave_account::endpoint_client::detail {

namespace {

struct JSONRequestBodyNoToValue {
  static constexpr char kName[] = "JSONRequestBodyNoToValue";
};

struct JSONRequestBodyStaticToValue {
  static constexpr char kName[] = "JSONRequestBodyStaticToValue";
  static base::DictValue ToValue();
};

struct JSONRequestBodyNonConstToValue {
  static constexpr char kName[] = "JSONRequestBodyNonConstToValue";
  base::DictValue ToValue();
};

struct JSONRequestBodyToValueWithWrongReturnType {
  static constexpr char kName[] = "JSONRequestBodyToValueWithWrongReturnType";
  std::string ToValue() const;
};

struct JSONRequestBodyToValueWithWrongParameterType {
  static constexpr char kName[] =
      "JSONRequestBodyToValueWithWrongParameterType";
  base::DictValue ToValue(int) const;
};

struct JSONRequestBodyValid {
  static constexpr char kName[] = "JSONRequestBodyValid";
  base::DictValue ToValue() const;
};

class ProtobufRequestBodyPrivateInheritance : google::protobuf::MessageLite {
 public:
  static constexpr char kName[] = "ProtobufRequestBodyPrivateInheritance";
};

struct ProtobufRequestBodyValid : google::protobuf::MessageLite {
  static constexpr char kName[] = "ProtobufRequestBodyValid";
};

template <typename T, bool SatisfiesConcept>
struct IsRequestBodyTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsRequestBodyTestCases = testing::Types<
    IsRequestBodyTestCase<JSONRequestBodyNoToValue, false>,
    IsRequestBodyTestCase<JSONRequestBodyStaticToValue, false>,
    IsRequestBodyTestCase<JSONRequestBodyNonConstToValue, false>,
    IsRequestBodyTestCase<JSONRequestBodyToValueWithWrongReturnType, false>,
    IsRequestBodyTestCase<JSONRequestBodyToValueWithWrongParameterType, false>,
    IsRequestBodyTestCase<JSONRequestBodyValid, true>,
    IsRequestBodyTestCase<ProtobufRequestBodyPrivateInheritance, false>,
    IsRequestBodyTestCase<ProtobufRequestBodyValid, true>>;

struct IsRequestBodyTestCaseName {
  template <typename IsRequestBodyTestCase>
  static std::string GetName(int) {
    return base::StrCat(
        {IsRequestBodyTestCase::Type::kName, "_does",
         (IsRequestBodyTestCase::kSatisfiesConcept ? "" : "_not")});
  }
};

template <typename>
struct IsRequestBodyTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsRequestBodyTest,
                 IsRequestBodyTestCases,
                 IsRequestBodyTestCaseName);

TYPED_TEST(IsRequestBodyTest, SatisfyConcept) {
  EXPECT_EQ(IsRequestBody<typename TypeParam::Type>,
            TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::detail
