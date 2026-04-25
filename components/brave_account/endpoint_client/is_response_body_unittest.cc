/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_response_body.h"

#include <optional>
#include <string>

#include "base/strings/strcat.h"
#include "google/protobuf/message_lite.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class Value;
}  // namespace base

namespace brave_account::endpoint_client::detail {

namespace {

struct JSONResponseBodyNoFromValue {
  static constexpr char kName[] = "JSONResponseBodyNoFromValue";
};

struct JSONResponseBodyNonStaticFromValue {
  static constexpr char kName[] = "JSONResponseBodyNonStaticFromValue";
  std::optional<JSONResponseBodyNonStaticFromValue> FromValue(
      const base::Value&);
};

struct JSONResponseBodyFromValueWithWrongReturnType {
  static constexpr char kName[] =
      "JSONResponseBodyFromValueWithWrongReturnType";
  static JSONResponseBodyFromValueWithWrongReturnType FromValue(
      const base::Value&);
};

struct JSONResponseBodyFromValueWithWrongParameterType {
  static constexpr char kName[] =
      "JSONResponseBodyFromValueWithWrongParameterType";
  static std::optional<JSONResponseBodyFromValueWithWrongParameterType>
  FromValue(const std::string&);
};

struct JSONResponseBodyValid {
  static constexpr char kName[] = "JSONResponseBodyValid";
  static std::optional<JSONResponseBodyValid> FromValue(const base::Value&);
};

class ProtobufResponseBodyPrivateInheritance : google::protobuf::MessageLite {
 public:
  static constexpr char kName[] = "ProtobufResponseBodyPrivateInheritance";
};

struct ProtobufResponseBodyValid : google::protobuf::MessageLite {
  static constexpr char kName[] = "ProtobufResponseBodyValid";
};

template <typename T, bool SatisfiesConcept>
struct IsResponseBodyTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsResponseBodyTestCases = testing::Types<
    IsResponseBodyTestCase<JSONResponseBodyNoFromValue, false>,
    IsResponseBodyTestCase<JSONResponseBodyNonStaticFromValue, false>,
    IsResponseBodyTestCase<JSONResponseBodyFromValueWithWrongReturnType, false>,
    IsResponseBodyTestCase<JSONResponseBodyFromValueWithWrongParameterType,
                           false>,
    IsResponseBodyTestCase<JSONResponseBodyValid, true>,
    IsResponseBodyTestCase<ProtobufResponseBodyPrivateInheritance, false>,
    IsResponseBodyTestCase<ProtobufResponseBodyValid, true>>;

struct IsResponseBodyTestCaseName {
  template <typename IsResponseBodyTestCase>
  static std::string GetName(int) {
    return base::StrCat(
        {IsResponseBodyTestCase::Type::kName, "_does",
         (IsResponseBodyTestCase::kSatisfiesConcept ? "" : "_not")});
  }
};

template <typename>
struct IsResponseBodyTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsResponseBodyTest,
                 IsResponseBodyTestCases,
                 IsResponseBodyTestCaseName);

TYPED_TEST(IsResponseBodyTest, SatisfyConcept) {
  EXPECT_EQ(IsResponseBody<typename TypeParam::Type>,
            TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::detail
