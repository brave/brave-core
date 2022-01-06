/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/core/url_fetcher.h"

#include <string>
#include <utility>

#include "base/callback_helpers.h"
#include "bat/ledger/internal/core/bat_ledger_test.h"

namespace ledger {

class URLFetcherTest : public BATLedgerTest {};

TEST_F(URLFetcherTest, FetchDelegatesToLedgerClient) {
  auto test_response = mojom::UrlResponse::New();
  test_response->status_code = 200;
  test_response->body = "hello_world";
  AddNetworkResultForTesting("test-url", mojom::UrlMethod::GET,
                             std::move(test_response));

  absl::optional<URLResponse> response;

  context()
      .Get<URLFetcher>()
      .Fetch(URLRequest::Get("test-url"))
      .Then(base::BindLambdaForTesting(
          [&response](URLResponse r) { response = std::move(r); }));

  task_environment()->RunUntilIdle();
  ASSERT_TRUE(response);
  EXPECT_EQ(response->status_code(), 200);
  EXPECT_EQ(response->ReadBodyAsText(), "hello_world");
}

TEST_F(URLFetcherTest, OnlyAllowedRequestHeadersAreLogged) {
  std::string log_output;
  SetLogCallbackForTesting(base::BindLambdaForTesting(
      [&log_output](const std::string& message) { log_output += message; }));

  auto request = URLRequest::Get("test-url");

  // Allowed headers:
  request.AddHeader("Digest", "digest_value");
  request.AddHeader("sIgnature", "signature_value");
  request.AddHeader("accepT", "accept_value");
  request.AddHeader("contenT-type", "content_type_value");

  // Non-allowed headers:
  request.AddHeader("secret", "secret_value");
  request.AddHeader("Cookie", "cookie_value");
  request.AddHeader("Authorization", "auth_value");

  context().Get<URLFetcher>().Fetch(std::move(request)).Then(base::DoNothing());

  task_environment()->RunUntilIdle();

  EXPECT_NE(log_output.find("digest_value"), std::string::npos);
  EXPECT_NE(log_output.find("signature_value"), std::string::npos);
  EXPECT_NE(log_output.find("accept_value"), std::string::npos);
  EXPECT_NE(log_output.find("content_type_value"), std::string::npos);

  EXPECT_EQ(log_output.find("secret_value"), std::string::npos);
  EXPECT_EQ(log_output.find("cookie_value"), std::string::npos);
  EXPECT_EQ(log_output.find("auth_value"), std::string::npos);
}

TEST_F(URLFetcherTest, LogResponseBody) {
  std::string log_output;
  SetLogCallbackForTesting(base::BindLambdaForTesting(
      [&log_output](const std::string& message) { log_output += message; }));

  auto test_response = mojom::UrlResponse::New();
  test_response->status_code = 200;
  test_response->body = "hello_world";
  AddNetworkResultForTesting("test-url", mojom::UrlMethod::GET,
                             std::move(test_response));

  context()
      .Get<URLFetcher>()
      .Fetch(URLRequest::Get("test-url"))
      .Then(base::DoNothing());

  task_environment()->RunUntilIdle();
  EXPECT_NE(log_output.find("hello_world"), std::string::npos);
}

TEST_F(URLFetcherTest, ErrorLogging) {
  std::string log_output;
  SetLogCallbackForTesting(base::BindLambdaForTesting(
      [&log_output](const std::string& message) { log_output += message; }));

  auto test_response = mojom::UrlResponse::New();
  test_response->status_code = 500;
  test_response->body = "error!!";
  AddNetworkResultForTesting("test-url", mojom::UrlMethod::GET,
                             std::move(test_response));

  auto request = mojom::UrlRequest::New();
  request->url = "test-url";

  context()
      .Get<URLFetcher>()
      .Fetch(URLRequest::Get("test-url"))
      .Then(base::DoNothing());

  task_environment()->RunUntilIdle();
  EXPECT_NE(log_output.find("error!!"), std::string::npos);
}

}  // namespace ledger
