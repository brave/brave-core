/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/concepts.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {
struct TestNames {
  template <typename T>
  static std::string GetName(int) {
    using TestType = std::tuple_element_t<0, T>;
    using ExpectedResult = std::tuple_element_t<1, T>;
    return std::string(TestType::kName) + "_does" +
           (ExpectedResult::value ? "" : "_not");
  }
};
}  // namespace

namespace brave_account::endpoint_client::concepts {
// -------------------------------
//      Request concept tests
// -------------------------------

struct RequestNoToValue {
  static constexpr char kName[] = "RequestNoToValue";
};

struct RequestStaticToValue {
  static constexpr char kName[] = "RequestStaticToValue";
  static base::Value::Dict ToValue();
};

struct RequestToValueWithWrongReturnType {
  static constexpr char kName[] = "RequestToValueWithWrongReturnType";
  void ToValue() const;
};

struct RequestToValueWithWrongParameterType {
  static constexpr char kName[] = "RequestToValueWithWrongParameterType";
  base::Value::Dict ToValue(int) const;
};

struct ValidRequest {
  static constexpr char kName[] = "ValidRequest";
  base::Value::Dict ToValue() const;
};

template <typename T>
struct RequestConceptTest : testing::Test {
  using TestType = std::tuple_element_t<0, T>;
  using ExpectedResult = std::tuple_element_t<1, T>;
};

using RequestTestTypes = testing::Types<
    std::tuple<RequestNoToValue, std::false_type>,
    std::tuple<RequestStaticToValue, std::false_type>,
    std::tuple<RequestToValueWithWrongReturnType, std::false_type>,
    std::tuple<RequestToValueWithWrongParameterType, std::false_type>,
    std::tuple<ValidRequest, std::true_type>>;

TYPED_TEST_SUITE(RequestConceptTest, RequestTestTypes, TestNames);

TYPED_TEST(RequestConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(Request<TestType>, ExpectedResult::value);
}

// --------------------------------
//      Response concept tests
// --------------------------------

struct ResponseNoFromValue {
  static constexpr char kName[] = "ResponseNoFromValue";
};

struct ResponseNonStaticFromValue {
  static constexpr char kName[] = "ResponseNonStaticFromValue";
  std::optional<ResponseNonStaticFromValue> FromValue(const base::Value&);
};

struct ResponseFromValueWithWrongReturnType {
  static constexpr char kName[] = "ResponseFromValueWithWrongReturnType";
  static ResponseFromValueWithWrongReturnType FromValue(const base::Value&);
};

struct ResponseFromValueWithWrongParameterType {
  static constexpr char kName[] = "ResponseFromValueWithWrongParameterType";
  static std::optional<ResponseFromValueWithWrongParameterType> FromValue(int);
};

struct ValidResponse {
  static constexpr char kName[] = "ValidResponse";
  static std::optional<ValidResponse> FromValue(const base::Value&);
};

template <typename T>
struct ResponseConceptTest : testing::Test {
  using TestType = std::tuple_element_t<0, T>;
  using ExpectedResult = std::tuple_element_t<1, T>;
};

using ResponseTestTypes = testing::Types<
    std::tuple<ResponseNoFromValue, std::false_type>,
    std::tuple<ResponseNonStaticFromValue, std::false_type>,
    std::tuple<ResponseFromValueWithWrongReturnType, std::false_type>,
    std::tuple<ResponseFromValueWithWrongParameterType, std::false_type>,
    std::tuple<ValidResponse, std::true_type>>;

TYPED_TEST_SUITE(ResponseConceptTest, ResponseTestTypes, TestNames);

TYPED_TEST(ResponseConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(Response<TestType>, ExpectedResult::value);
}

// -----------------------------
//      Error concept tests
// -----------------------------

struct ErrorNoFromValue {
  static constexpr char kName[] = "ErrorNoFromValue";
};

struct ErrorNonStaticFromValue {
  static constexpr char kName[] = "ErrorNonStaticFromValue";
  std::optional<ErrorNonStaticFromValue> FromValue(const base::Value&);
};

struct ErrorFromValueWithWrongReturnType {
  static constexpr char kName[] = "ErrorFromValueWithWrongReturnType";
  static ErrorFromValueWithWrongReturnType FromValue(const base::Value&);
};

struct ErrorFromValueWithWrongParameterType {
  static constexpr char kName[] = "ErrorFromValueWithWrongParameterType";
  static std::optional<ErrorFromValueWithWrongParameterType> FromValue(int);
};

struct ValidError {
  static constexpr char kName[] = "ValidError";
  static std::optional<ValidError> FromValue(const base::Value&);
};

template <typename T>
struct ErrorConceptTest : testing::Test {
  using TestType = std::tuple_element_t<0, T>;
  using ExpectedResult = std::tuple_element_t<1, T>;
};

using ErrorTestTypes = testing::Types<
    std::tuple<ErrorNoFromValue, std::false_type>,
    std::tuple<ErrorNonStaticFromValue, std::false_type>,
    std::tuple<ErrorFromValueWithWrongReturnType, std::false_type>,
    std::tuple<ErrorFromValueWithWrongParameterType, std::false_type>,
    std::tuple<ValidError, std::true_type>>;

TYPED_TEST_SUITE(ErrorConceptTest, ErrorTestTypes, TestNames);

TYPED_TEST(ErrorConceptTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(Error<TestType>, ExpectedResult::value);
}

// --------------------------------
//      Endpoint concept tests
// --------------------------------

struct EndpointInvalidRequest {
  static constexpr char kName[] = "EndpointInvalidRequest";
  using Request = RequestNoToValue;
  using Response = ValidResponse;
  using Error = ValidError;
  static GURL URL();
  static std::string_view Method();
};

struct EndpointInvalidResponse {
  static constexpr char kName[] = "EndpointInvalidResponse";
  using Request = ValidRequest;
  using Response = ResponseNoFromValue;
  using Error = ValidError;
  static GURL URL();
  static std::string_view Method();
};

struct EndpointInvalidError {
  static constexpr char kName[] = "EndpointInvalidError";
  using Request = ValidRequest;
  using Response = ValidResponse;
  using Error = ErrorNoFromValue;
  static GURL URL();
  static std::string_view Method();
};

struct EndpointInvalidURL {
  static constexpr char kName[] = "EndpointInvalidURL";
  using Request = ValidRequest;
  using Response = ValidResponse;
  using Error = ValidError;
  static std::string URL();
  static std::string_view Method();
};

struct EndpointInvalidMethod {
  static constexpr char kName[] = "EndpointInvalidMethod";
  using Request = ValidRequest;
  using Response = ValidResponse;
  using Error = ValidError;
  static GURL URL();
  static bool Method();
};

template <typename T>
struct EndpointConceptInvalidTest : testing::Test {
  using TestType = std::tuple_element_t<0, T>;
  using ExpectedResult = std::tuple_element_t<1, T>;
};

// Hand-crafted invalid endpoints.
using EndpointInvalidTypes =
    testing::Types<std::tuple<EndpointInvalidRequest, std::false_type>,
                   std::tuple<EndpointInvalidResponse, std::false_type>,
                   std::tuple<EndpointInvalidError, std::false_type>,
                   std::tuple<EndpointInvalidURL, std::false_type>,
                   std::tuple<EndpointInvalidMethod, std::false_type>>;

TYPED_TEST_SUITE(EndpointConceptInvalidTest, EndpointInvalidTypes, TestNames);

TYPED_TEST(EndpointConceptInvalidTest, SatisfyConcept) {
  using TestType = typename TestFixture::TestType;
  using ExpectedResult = typename TestFixture::ExpectedResult;
  EXPECT_EQ(Endpoint<TestType>, ExpectedResult::value);
}

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
  using Response = ValidResponse;
};

template <bool>
struct MaybeError {};

template <>
struct MaybeError<true> {
  using Error = ValidError;
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
struct EndpointConceptMatrixTest : testing::Test {};

// Exhaustive presence/absence matrix.
using EndpointMatrixTypes = testing::Types<EndpointCase<0, 0, 0, 0, 0>,
                                           EndpointCase<0, 0, 0, 0, 1>,
                                           EndpointCase<0, 0, 0, 1, 0>,
                                           EndpointCase<0, 0, 0, 1, 1>,
                                           EndpointCase<0, 0, 1, 0, 0>,
                                           EndpointCase<0, 0, 1, 0, 1>,
                                           EndpointCase<0, 0, 1, 1, 0>,
                                           EndpointCase<0, 0, 1, 1, 1>,
                                           EndpointCase<0, 1, 0, 0, 0>,
                                           EndpointCase<0, 1, 0, 0, 1>,
                                           EndpointCase<0, 1, 0, 1, 0>,
                                           EndpointCase<0, 1, 0, 1, 1>,
                                           EndpointCase<0, 1, 1, 0, 0>,
                                           EndpointCase<0, 1, 1, 0, 1>,
                                           EndpointCase<0, 1, 1, 1, 0>,
                                           EndpointCase<0, 1, 1, 1, 1>,
                                           EndpointCase<1, 0, 0, 0, 0>,
                                           EndpointCase<1, 0, 0, 0, 1>,
                                           EndpointCase<1, 0, 0, 1, 0>,
                                           EndpointCase<1, 0, 0, 1, 1>,
                                           EndpointCase<1, 0, 1, 0, 0>,
                                           EndpointCase<1, 0, 1, 0, 1>,
                                           EndpointCase<1, 0, 1, 1, 0>,
                                           EndpointCase<1, 0, 1, 1, 1>,
                                           EndpointCase<1, 1, 0, 0, 0>,
                                           EndpointCase<1, 1, 0, 0, 1>,
                                           EndpointCase<1, 1, 0, 1, 0>,
                                           EndpointCase<1, 1, 0, 1, 1>,
                                           EndpointCase<1, 1, 1, 0, 0>,
                                           EndpointCase<1, 1, 1, 0, 1>,
                                           EndpointCase<1, 1, 1, 1, 0>,
                                           EndpointCase<1, 1, 1, 1, 1>>;

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

TYPED_TEST_SUITE(EndpointConceptMatrixTest, EndpointMatrixTypes, EndpointName);

TYPED_TEST(EndpointConceptMatrixTest, SatisfyConcept) {
  EXPECT_EQ(Endpoint<TypeParam>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client::concepts
