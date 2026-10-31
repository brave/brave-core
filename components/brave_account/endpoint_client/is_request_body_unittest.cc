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

struct NotARequestBody {
  static constexpr char kName[] = "NotARequestBody";
};

struct JSONRequestBodyValid {
  static constexpr char kName[] = "JSONRequestBodyValid";
  base::DictValue ToValue() const;
};

struct ProtobufRequestBodyValid : google::protobuf::MessageLite {
  static constexpr char kName[] = "ProtobufRequestBodyValid";
};

template <typename T, bool SatisfiesConcept>
struct IsRequestBodyTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

// ToValue() and Protobuf edge cases are covered by has_to_value_unittest.cc
// and is_protobuf_message_unittest.cc.
using IsRequestBodyTestCases =
    testing::Types<IsRequestBodyTestCase<NotARequestBody, false>,
                   IsRequestBodyTestCase<JSONRequestBodyValid, true>,
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
