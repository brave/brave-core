/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_response.h"

#include <optional>
#include <type_traits>

#include "base/values.h"
#include "brave/components/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace endpoint_client::internal {

namespace {

// Test cases take their type as a value (`kType<T>`), not as a type template
// argument. Registering typed tests makes gtest call
// `MakeAndRegisterTestInfo()` unqualified, and argument-dependent lookup then
// instantiates the type template arguments of the test cases. For the types
// `Response<>` rejects, that would trigger their `static_assert`. ADL ignores
// non-type template arguments.
template <typename T>
inline constexpr std::type_identity<T> kType{};

struct ValidResponseBody {
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

template <auto TypeIdentity, bool SatisfiesConcept>
struct IsResponseTestCase {
  using Type = typename decltype(TypeIdentity)::type;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsResponseTestCases = testing::Types<
    IsResponseTestCase<kType<void*>, false>,
    IsResponseTestCase<kType<volatile int>, false>,
    IsResponseTestCase<kType<ValidResponseBody>, false>,
    IsResponseTestCase<kType<Response<>>, true>,
    IsResponseTestCase<kType<Response<ValidResponseBody, ValidResponseBody>>,
                       true>,
    IsResponseTestCase<kType<Response<ValidResponseBody>>, false>,
    IsResponseTestCase<kType<Response<ValidResponseBody, int>>, false>,
    IsResponseTestCase<kType<Response<int, ValidResponseBody>>, false>>;

template <typename>
struct IsResponseTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsResponseTest, IsResponseTestCases);

TYPED_TEST(IsResponseTest, SatisfyConcept) {
  EXPECT_EQ(IsResponse<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::internal
