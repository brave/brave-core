/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_response.h"

#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

namespace {

template <typename T, bool SatisfiesConcept>
struct IsResponseTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsResponseTestCases = testing::Types<
    IsResponseTestCase<void*, false>,
    IsResponseTestCase<volatile int, false>,
    IsResponseTestCase<JSONSuccessBody, false>,
    IsResponseTestCase<Response<JSONSuccessBody, JSONErrorBody>, true>,
    IsResponseTestCase<ProtobufSuccessBody, false>,
    IsResponseTestCase<Response<ProtobufSuccessBody, ProtobufErrorBody>, true>,
    IsResponseTestCase<Response<JSONSuccessBody, ProtobufErrorBody>, false>>;

template <typename>
struct IsResponseTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsResponseTest, IsResponseTestCases);

TYPED_TEST(IsResponseTest, SatisfyConcept) {
  EXPECT_EQ(IsResponse<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::detail
