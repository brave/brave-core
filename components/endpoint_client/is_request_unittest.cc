/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_request.h"

#include <type_traits>

#include "brave/components/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/endpoint_client/request_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace endpoint_client::internal {

namespace {

// Test cases take their type as a value (`kType<T>`), not as a type template
// argument. Registering typed tests makes gtest call
// `MakeAndRegisterTestInfo()` unqualified, and argument-dependent lookup then
// instantiates the type template arguments of the test cases. For the types
// `Request<>` rejects, that would trigger their `static_assert`. ADL ignores
// non-type template arguments.
template <typename T>
inline constexpr std::type_identity<T> kType{};

template <auto TypeIdentity, bool SatisfiesConcept>
struct IsRequestTestCase {
  using Type = typename decltype(TypeIdentity)::type;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsRequestTestCases = testing::Types<
    IsRequestTestCase<kType<void*>, false>,
    IsRequestTestCase<kType<volatile int>, false>,
    IsRequestTestCase<kType<Get>, true>,
    IsRequestTestCase<kType<Head>, true>,
    IsRequestTestCase<kType<Delete<>>, true>,
    IsRequestTestCase<kType<Post<>>, true>,
    IsRequestTestCase<kType<JSONRequestBody>, false>,
    IsRequestTestCase<kType<Connect<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Delete<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Options<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Patch<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Post<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Put<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Trace<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Track<JSONRequestBody>>, true>,
    IsRequestTestCase<kType<Request<Method::kGet, JSONRequestBody>>, false>,
    IsRequestTestCase<kType<Request<Method::kHead, JSONRequestBody>>, false>,
    IsRequestTestCase<kType<ProtobufRequestBody>, false>,
    IsRequestTestCase<kType<Connect<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Delete<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Options<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Patch<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Post<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Put<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Trace<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Track<ProtobufRequestBody>>, true>,
    IsRequestTestCase<kType<Request<Method::kGet, ProtobufRequestBody>>, false>,
    IsRequestTestCase<kType<Request<Method::kHead, ProtobufRequestBody>>,
                      false>>;

template <typename>
struct IsRequestTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsRequestTest, IsRequestTestCases);

TYPED_TEST(IsRequestTest, SatisfyConcept) {
  EXPECT_EQ(IsRequest<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::internal
