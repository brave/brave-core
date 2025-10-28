/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_response_body.h"

#include <optional>
#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/concept_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client::detail {

struct ResponseBodyNoFromValue {
  static constexpr char kName[] = "ResponseBodyNoFromValue";
};

struct ResponseBodyNonStaticFromValue {
  static constexpr char kName[] = "ResponseBodyNonStaticFromValue";
  std::optional<ResponseBodyNonStaticFromValue> FromValue(const base::Value&);
};

struct ResponseBodyFromValueWithWrongReturnType {
  static constexpr char kName[] = "ResponseBodyFromValueWithWrongReturnType";
  static ResponseBodyFromValueWithWrongReturnType FromValue(const base::Value&);
};

struct ResponseBodyFromValueWithWrongParameterType {
  static constexpr char kName[] = "ResponseBodyFromValueWithWrongParameterType";
  static std::optional<ResponseBodyFromValueWithWrongParameterType> FromValue(
      int);
};

struct ValidResponseBody {
  static constexpr char kName[] = "ValidResponseBody";
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

template <typename T>
using IsResponseBodyConceptTest = ConceptTest::Fixture<T>;

using ResponseBodyTestTypes = testing::Types<
    std::tuple<ResponseBodyNoFromValue, std::false_type>,
    std::tuple<ResponseBodyNonStaticFromValue, std::false_type>,
    std::tuple<ResponseBodyFromValueWithWrongReturnType, std::false_type>,
    std::tuple<ResponseBodyFromValueWithWrongParameterType, std::false_type>,
    std::tuple<ValidResponseBody, std::true_type>>;

TYPED_TEST_SUITE(IsResponseBodyConceptTest,
                 ResponseBodyTestTypes,
                 ConceptTest::NameGenerator);

TYPED_TEST(IsResponseBodyConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(IsResponseBody<TestType>, ExpectedResult::value);
}

}  // namespace brave_account::endpoint_client::detail
