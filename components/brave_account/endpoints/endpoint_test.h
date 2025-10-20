/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINT_TEST_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINT_TEST_H_

#include <optional>
#include <string>
#include <tuple>

#include "base/check_deref.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoints/error.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoints {

inline bool operator==(const Error& lhs, const Error& rhs) {
  return lhs.code == rhs.code;
}

template <endpoint_client::IsEndpoint T>
struct EndpointTestCase {
  using Expected = brave_account::endpoint_client::Reply<T>;

  std::string test_name;
  net::HttpStatusCode http_status_code;
  std::string raw_reply;
  Expected reply;
};

template <endpoint_client::IsEndpoint T>
class EndpointTest : public testing::TestWithParam<const EndpointTestCase<T>*> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return CHECK_DEREF(info.param).test_name;
  };

 protected:
  void RunTestCase() {
    const auto& test_case = CHECK_DEREF(this->GetParam());

    test_url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& resource_request) {
          test_url_loader_factory_.AddResponse(resource_request.url.spec(),
                                               test_case.raw_reply,
                                               test_case.http_status_code);
        }));

    base::test::TestFuture<int, typename EndpointTestCase<T>::Expected> future;
    endpoint_client::Client<T>::Send(
        test_url_loader_factory_.GetSafeWeakWrapper(), typename T::Request(),
        future.GetCallback());
    EXPECT_EQ(future.Get(),
              std::tie(test_case.http_status_code, test_case.reply))
        << GetErrorMessage(std::get<1>(future.Take()));
  }

  template <typename Reply>
  std::string GetErrorMessage(const Reply& r) {
    if (r.has_value()) {
      return {};
    }
    if (const auto* network =
            std::get_if<brave_account::endpoint_client::NetworkError>(
                &r.error())) {
      return network->error_message;
    } else if (const auto* parse =
                   std::get_if<brave_account::endpoint_client::ParseError>(
                       &r.error())) {
      return parse->error_message;
    }
    return "Invalid endpoint error";
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
};

}  // namespace brave_account::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINTS_ENDPOINT_TEST_H_
