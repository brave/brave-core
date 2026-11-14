/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_request.h"

#include <type_traits>

#include "base/values.h"
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

struct ValidRequestBody {
  base::DictValue ToValue() const;
};

template <auto TypeIdentity, bool SatisfiesConcept>
struct IsRequestTestCase {
  using Type = typename decltype(TypeIdentity)::type;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsRequestTestCases = testing::Types<
    IsRequestTestCase<kType<void*>, false>,
    IsRequestTestCase<kType<volatile int>, false>,
    IsRequestTestCase<kType<ValidRequestBody>, false>,
    IsRequestTestCase<kType<Connect<>>, true>,
    IsRequestTestCase<kType<Delete<>>, true>,
    IsRequestTestCase<kType<Get>, true>,
    IsRequestTestCase<kType<Head>, true>,
    IsRequestTestCase<kType<Options<>>, true>,
    IsRequestTestCase<kType<Patch<>>, true>,
    IsRequestTestCase<kType<Post<>>, true>,
    IsRequestTestCase<kType<Put<>>, true>,
    IsRequestTestCase<kType<Trace<>>, true>,
    IsRequestTestCase<kType<Track<>>, true>,
    IsRequestTestCase<kType<Connect<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Delete<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Options<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Patch<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Post<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Put<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Trace<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Track<ValidRequestBody>>, true>,
    IsRequestTestCase<kType<Request<Method::kGet, ValidRequestBody>>, false>,
    IsRequestTestCase<kType<Request<Method::kHead, ValidRequestBody>>, false>,
    IsRequestTestCase<kType<Request<Method::kPost, int>>, false>>;

template <typename>
struct IsRequestTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsRequestTest, IsRequestTestCases);

TYPED_TEST(IsRequestTest, SatisfyConcept) {
  EXPECT_EQ(IsRequest<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::internal
