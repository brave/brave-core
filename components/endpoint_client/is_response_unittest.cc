/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_response.h"

#include <optional>

#include "base/values.h"
#include "brave/components/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace endpoint_client::internal {

namespace {

struct ValidResponseBody {
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

template <typename T, bool SatisfiesConcept>
struct IsResponseTestCase {
  using Type = T;
  static constexpr bool kSatisfiesConcept = SatisfiesConcept;
};

using IsResponseTestCases = testing::Types<
    IsResponseTestCase<void*, false>,
    IsResponseTestCase<volatile int, false>,
    IsResponseTestCase<ValidResponseBody, false>,
    IsResponseTestCase<Response<ValidResponseBody, ValidResponseBody>, true>>;

template <typename>
struct IsResponseTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsResponseTest, IsResponseTestCases);

TYPED_TEST(IsResponseTest, SatisfyConcept) {
  EXPECT_EQ(IsResponse<typename TypeParam::Type>, TypeParam::kSatisfiesConcept);
}

}  // namespace endpoint_client::internal
