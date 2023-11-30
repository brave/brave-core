/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "base/functional/callback.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace api_request_helper {

namespace {

MATCHER_P(MatchesAPIRequestResult, request_result, "") {
  return arg == *request_result;
}

std::optional<std::string> ConversionCallback(
    const std::string& expected_raw_response,
    const std::optional<std::string>& converted_response,
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
    loader_wrapper_handler_ =
        std::make_unique<APIRequestHelper::URLLoaderHandler>(
            api_request_helper_.get());
  }
  ~ApiRequestHelperUnitTest() override = default;

  void SetInterceptor(const std::string& expected_method,
                      const GURL& expected_url,
                      const std::string& content_to_respond,
                      bool enable_cache) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, expected_method, expected_url, content_to_respond,
         enable_cache](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          EXPECT_EQ(request.url, expected_url);
          EXPECT_EQ(request.method, expected_method);
          if (enable_cache) {
            EXPECT_EQ(request.load_flags, net::LOAD_DO_NOT_SAVE_COOKIES);
          } else {
            EXPECT_EQ(request.load_flags, net::LOAD_DO_NOT_SAVE_COOKIES |
                                              net::LOAD_BYPASS_CACHE |
                                              net::LOAD_DISABLE_CACHE);
          }
          url_loader_factory_.AddResponse(request.url.spec(),
                                          content_to_respond);
        }));
  }

  void OnRequestResponse(bool* callback_called,
                         const std::string& expected_response,
                         const GURL expected_url,
                         const int expected_http_code,
                         const int expected_error_code,
                         APIRequestResult api_request_result) {
    *callback_called = true;
    EXPECT_EQ(expected_http_code, api_request_result.response_code());
    EXPECT_EQ(expected_response, api_request_result.body());
    EXPECT_EQ(expected_error_code, api_request_result.error_code());
    EXPECT_EQ(expected_url, api_request_result.final_url());
  }

  void SendRequest(const std::string& server_raw_response,
                   const std::string& expected_body,
                   const base::Value& expected_value_body,
                   const int expected_http_code = 200,
                   const int expected_error_code = net::OK,
                   APIRequestHelper::ResponseConversionCallback
                       conversion_callback = base::NullCallback(),
                   bool enable_cache = false) {
    GURL network_url("http://localhost/");

    APIRequestResult expected_result(
        expected_http_code, expected_body, expected_value_body.Clone(),
        {{"content-type", "text/html"}}, expected_error_code, network_url);
    base::MockCallback<APIRequestHelper::ResultCallback> callback;
    EXPECT_CALL(callback, Run(MatchesAPIRequestResult(&expected_result)));

    SetInterceptor("POST", network_url, server_raw_response, enable_cache);
    api_request_helper_->Request(
        "POST", network_url, "", "application/json", callback.Get(), {},
        APIRequestOptions(false, enable_cache, -1u, std::nullopt),
        std::move(conversion_callback));
    base::RunLoop().RunUntilIdle();
  }

  void SendMessageSSEJSON(std::string_view string_piece,
                          APIRequestHelper::DataReceivedCallback callback) {
    loader_wrapper_handler_->send_sse_data_for_testing(string_piece, true,
                                                       std::move(callback));
  }

  void SendMessageSSE(std::string_view string_piece,
                      APIRequestHelper::DataReceivedCallback callback) {
    loader_wrapper_handler_->send_sse_data_for_testing(string_piece, false,
                                                       std::move(callback));
  }

 protected:
  std::unique_ptr<APIRequestHelper> api_request_helper_;

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<APIRequestHelper::URLLoaderHandler> loader_wrapper_handler_;
};

TEST_F(ApiRequestHelperUnitTest, SanitizedRequest) {
  std::string expected_sanitized_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":1.8446744073709552e+19}";
  std::string server_raw_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":18446744073709551615}";
  SendRequest(server_raw_response, expected_sanitized_response,
              ParseJson(expected_sanitized_response));
  SendRequest("", "", base::Value());
  SendRequest("{}", "{}", base::Value(base::Value::Type::DICT));
  SendRequest("{", "", base::Value());
  SendRequest("0", "", base::Value());
  SendRequest("a", "", base::Value());
  // Android's sanitizer doesn't support trailing commas.
#if !BUILDFLAG(IS_ANDROID)
  SendRequest("{\"a\":1,}", "{\"a\":1}", ParseJson("{\"a\":1}"));
#endif
}

TEST_F(ApiRequestHelperUnitTest, RequestWithConversion) {
  std::string expected_sanitized_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"18446744073709551615\"}";
  std::string server_raw_response =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":18446744073709551615}";
  SendRequest(server_raw_response, expected_sanitized_response,
              ParseJson(expected_sanitized_response), 200, net::OK,
              base::BindOnce(&ConversionCallback, server_raw_response,
                             expected_sanitized_response));

  // Broken json after conversion
  // expecting empty response body after sanitization
  SendRequest(
      server_raw_response, "", base::Value(), 200, net::OK,
      base::BindOnce(&ConversionCallback, server_raw_response, "broken json"));
  // Empty json after conversion
  // expecting empty response body after sanitization
  SendRequest(server_raw_response, "", base::Value(), 200, net::OK,
              base::BindOnce(&ConversionCallback, server_raw_response, ""));

  // Returning std::nullopt in conversion callback results in empty response
  server_raw_response = "{}";
  SendRequest(
      server_raw_response, "", base::Value(), 422, net::OK,
      base::BindOnce(&ConversionCallback, server_raw_response, std::nullopt));
}

TEST_F(ApiRequestHelperUnitTest, Is2XXResponseCode) {
  EXPECT_TRUE(
      APIRequestResult(200, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_TRUE(
      APIRequestResult(201, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_TRUE(
      APIRequestResult(250, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_TRUE(
      APIRequestResult(299, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());

  EXPECT_FALSE(
      APIRequestResult(0, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(1, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(-1, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(199, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(300, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
  EXPECT_FALSE(
      APIRequestResult(500, {}, {}, {}, net::OK, GURL()).Is2XXResponseCode());
}

TEST_F(ApiRequestHelperUnitTest, EnableCache) {
  SendRequest("{}", "{}", base::Value(base::Value::Type::DICT), 200, net::OK,
              base::NullCallback(), false);
  SendRequest("{}", "{}", base::Value(base::Value::Type::DICT), 200, net::OK,
              base::NullCallback(), true);
}

TEST_F(ApiRequestHelperUnitTest, URLLoaderHandlerParsing) {
  base::RunLoop run_loop;
  SendMessageSSE("data: [DONE]",
                 base::BindRepeating(
                     [](base::RunLoop* run_loop,
                        data_decoder::DataDecoder::ValueOrError result) {
                       EXPECT_EQ("data: [DONE]", result->GetString());
                       run_loop->Quit();
                     },
                     &run_loop));
  run_loop.RunUntilIdle();
}

TEST_F(ApiRequestHelperUnitTest, SSEJsonParsing) {
  base::RunLoop run_loop;
  SendMessageSSEJSON(
      "data: {\"completion\": \" Hello there!\", \"stop\": null}",
      base::BindRepeating(
          [](base::RunLoop* run_loop,
             data_decoder::DataDecoder::ValueOrError result) {
            const std::string* completion =
                result->GetDict().FindString("completion");
            EXPECT_EQ(" Hello there!", *completion);
            run_loop->Quit();
          },
          &run_loop));
  run_loop.Run();

  base::RunLoop run_loop2;
  SendMessageSSEJSON(
      "data: {\"completion\": \" Hello there! How are you?\", \"stop\": null}",
      base::BindRepeating(
          [](base::RunLoop* run_loop,
             data_decoder::DataDecoder::ValueOrError result) {
            const std::string* completion =
                result->GetDict().FindString("completion");
            EXPECT_EQ(" Hello there! How are you?", *completion);
            run_loop->Quit();
          },
          &run_loop2));
  run_loop2.Run();

  // This test verifies that the callback is not called when the response is
  // "[DONE]". We use a run loop to wait for the callback to be called, and
  // we expect it to never be called.
  base::RunLoop run_loop3;
  SendMessageSSEJSON("data: [DONE]",
                     base::BindRepeating(
                         [](base::RunLoop* run_loop,
                            data_decoder::DataDecoder::ValueOrError result) {
                           run_loop->Quit();
                         },
                         &run_loop3));
  run_loop3.RunUntilIdle();

  // Testing with no JSON and an empty string
  base::RunLoop run_loop4;
  SendMessageSSEJSON("",
                     base::BindRepeating(
                         [](base::RunLoop* run_loop,
                            data_decoder::DataDecoder::ValueOrError result) {
                           run_loop->Quit();
                         },
                         &run_loop4));
  run_loop4.RunUntilIdle();
}

}  // namespace api_request_helper
