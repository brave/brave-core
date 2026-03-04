/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_request.h"

#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

namespace {

template <typename T, bool SatisfiesConcept>
struct IsRequestTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsRequestTestCases =
    testing::Types<IsRequestTestCase<void*, false>,
                   IsRequestTestCase<volatile int, false>,
                   IsRequestTestCase<JSONRequestBody, false>,
                   IsRequestTestCase<CONNECT<JSONRequestBody>, true>,
                   IsRequestTestCase<DELETE<JSONRequestBody>, true>,
                   IsRequestTestCase<GET<JSONRequestBody>, true>,
                   IsRequestTestCase<HEAD<JSONRequestBody>, true>,
                   IsRequestTestCase<OPTIONS<JSONRequestBody>, true>,
                   IsRequestTestCase<PATCH<JSONRequestBody>, true>,
                   IsRequestTestCase<POST<JSONRequestBody>, true>,
                   IsRequestTestCase<PUT<JSONRequestBody>, true>,
                   IsRequestTestCase<TRACE<JSONRequestBody>, true>,
                   IsRequestTestCase<TRACK<JSONRequestBody>, true>,
                   IsRequestTestCase<ProtobufRequestBody, false>,
                   IsRequestTestCase<CONNECT<ProtobufRequestBody>, true>,
                   IsRequestTestCase<DELETE<ProtobufRequestBody>, true>,
                   IsRequestTestCase<GET<ProtobufRequestBody>, true>,
                   IsRequestTestCase<HEAD<ProtobufRequestBody>, true>,
                   IsRequestTestCase<OPTIONS<ProtobufRequestBody>, true>,
                   IsRequestTestCase<PATCH<ProtobufRequestBody>, true>,
                   IsRequestTestCase<POST<ProtobufRequestBody>, true>,
                   IsRequestTestCase<PUT<ProtobufRequestBody>, true>,
                   IsRequestTestCase<TRACE<ProtobufRequestBody>, true>,
                   IsRequestTestCase<TRACK<ProtobufRequestBody>, true>>;

template <typename>
struct IsRequestTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsRequestTest, IsRequestTestCases);

TYPED_TEST(IsRequestTest, SatisfyConcept) {
  EXPECT_EQ(IsRequest<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::detail
