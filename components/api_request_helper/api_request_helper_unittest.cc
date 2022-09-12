/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace api_request_helper {

namespace {
absl::optional<std::string> ConversionCallback(
    const std::string& expected_raw_response,
    const absl::optional<std::string>& converted_response,
    const std::string& raw_response) {
  EXPECT_EQ(expected_raw_response, raw_response);
  return converted_response;
}
}  // namespace

class ApiRequestHelperUnitTest : public testing::Test {
 public:
  ApiRequestHelperUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    api_request_helper_ = std::make_unique<APIRequestHelper>(
        net::NetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS),
        shared_url_loader_factory_);
  }
  ~ApiRequestHelperUnitTest() override = default;

  void SetInterceptor(const std::string& expected_method,
                      const GURL& expected_url,
                      const std::string& content_to_respond) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_method, expected_url,
         content_to_respond](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          EXPECT_EQ(request.url, expected_url);
          EXPECT_EQ(request.method, expected_method);
          url_loader_factory_.AddResponse(request.url.spec(),
                                          content_to_respond);
        }));
  }

  void OnRequestResponse(bool* callback_called,
                         const std::string& expected_response,
                         const int expected_http_code,
                         APIRequestResult api_request_result) {
    *callback_called = true;
    EXPECT_EQ(expected_http_code, api_request_result.response_code());
    EXPECT_EQ(expected_response, api_request_result.body());
  }

  void SendRequest(const std::string& server_raw_response,
                   const std::string& expected_sanitized_response,
                   const int expected_http_code = 200,
                   APIRequestHelper::ResponseConversionCallback
                       conversion_callback = base::NullCallback()) {
    bool callback_called = false;
    GURL network_url("http://localhost/");
    SetInterceptor("POST", network_url, server_raw_response);
    api_request_helper_->Request(
        "POST", network_url, "", "application/json", false,
        base::BindOnce(&ApiRequestHelperUnitTest::OnRequestResponse,
                       base::Unretained(this), &callback_called,
                       expected_sanitized_response, expected_http_code),
        {}, -1u, std::move(conversion_callback));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

 protected:
  std::unique_ptr<APIRequestHelper> api_request_helper_;

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(ApiRequestHelperUnitTest, SanitizedRequest) {
  std::string expected_sanitized_response =
#if BUILDFLAG(IS_ANDROID)
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":1.8446744073709552E19}";
#else
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":1.8446744073709552e+19}";
#endif
  std::string server_raw_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":18446744073709551615}";
  SendRequest(server_raw_response, expected_sanitized_response);
  SendRequest("", "");
  SendRequest("{}", "{}");
  SendRequest("{", "");
  SendRequest("0", "");
  SendRequest("a", "");
  // Android's sanitizer doesn't support trailing commas.
#if !BUILDFLAG(IS_ANDROID)
  SendRequest("{\"a\":1,}", "{\"a\":1}");
#endif
}

TEST_F(ApiRequestHelperUnitTest, RequestWithConversion) {
  std::string expected_sanitized_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"18446744073709551615\"}";
  std::string server_raw_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":18446744073709551615}";
  SendRequest(server_raw_response, expected_sanitized_response, 200,
              base::BindOnce(&ConversionCallback, server_raw_response,
                             expected_sanitized_response));

  // Broken json after conversion
  // expecting empty response body after sanitization
  SendRequest(
      server_raw_response, "", 200,
      base::BindOnce(&ConversionCallback, server_raw_response, "broken json"));
  // Empty json after conversion
  // expecting empty response body after sanitization
  SendRequest(server_raw_response, "", 200,
              base::BindOnce(&ConversionCallback, server_raw_response, ""));

  // Returning absl::nullopt in conversion callback doesn't override the
  // response
  server_raw_response = "{}";
  SendRequest(
      server_raw_response, server_raw_response, 422,
      base::BindOnce(&ConversionCallback, server_raw_response, absl::nullopt));
}

TEST_F(ApiRequestHelperUnitTest, Is2XXResponseCode) {
  EXPECT_TRUE(APIRequestResult(200, {}, {}).Is2XXResponseCode());
  EXPECT_TRUE(APIRequestResult(201, {}, {}).Is2XXResponseCode());
  EXPECT_TRUE(APIRequestResult(250, {}, {}).Is2XXResponseCode());
  EXPECT_TRUE(APIRequestResult(299, {}, {}).Is2XXResponseCode());

  EXPECT_FALSE(APIRequestResult(0, {}, {}).Is2XXResponseCode());
  EXPECT_FALSE(APIRequestResult(1, {}, {}).Is2XXResponseCode());
  EXPECT_FALSE(APIRequestResult(-1, {}, {}).Is2XXResponseCode());
  EXPECT_FALSE(APIRequestResult(199, {}, {}).Is2XXResponseCode());
  EXPECT_FALSE(APIRequestResult(300, {}, {}).Is2XXResponseCode());
  EXPECT_FALSE(APIRequestResult(500, {}, {}).Is2XXResponseCode());
}

}  // namespace api_request_helper
