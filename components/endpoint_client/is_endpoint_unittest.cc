/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/is_endpoint.h"

#include <optional>
#include <string>

#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/endpoint_client/request_types.h"
#include "brave/components/endpoint_client/response.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace endpoint_client {

namespace {

struct InvalidRequest {};

using ValidRequest = Get;

struct InvalidResponse {};

struct ValidResponseBody {
  static std::optional<ValidResponseBody> FromValue(const base::Value&);
};

using ValidResponse = Response<ValidResponseBody, ValidResponseBody>;

enum class State { kAbsent, kPresentInvalid, kPresentValid };

template <State>
struct MaybeRequest {};

template <>
struct MaybeRequest<State::kPresentInvalid> {
  using Request = InvalidRequest;
};

template <>
struct MaybeRequest<State::kPresentValid> {
  using Request = ValidRequest;
};

template <State>
struct MaybeResponse {};

template <>
struct MaybeResponse<State::kPresentInvalid> {
  using Response = InvalidResponse;
};

template <>
struct MaybeResponse<State::kPresentValid> {
  using Response = ValidResponse;
};

template <State>
struct MaybeURL {};

template <>
struct MaybeURL<State::kPresentInvalid> {
  static std::string URL();
};

template <>
struct MaybeURL<State::kPresentValid> {
  static GURL URL();
};

template <State RequestState, State ResponseState, State URLState>
struct IsEndpointTestCase : MaybeRequest<RequestState>,
                            MaybeResponse<ResponseState>,
                            MaybeURL<URLState> {
  static constexpr State kRequestState = RequestState;
  static constexpr State kResponseState = ResponseState;
  static constexpr State kURLState = URLState;
  static constexpr bool kSatisfiesConcept =
      RequestState == State::kPresentValid &&
      ResponseState == State::kPresentValid && URLState == State::kPresentValid;
};

// clang-format off
// Test cases: 3 request states x 3 response states x 3 URL states = 27 tests
using IsEndpointTestCases = testing::Types<
    IsEndpointTestCase<State::kAbsent, State::kAbsent, State::kAbsent>,
    IsEndpointTestCase<State::kAbsent, State::kAbsent, State::kPresentInvalid>,
    IsEndpointTestCase<State::kAbsent, State::kAbsent, State::kPresentValid>,
    IsEndpointTestCase<State::kAbsent, State::kPresentInvalid, State::kAbsent>,
    IsEndpointTestCase<State::kAbsent, State::kPresentInvalid, State::kPresentInvalid>,
    IsEndpointTestCase<State::kAbsent, State::kPresentInvalid, State::kPresentValid>,
    IsEndpointTestCase<State::kAbsent, State::kPresentValid, State::kAbsent>,
    IsEndpointTestCase<State::kAbsent, State::kPresentValid, State::kPresentInvalid>,
    IsEndpointTestCase<State::kAbsent, State::kPresentValid, State::kPresentValid>,
    IsEndpointTestCase<State::kPresentInvalid, State::kAbsent, State::kAbsent>,
    IsEndpointTestCase<State::kPresentInvalid, State::kAbsent, State::kPresentInvalid>,
    IsEndpointTestCase<State::kPresentInvalid, State::kAbsent, State::kPresentValid>,
    IsEndpointTestCase<State::kPresentInvalid, State::kPresentInvalid, State::kAbsent>,
    IsEndpointTestCase<State::kPresentInvalid, State::kPresentInvalid, State::kPresentInvalid>,
    IsEndpointTestCase<State::kPresentInvalid, State::kPresentInvalid, State::kPresentValid>,
    IsEndpointTestCase<State::kPresentInvalid, State::kPresentValid, State::kAbsent>,
    IsEndpointTestCase<State::kPresentInvalid, State::kPresentValid, State::kPresentInvalid>,
    IsEndpointTestCase<State::kPresentInvalid, State::kPresentValid, State::kPresentValid>,
    IsEndpointTestCase<State::kPresentValid, State::kAbsent, State::kAbsent>,
    IsEndpointTestCase<State::kPresentValid, State::kAbsent, State::kPresentInvalid>,
    IsEndpointTestCase<State::kPresentValid, State::kAbsent, State::kPresentValid>,
    IsEndpointTestCase<State::kPresentValid, State::kPresentInvalid, State::kAbsent>,
    IsEndpointTestCase<State::kPresentValid, State::kPresentInvalid, State::kPresentInvalid>,
    IsEndpointTestCase<State::kPresentValid, State::kPresentInvalid, State::kPresentValid>,
    IsEndpointTestCase<State::kPresentValid, State::kPresentValid, State::kAbsent>,
    IsEndpointTestCase<State::kPresentValid, State::kPresentValid, State::kPresentInvalid>,
    IsEndpointTestCase<State::kPresentValid, State::kPresentValid, State::kPresentValid>
>;
// clang-format on

struct IsEndpointTestCaseName {
  template <typename IsEndpointTestCase>
  static std::string GetName(int) {
    auto state_to_string = [](State state) {
      switch (state) {
        case State::kAbsent:
          return "Absent";
        case State::kPresentInvalid:
          return "Invalid";
        case State::kPresentValid:
          return "Valid";
      }
    };

    return base::StrCat(
        {"Request_", state_to_string(IsEndpointTestCase::kRequestState),
         "_Response_", state_to_string(IsEndpointTestCase::kResponseState),
         "_URL_", state_to_string(IsEndpointTestCase::kURLState), "_does",
         (IsEndpointTestCase::kSatisfiesConcept ? "" : "_not")});
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
