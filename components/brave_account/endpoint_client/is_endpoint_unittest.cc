/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_endpoint.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/concept_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

struct InvalidRequestBody {};

struct ValidRequestBody {
  base::Value::Dict ToValue() const;
};

struct InvalidResponseBody {};

struct ValidResponseBody {
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

struct InvalidErrorBody {};

struct ValidErrorBody {
  static std::optional<ValidErrorBody> FromValue(const base::Value&);
};

struct EndpointInvalidRequestBody {
  static constexpr char kName[] = "EndpointInvalidRequestBody";
  using Request = InvalidRequestBody;
  using Response = ValidResponseBody;
  using Error = ValidErrorBody;
  static GURL URL();
  static std::string_view Method();
};

struct EndpointInvalidResponseBody {
  static constexpr char kName[] = "EndpointInvalidResponseBody";
  using Request = ValidRequestBody;
  using Response = InvalidResponseBody;
  using Error = ValidErrorBody;
  static GURL URL();
  static std::string_view Method();
};

struct EndpointInvalidErrorBody {
  static constexpr char kName[] = "EndpointInvalidErrorBody";
  using Request = ValidRequestBody;
  using Response = ValidResponseBody;
  using Error = InvalidErrorBody;
  static GURL URL();
  static std::string_view Method();
};

struct EndpointInvalidURL {
  static constexpr char kName[] = "EndpointInvalidURL";
  using Request = ValidRequestBody;
  using Response = ValidResponseBody;
  using Error = ValidErrorBody;
  static std::string URL();
  static std::string_view Method();
};

struct EndpointInvalidMethod {
  static constexpr char kName[] = "EndpointInvalidMethod";
  using Request = ValidRequestBody;
  using Response = ValidResponseBody;
  using Error = ValidErrorBody;
  static GURL URL();
  static bool Method();
};

template <typename T>
using IsEndpointConceptTest = ConceptTest::Fixture<T>;

// Hand-crafted invalid endpoints.
using EndpointTestTypes =
    testing::Types<std::tuple<EndpointInvalidRequestBody, std::false_type>,
                   std::tuple<EndpointInvalidResponseBody, std::false_type>,
                   std::tuple<EndpointInvalidErrorBody, std::false_type>,
                   std::tuple<EndpointInvalidURL, std::false_type>,
                   std::tuple<EndpointInvalidMethod, std::false_type>>;

TYPED_TEST_SUITE(IsEndpointConceptTest,
                 EndpointTestTypes,
                 ConceptTest::NameGenerator);

TYPED_TEST(IsEndpointConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(IsEndpoint<TestType>, ExpectedResult::value);
}

template <bool>
struct MaybeRequest {};

template <>
struct MaybeRequest<true> {
  using Request = ValidRequestBody;
};

template <bool>
struct MaybeResponse {};

template <>
struct MaybeResponse<true> {
  using Response = ValidResponseBody;
};

template <bool>
struct MaybeError {};

template <>
struct MaybeError<true> {
  using Error = ValidErrorBody;
};

template <bool>
struct MaybeURL {};

template <>
struct MaybeURL<true> {
  static GURL URL();
};

template <bool>
struct MaybeMethod {};

template <>
struct MaybeMethod<true> {
  static std::string_view Method();
};

template <bool HasRequest,
          bool HasResponse,
          bool HasError,
          bool HasURL,
          bool HasMethod>
struct EndpointCase : MaybeRequest<HasRequest>,
                      MaybeResponse<HasResponse>,
                      MaybeError<HasError>,
                      MaybeURL<HasURL>,
                      MaybeMethod<HasMethod> {
  static constexpr bool kHasRequest = HasRequest;
  static constexpr bool kHasResponse = HasResponse;
  static constexpr bool kHasError = HasError;
  static constexpr bool kHasURL = HasURL;
  static constexpr bool kHasMethod = HasMethod;
  static constexpr bool kSatisfiesConcept =
      kHasRequest && kHasResponse && kHasError && kHasURL && kHasMethod;
};

template <typename>
struct IsEndpointConceptMatrixTest : testing::Test {};

// Exhaustive presence/absence matrix.
using EndpointMatrix =
    testing::Types<EndpointCase<false, false, false, false, false>,
                   EndpointCase<false, false, false, false, true>,
                   EndpointCase<false, false, false, true, false>,
                   EndpointCase<false, false, false, true, true>,
                   EndpointCase<false, false, true, false, false>,
                   EndpointCase<false, false, true, false, true>,
                   EndpointCase<false, false, true, true, false>,
                   EndpointCase<false, false, true, true, true>,
                   EndpointCase<false, true, false, false, false>,
                   EndpointCase<false, true, false, false, true>,
                   EndpointCase<false, true, false, true, false>,
                   EndpointCase<false, true, false, true, true>,
                   EndpointCase<false, true, true, false, false>,
                   EndpointCase<false, true, true, false, true>,
                   EndpointCase<false, true, true, true, false>,
                   EndpointCase<false, true, true, true, true>,
                   EndpointCase<true, false, false, false, false>,
                   EndpointCase<true, false, false, false, true>,
                   EndpointCase<true, false, false, true, false>,
                   EndpointCase<true, false, false, true, true>,
                   EndpointCase<true, false, true, false, false>,
                   EndpointCase<true, false, true, false, true>,
                   EndpointCase<true, false, true, true, false>,
                   EndpointCase<true, false, true, true, true>,
                   EndpointCase<true, true, false, false, false>,
                   EndpointCase<true, true, false, false, true>,
                   EndpointCase<true, true, false, true, false>,
                   EndpointCase<true, true, false, true, true>,
                   EndpointCase<true, true, true, false, false>,
                   EndpointCase<true, true, true, false, true>,
                   EndpointCase<true, true, true, true, false>,
                   EndpointCase<true, true, true, true, true>>;

struct EndpointName {
  template <typename T>
  static std::string GetName(int) {
    auto name =
        [](auto... bools) {
          std::string bits;
          ((bits += (bools ? "1" : "0")), ...);
          return bits;
        }(T::kHasRequest, T::kHasResponse, T::kHasError, T::kHasURL,
          T::kHasMethod) +
        "_Endpoint";

    if (!T::kHasRequest) {
      name += "NoRequest";
    }

    if (!T::kHasResponse) {
      name += "NoResponse";
    }

    if (!T::kHasError) {
      name += "NoError";
    }

    if (!T::kHasURL) {
      name += "NoURL";
    }

    if (!T::kHasMethod) {
      name += "NoMethod";
    }

    return name + "_does" + (T::kSatisfiesConcept ? "" : "_not");
  }
};

TYPED_TEST_SUITE(IsEndpointConceptMatrixTest, EndpointMatrix, EndpointName);

TYPED_TEST(IsEndpointConceptMatrixTest, SatisfyConcept) {
  EXPECT_EQ(IsEndpoint<TypeParam>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client
