/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_response.h"

#include <optional>
#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/concept_test.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

namespace {

struct ValidResponseBody {
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

template <typename T>
using IsResponseConceptTest = ConceptTest::Fixture<T>;

using ResponseTestTypes = testing::Types<
    std::tuple<void*, std::false_type>,
    std::tuple<volatile int, std::false_type>,
    std::tuple<ValidResponseBody, std::false_type>,
    std::tuple<Response<ValidResponseBody, ValidResponseBody>, std::true_type>>;

}  // namespace

TYPED_TEST_SUITE(IsResponseConceptTest, ResponseTestTypes);

TYPED_TEST(IsResponseConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(IsResponse<TestType>, ExpectedResult::value);
}

}  // namespace brave_account::endpoint_client::detail
