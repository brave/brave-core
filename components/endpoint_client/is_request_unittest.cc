/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_request.h"

#include "brave/components/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/endpoint_client/request_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace endpoint_client::detail {

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
                   IsRequestTestCase<Connect<JSONRequestBody>, true>,
                   IsRequestTestCase<Delete<JSONRequestBody>, true>,
                   IsRequestTestCase<Get<JSONRequestBody>, true>,
                   IsRequestTestCase<Head<JSONRequestBody>, true>,
                   IsRequestTestCase<Options<JSONRequestBody>, true>,
                   IsRequestTestCase<Patch<JSONRequestBody>, true>,
                   IsRequestTestCase<Post<JSONRequestBody>, true>,
                   IsRequestTestCase<Put<JSONRequestBody>, true>,
                   IsRequestTestCase<Trace<JSONRequestBody>, true>,
                   IsRequestTestCase<Track<JSONRequestBody>, true>,
                   IsRequestTestCase<ProtobufRequestBody, false>,
                   IsRequestTestCase<Connect<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Delete<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Get<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Head<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Options<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Patch<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Post<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Put<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Trace<ProtobufRequestBody>, true>,
                   IsRequestTestCase<Track<ProtobufRequestBody>, true>>;

template <typename>
struct IsRequestTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsRequestTest, IsRequestTestCases);

TYPED_TEST(IsRequestTest, SatisfyConcept) {
  EXPECT_EQ(IsRequest<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::detail
