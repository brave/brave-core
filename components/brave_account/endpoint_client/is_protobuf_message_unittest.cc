/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_protobuf_message.h"

#include <string>

#include "base/strings/strcat.h"
#include "google/protobuf/message_lite.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

namespace {

struct NotDerived {
  static constexpr char kName[] = "NotDerived";
};

class PrivateInheritance : google::protobuf::MessageLite {
 public:
  static constexpr char kName[] = "PrivateInheritance";
};

struct PublicInheritance : google::protobuf::MessageLite {
  static constexpr char kName[] = "PublicInheritance";
};

template <typename T, bool SatisfiesConcept>
struct IsProtobufMessageTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsProtobufMessageTestCases =
    testing::Types<IsProtobufMessageTestCase<NotDerived, false>,
                   IsProtobufMessageTestCase<PrivateInheritance, false>,
                   IsProtobufMessageTestCase<PublicInheritance, true>>;

struct IsProtobufMessageTestCaseName {
  template <typename IsProtobufMessageTestCase>
  static std::string GetName(int) {
    return base::StrCat(
        {IsProtobufMessageTestCase::Type::kName, "_does",
         (IsProtobufMessageTestCase::kSatisfiesConcept ? "" : "_not")});
  }
};

template <typename>
struct IsProtobufMessageTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsProtobufMessageTest,
                 IsProtobufMessageTestCases,
                 IsProtobufMessageTestCaseName);

TYPED_TEST(IsProtobufMessageTest, SatisfyConcept) {
  EXPECT_EQ(IsProtobufMessage<typename TypeParam::Type>,
            TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::detail
