/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_stream.h"

#include <optional>
#include <type_traits>

#include "base/values.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/endpoint_client/stream.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace endpoint_client::internal {

namespace {

// See is_response_unittest.cc for why test cases take their type as a value.
template <typename T>
inline constexpr std::type_identity<T> kType{};

struct ValidResponseBody {
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

template <auto TypeIdentity, bool SatisfiesConcept>
struct IsStreamTestCase {
  using Type = typename decltype(TypeIdentity)::type;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsStreamTestCases = testing::Types<
    IsStreamTestCase<kType<void*>, false>,
    IsStreamTestCase<kType<ValidResponseBody>, false>,
    IsStreamTestCase<kType<Response<>>, false>,
    IsStreamTestCase<kType<Response<ValidResponseBody, ValidResponseBody>>,
                     false>,
    IsStreamTestCase<kType<Stream<>>, true>,
    IsStreamTestCase<kType<Stream<ValidResponseBody>>, true>,
    IsStreamTestCase<kType<Stream<int>>, false>,
    IsStreamTestCase<kType<Stream<ValidResponseBody, ValidResponseBody>>,
                     false>>;

template <typename>
struct IsStreamTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsStreamTest, IsStreamTestCases);

TYPED_TEST(IsStreamTest, SatisfyConcept) {
  EXPECT_EQ(IsStream<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::internal
