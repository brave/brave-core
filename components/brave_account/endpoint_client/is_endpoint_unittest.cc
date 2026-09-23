/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/is_endpoint.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoint_client {

namespace {

struct InvalidRequest {};
struct InvalidResponse {};

using ValidJSONRequest = POST<JSONRequestBody>;
using ValidJSONResponse = Response<JSONSuccessBody, JSONErrorBody>;

using ValidProtobufRequest = POST<ProtobufRequestBody>;
using ValidProtobufResponse = Response<ProtobufSuccessBody, ProtobufErrorBody>;

enum class TypeAliasState {
  kAbsent,
  kPresentInvalidJSON,
  kPresentValidJSON,
  kPresentInvalidProtobuf,
  kPresentValidProtobuf
};

enum class FunctionState { kAbsent, kPresentInvalid, kPresentValid };

template <TypeAliasState>
struct MaybeRequest {};

template <>
struct MaybeRequest<TypeAliasState::kPresentInvalidJSON> {
  using Request = InvalidRequest;
};

template <>
struct MaybeRequest<TypeAliasState::kPresentValidJSON> {
  using Request = ValidJSONRequest;
};

template <>
struct MaybeRequest<TypeAliasState::kPresentInvalidProtobuf> {
  using Request = InvalidRequest;
};

template <>
struct MaybeRequest<TypeAliasState::kPresentValidProtobuf> {
  using Request = ValidProtobufRequest;
};

template <TypeAliasState>
struct MaybeResponse {};

template <>
struct MaybeResponse<TypeAliasState::kPresentInvalidJSON> {
  using Response = InvalidResponse;
};

template <>
struct MaybeResponse<TypeAliasState::kPresentValidJSON> {
  using Response = ValidJSONResponse;
};

template <>
struct MaybeResponse<TypeAliasState::kPresentInvalidProtobuf> {
  using Response = InvalidResponse;
};

template <>
struct MaybeResponse<TypeAliasState::kPresentValidProtobuf> {
  using Response = ValidProtobufResponse;
};

template <FunctionState>
struct MaybeURL {};

template <>
struct MaybeURL<FunctionState::kPresentInvalid> {
  static std::string URL();
};

template <>
struct MaybeURL<FunctionState::kPresentValid> {
  static GURL URL();
};

template <TypeAliasState RequestState,
          TypeAliasState ResponseState,
          FunctionState URLState>
struct IsEndpointTestCase : MaybeRequest<RequestState>,
                            MaybeResponse<ResponseState>,
                            MaybeURL<URLState> {
  static constexpr TypeAliasState kRequestState = RequestState;
  static constexpr TypeAliasState kResponseState = ResponseState;
  static constexpr FunctionState kURLState = URLState;
  static constexpr bool kSatisfiesConcept =
      ((RequestState == TypeAliasState::kPresentValidJSON &&
        ResponseState == TypeAliasState::kPresentValidJSON) ||
       (RequestState == TypeAliasState::kPresentValidProtobuf &&
        ResponseState == TypeAliasState::kPresentValidProtobuf)) &&
      URLState == FunctionState::kPresentValid;
};

// clang-format off
// Test cases: 5 request states x 5 response states x 3 URL states = 75 tests
using IsEndpointTestCases = testing::Types<
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kAbsent, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidJSON, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidJSON, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentInvalidProtobuf, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<TypeAliasState::kPresentValidProtobuf, TypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>
>;
// clang-format on

struct IsEndpointTestCaseName {
  template <typename IsEndpointTestCase>
  static std::string GetName(int) {
    auto type_alias_state_to_string = [](TypeAliasState state) {
      switch (state) {
        case TypeAliasState::kAbsent:
          return "Absent";
        case TypeAliasState::kPresentInvalidJSON:
          return "InvalidJSON";
        case TypeAliasState::kPresentValidJSON:
          return "ValidJSON";
        case TypeAliasState::kPresentInvalidProtobuf:
          return "InvalidProtobuf";
        case TypeAliasState::kPresentValidProtobuf:
          return "ValidProtobuf";
      }
    };

    auto function_state_to_string = [](FunctionState state) {
      switch (state) {
        case FunctionState::kAbsent:
          return "Absent";
        case FunctionState::kPresentInvalid:
          return "Invalid";
        case FunctionState::kPresentValid:
          return "Valid";
      }
    };

    return base::StrCat(
        {"Request_",
         type_alias_state_to_string(IsEndpointTestCase::kRequestState),
         "_Response_",
         type_alias_state_to_string(IsEndpointTestCase::kResponseState),
         "_URL_", function_state_to_string(IsEndpointTestCase::kURLState),
         "_does", (IsEndpointTestCase::kSatisfiesConcept ? "" : "_not")});
  }
};

template <typename>
struct IsEndpointTest : testing::Test {};

}  // namespace

TYPED_TEST_SUITE(IsEndpointTest, IsEndpointTestCases, IsEndpointTestCaseName);

TYPED_TEST(IsEndpointTest, SatisfyConcept) {
  EXPECT_EQ(IsEndpoint<TypeParam>, TypeParam::kSatisfiesConcept);
}

}  // namespace brave_account::endpoint_client
