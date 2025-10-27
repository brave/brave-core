/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_request_body.h"

#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/concept_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

struct RequestBodyNoToValue {
  static constexpr char kName[] = "RequestBodyNoToValue";
};

struct RequestBodyStaticToValue {
  static constexpr char kName[] = "RequestBodyStaticToValue";
  static base::Value::Dict ToValue();
};

struct RequestBodyToValueWithWrongReturnType {
  static constexpr char kName[] = "RequestBodyToValueWithWrongReturnType";
  void ToValue() const;
};

struct RequestBodyToValueWithWrongParameterType {
  static constexpr char kName[] = "RequestBodyToValueWithWrongParameterType";
  base::Value::Dict ToValue(int) const;
};

struct ValidRequestBody {
  static constexpr char kName[] = "ValidRequestBody";
  base::Value::Dict ToValue() const;
};

template <typename T>
using IsRequestBodyConceptTest = ConceptTest::Fixture<T>;

using RequestBodyTestTypes = testing::Types<
    std::tuple<RequestBodyNoToValue, std::false_type>,
    std::tuple<RequestBodyStaticToValue, std::false_type>,
    std::tuple<RequestBodyToValueWithWrongReturnType, std::false_type>,
    std::tuple<RequestBodyToValueWithWrongParameterType, std::false_type>,
    std::tuple<ValidRequestBody, std::true_type>>;

TYPED_TEST_SUITE(IsRequestBodyConceptTest,
                 RequestBodyTestTypes,
                 ConceptTest::NameGenerator);

TYPED_TEST(IsRequestBodyConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(IsRequestBody<TestType>, ExpectedResult::value);
}

}  // namespace brave_account::endpoint_client::detail
