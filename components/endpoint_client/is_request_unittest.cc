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
    IsRequestTestCase<kType<JSONNonEmptyRequestBody>, false>,
    IsRequestTestCase<kType<Connect<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Delete<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Options<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Patch<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Post<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Put<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Trace<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Track<JSONNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Request<Method::kGet, JSONNonEmptyRequestBody>>,
                      false>,
    IsRequestTestCase<kType<Request<Method::kHead, JSONNonEmptyRequestBody>>,
                      false>,
    IsRequestTestCase<kType<ProtobufNonEmptyRequestBody>, false>,
    IsRequestTestCase<kType<Connect<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Delete<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Options<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Patch<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Post<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Put<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Trace<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Track<ProtobufNonEmptyRequestBody>>, true>,
    IsRequestTestCase<kType<Request<Method::kGet, ProtobufNonEmptyRequestBody>>,
                      false>,
    IsRequestTestCase<
        kType<Request<Method::kHead, ProtobufNonEmptyRequestBody>>,
        false>>;

template <typename>
struct IsRequestTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsRequestTest, IsRequestTestCases);

TYPED_TEST(IsRequestTest, SatisfyConcept) {
  EXPECT_EQ(IsRequest<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::internal
