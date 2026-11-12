/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_endpoint.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/components/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/endpoint_client/protobuf_test_endpoint_bodies.pb.h"
#include "brave/components/endpoint_client/request_types.h"
#include "brave/components/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace endpoint_client {

namespace {

struct InvalidRequest {};
struct InvalidResponse {};

using ValidBodylessRequest = Get;

using ValidJSONRequest = Post<JSONRequestBody>;
using ValidJSONResponse = Response<JSONSuccessBody, JSONErrorBody>;

using ValidProtobufRequest = Post<ProtobufRequestBody>;
using ValidProtobufResponse = Response<ProtobufSuccessBody, ProtobufErrorBody>;

enum class RequestTypeAliasState {
  kAbsent,
  kPresentBodyless,
  kPresentInvalidJSON,
  kPresentValidJSON,
  kPresentInvalidProtobuf,
  kPresentValidProtobuf
};

enum class ResponseTypeAliasState {
  kAbsent,
  kPresentInvalidJSON,
  kPresentValidJSON,
  kPresentInvalidProtobuf,
  kPresentValidProtobuf
};

enum class FunctionState { kAbsent, kPresentInvalid, kPresentValid };

template <RequestTypeAliasState>
struct MaybeRequest {};

template <>
struct MaybeRequest<RequestTypeAliasState::kPresentBodyless> {
  using Request = ValidBodylessRequest;
};

template <>
struct MaybeRequest<RequestTypeAliasState::kPresentInvalidJSON> {
  using Request = InvalidRequest;
};

template <>
struct MaybeRequest<RequestTypeAliasState::kPresentValidJSON> {
  using Request = ValidJSONRequest;
};

template <>
struct MaybeRequest<RequestTypeAliasState::kPresentInvalidProtobuf> {
  using Request = InvalidRequest;
};

template <>
struct MaybeRequest<RequestTypeAliasState::kPresentValidProtobuf> {
  using Request = ValidProtobufRequest;
};

template <ResponseTypeAliasState>
struct MaybeResponse {};

template <>
struct MaybeResponse<ResponseTypeAliasState::kPresentInvalidJSON> {
  using Response = InvalidResponse;
};

template <>
struct MaybeResponse<ResponseTypeAliasState::kPresentValidJSON> {
  using Response = ValidJSONResponse;
};

template <>
struct MaybeResponse<ResponseTypeAliasState::kPresentInvalidProtobuf> {
  using Response = InvalidResponse;
};

template <>
struct MaybeResponse<ResponseTypeAliasState::kPresentValidProtobuf> {
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

template <RequestTypeAliasState RequestState,
          ResponseTypeAliasState ResponseState,
          FunctionState URLState>
struct IsEndpointTestCase : MaybeRequest<RequestState>,
                            MaybeResponse<ResponseState>,
                            MaybeURL<URLState> {
  static constexpr RequestTypeAliasState kRequestState = RequestState;
  static constexpr ResponseTypeAliasState kResponseState = ResponseState;
  static constexpr FunctionState kURLState = URLState;
  static constexpr bool kSatisfiesConcept =
      (((RequestState == RequestTypeAliasState::kPresentBodyless ||
         RequestState == RequestTypeAliasState::kPresentValidJSON) &&
        ResponseState == ResponseTypeAliasState::kPresentValidJSON) ||
       ((RequestState == RequestTypeAliasState::kPresentBodyless ||
         RequestState == RequestTypeAliasState::kPresentValidProtobuf) &&
        ResponseState == ResponseTypeAliasState::kPresentValidProtobuf)) &&
      URLState == FunctionState::kPresentValid;
};

// clang-format off
// Test cases: 6 request states x 5 response states x 3 URL states = 90 tests
using IsEndpointTestCases = testing::Types<
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kAbsent, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentBodyless, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidJSON, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidJSON, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentInvalidProtobuf, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kAbsent, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kAbsent, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kAbsent, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentInvalidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentValidJSON, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentInvalidProtobuf, FunctionState::kPresentValid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kAbsent>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentInvalid>,
    IsEndpointTestCase<RequestTypeAliasState::kPresentValidProtobuf, ResponseTypeAliasState::kPresentValidProtobuf, FunctionState::kPresentValid>
>;
// clang-format on

struct IsEndpointTestCaseName {
  template <typename IsEndpointTestCase>
  static std::string GetName(int) {
    auto request_type_alias_state_to_string = [](RequestTypeAliasState state) {
      switch (state) {
        case RequestTypeAliasState::kAbsent:
          return "Absent";
        case RequestTypeAliasState::kPresentBodyless:
          return "Bodyless";
        case RequestTypeAliasState::kPresentInvalidJSON:
          return "InvalidJSON";
        case RequestTypeAliasState::kPresentValidJSON:
          return "ValidJSON";
        case RequestTypeAliasState::kPresentInvalidProtobuf:
          return "InvalidProtobuf";
        case RequestTypeAliasState::kPresentValidProtobuf:
          return "ValidProtobuf";
      }
    };

    auto response_type_alias_state_to_string =
        [](ResponseTypeAliasState state) {
          switch (state) {
            case ResponseTypeAliasState::kAbsent:
              return "Absent";
            case ResponseTypeAliasState::kPresentInvalidJSON:
              return "InvalidJSON";
            case ResponseTypeAliasState::kPresentValidJSON:
              return "ValidJSON";
            case ResponseTypeAliasState::kPresentInvalidProtobuf:
              return "InvalidProtobuf";
            case ResponseTypeAliasState::kPresentValidProtobuf:
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
         request_type_alias_state_to_string(IsEndpointTestCase::kRequestState),
         "_Response_",
         response_type_alias_state_to_string(
             IsEndpointTestCase::kResponseState),
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

}  // namespace endpoint_client
