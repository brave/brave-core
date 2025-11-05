/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_endpoint.h"

#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/concept_test.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

namespace {

struct InvalidRequest {};

struct ValidRequestBody {
  base::Value::Dict ToValue() const;
};

using ValidRequest = POST<ValidRequestBody>;

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
  using Request = InvalidRequest;
  using Response = ValidResponseBody;
  using Error = ValidErrorBody;
  static GURL URL();
};

struct EndpointInvalidResponseBody {
  static constexpr char kName[] = "EndpointInvalidResponseBody";
  using Request = ValidRequest;
  using Response = InvalidResponseBody;
  using Error = ValidErrorBody;
  static GURL URL();
};

struct EndpointInvalidErrorBody {
  static constexpr char kName[] = "EndpointInvalidErrorBody";
  using Request = ValidRequest;
  using Response = ValidResponseBody;
  using Error = InvalidErrorBody;
  static GURL URL();
};

struct EndpointInvalidURL {
  static constexpr char kName[] = "EndpointInvalidURL";
  using Request = ValidRequest;
  using Response = ValidResponseBody;
  using Error = ValidErrorBody;
  static std::string URL();
};

template <typename T>
using IsEndpointConceptTest = ConceptTest::Fixture<T>;

// Hand-crafted invalid endpoints.
using EndpointTestTypes =
    testing::Types<std::tuple<EndpointInvalidRequestBody, std::false_type>,
                   std::tuple<EndpointInvalidResponseBody, std::false_type>,
                   std::tuple<EndpointInvalidErrorBody, std::false_type>,
                   std::tuple<EndpointInvalidURL, std::false_type>>;

}  // namespace

TYPED_TEST_SUITE(IsEndpointConceptTest,
                 EndpointTestTypes,
                 ConceptTest::NameGenerator);

TYPED_TEST(IsEndpointConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(IsEndpoint<TestType>, ExpectedResult::value);
}

namespace {

template <bool>
struct MaybeRequest {};

template <>
struct MaybeRequest<true> {
  using Request = ValidRequest;
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

template <bool HasRequest, bool HasResponse, bool HasError, bool HasURL>
struct EndpointCase : MaybeRequest<HasRequest>,
                      MaybeResponse<HasResponse>,
                      MaybeError<HasError>,
                      MaybeURL<HasURL> {
  static constexpr bool kHasRequest = HasRequest;
  static constexpr bool kHasResponse = HasResponse;
  static constexpr bool kHasError = HasError;
  static constexpr bool kHasURL = HasURL;
  static constexpr bool kSatisfiesConcept =
      kHasRequest && kHasResponse && kHasError && kHasURL;
};

template <typename>
struct IsEndpointConceptMatrixTest : testing::Test {};

// Exhaustive presence/absence matrix.
using EndpointMatrix = testing::Types<EndpointCase<false, false, false, false>,
                                      EndpointCase<false, false, false, true>,
                                      EndpointCase<false, false, true, false>,
                                      EndpointCase<false, false, true, true>,
                                      EndpointCase<false, true, false, false>,
                                      EndpointCase<false, true, false, true>,
                                      EndpointCase<false, true, true, false>,
                                      EndpointCase<false, true, true, true>,
                                      EndpointCase<true, false, false, false>,
                                      EndpointCase<true, false, false, true>,
                                      EndpointCase<true, false, true, false>,
                                      EndpointCase<true, false, true, true>,
                                      EndpointCase<true, true, false, false>,
                                      EndpointCase<true, true, false, true>,
                                      EndpointCase<true, true, true, false>,
                                      EndpointCase<true, true, true, true>>;

struct EndpointName {
  template <typename T>
  static std::string GetName(int) {
    auto name =
        [](auto... bools) {
          std::string bits;
          ((bits += (bools ? "1" : "0")), ...);
          return bits;
        }(T::kHasRequest, T::kHasResponse, T::kHasError, T::kHasURL) +
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

    return name + "_does" + (T::kSatisfiesConcept ? "" : "_not");
  }
};

}  // namespace

TYPED_TEST_SUITE(IsEndpointConceptMatrixTest, EndpointMatrix, EndpointName);

TYPED_TEST(IsEndpointConceptMatrixTest, SatisfyConcept) {
  EXPECT_EQ(IsEndpoint<TypeParam>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client
