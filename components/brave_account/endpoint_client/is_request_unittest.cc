/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_request.h"

#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/concept_test.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

namespace {

struct ValidRequestBody {
  base::Value::Dict ToValue() const;
};

template <typename T>
using IsRequestConceptTest = ConceptTest::Fixture<T>;

using RequestTestTypes =
    testing::Types<std::tuple<void*, std::false_type>,
                   std::tuple<volatile int, std::false_type>,
                   std::tuple<ValidRequestBody, std::false_type>,
                   std::tuple<CONNECT<ValidRequestBody>, std::true_type>,
                   std::tuple<DELETE<ValidRequestBody>, std::true_type>,
                   std::tuple<GET<ValidRequestBody>, std::true_type>,
                   std::tuple<HEAD<ValidRequestBody>, std::true_type>,
                   std::tuple<OPTIONS<ValidRequestBody>, std::true_type>,
                   std::tuple<PATCH<ValidRequestBody>, std::true_type>,
                   std::tuple<POST<ValidRequestBody>, std::true_type>,
                   std::tuple<PUT<ValidRequestBody>, std::true_type>,
                   std::tuple<TRACE<ValidRequestBody>, std::true_type>,
                   std::tuple<TRACK<ValidRequestBody>, std::true_type>>;

}  // namespace

TYPED_TEST_SUITE(IsRequestConceptTest, RequestTestTypes);

TYPED_TEST(IsRequestConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(IsRequest<TestType>, ExpectedResult::value);
}

}  // namespace brave_account::endpoint_client::detail
