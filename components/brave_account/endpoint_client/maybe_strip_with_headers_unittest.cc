/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"

#include <concepts>
#include <tuple>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/request_types.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoint_client {

namespace {

using detail::MaybeStripWithHeaders;

struct TestRequestBody {
  base::Value::Dict ToValue() const;
};

using TestRequest = POST<TestRequestBody>;

template <typename T>
struct MaybeStripWithHeadersTest : testing::Test {
  using TestType = std::tuple_element_t<0, T>;
  using ExpectedType = std::tuple_element_t<1, T>;
};

using MaybeStripWithHeadersTestTypes =
    testing::Types<std::tuple<void*, void*>,
                   std::tuple<volatile int, volatile int>,
                   std::tuple<TestRequest, TestRequest>,
                   std::tuple<WithHeaders<TestRequest>, TestRequest>>;

}  // namespace

TYPED_TEST_SUITE(MaybeStripWithHeadersTest, MaybeStripWithHeadersTestTypes);

TYPED_TEST(MaybeStripWithHeadersTest, StripsWithHeaders) {
  using TestType = typename TestFixture::TestType;
  using ExpectedType = typename TestFixture::ExpectedType;
  EXPECT_TRUE((std::same_as<MaybeStripWithHeaders<TestType>, ExpectedType>));
}

}  // namespace brave_account::endpoint_client
