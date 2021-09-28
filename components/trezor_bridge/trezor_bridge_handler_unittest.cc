/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/trezor_bridge/trezor_bridge_handler.h"

#include "base/json/json_reader.h"
#include "base/test/bind.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

class TrezorBridgeHandlerUnitTest : public testing::Test {
 public:
  TrezorBridgeHandlerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        browser_context_(new content::TestBrowserContext()),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();

          if (request.method == "GET") {
            url_loader_factory_.AddResponse(request.url.spec(),
                                            "response for get");
          } else if (request.method == "POST") {
            base::StringPiece request_string(
                request.request_body->elements()
                    ->at(0)
                    .As<network::DataElementBytes>()
                    .AsStringPiece());
            EXPECT_EQ(request_string, "test");
            url_loader_factory_.AddResponse(request.url.spec(),
                                            "response for post");
          }
        }));
    handler_.reset(new TrezorBridgeHandler(shared_url_loader_factory_.get()));
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<content::TestBrowserContext> browser_context_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<TrezorBridgeHandler> handler_;
};

TEST_F(TrezorBridgeHandlerUnitTest, HandleLocalResourcePageRequests) {
  base::ListValue list;
  list.Append(base::Value("1"));
  list.Append(base::Value("./data/config.json"));
  bool callback_called = false;
  handler_->SetRequestCallbackForTesting(
      base::BindLambdaForTesting([&](const base::Value& value) {
        const auto ok = value.FindBoolKey("ok");
        ASSERT_TRUE(ok);
        ASSERT_TRUE(*ok);

        const std::string* text = value.FindStringKey("text");
        ASSERT_TRUE(text);
        ASSERT_FALSE(text->empty());

        const std::string* statusText = value.FindStringKey("statusText");
        ASSERT_TRUE(statusText);
        EXPECT_EQ(*statusText, "ok");

        callback_called = true;
      }));
  handler_->HandleFetchRequest(&list);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
}

TEST_F(TrezorBridgeHandlerUnitTest, HandleSuitePOSTRequests) {
  base::ListValue list;
  list.Append(base::Value("1"));
  list.Append(base::Value("http://127.0.0.1:21325/path"));
  absl::optional<base::Value> payload = base::JSONReader::Read(R"({
    "method": "POST",
    "body": "test"
  })");
  list.Append(payload.value().Clone());

  bool callback_called = false;
  handler_->SetRequestCallbackForTesting(
      base::BindLambdaForTesting([&](const base::Value& value) {
        const auto ok = value.FindBoolKey("ok");
        ASSERT_TRUE(ok);
        ASSERT_TRUE(*ok);

        const std::string* text = value.FindStringKey("text");
        ASSERT_TRUE(text);
        EXPECT_EQ(*text, "response for post");

        const std::string* statusText = value.FindStringKey("statusText");
        ASSERT_TRUE(statusText);
        EXPECT_EQ(*statusText, "ok");

        callback_called = true;
      }));
  handler_->HandleFetchRequest(&list);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
}

TEST_F(TrezorBridgeHandlerUnitTest, HandleSuiteGETRequests) {
  base::ListValue list;
  list.Append(base::Value("1"));
  list.Append(base::Value("http://127.0.0.1:21325/path"));
  absl::optional<base::Value> payload = base::JSONReader::Read(R"({
    "method": "GET"
  })");
  list.Append(payload.value().Clone());

  bool callback_called = false;
  handler_->SetRequestCallbackForTesting(
      base::BindLambdaForTesting([&](const base::Value& value) {
        const auto ok = value.FindBoolKey("ok");
        ASSERT_TRUE(ok);
        ASSERT_TRUE(*ok);

        const std::string* text = value.FindStringKey("text");
        ASSERT_TRUE(text);
        EXPECT_EQ(*text, "response for get");

        const std::string* statusText = value.FindStringKey("statusText");
        ASSERT_TRUE(statusText);
        EXPECT_EQ(*statusText, "ok");

        callback_called = true;
      }));
  handler_->HandleFetchRequest(&list);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
}

TEST_F(TrezorBridgeHandlerUnitTest, HandleUnknownGETRequests) {
  base::ListValue list;
  list.Append(base::Value("1"));
  list.Append(base::Value("http://1.1.1.1:21325/path"));
  absl::optional<base::Value> payload = base::JSONReader::Read(R"({
    "method": "GET"
  })");
  list.Append(payload.value().Clone());

  bool callback_called = false;
  handler_->SetRequestCallbackForTesting(
      base::BindLambdaForTesting([&](const base::Value& value) {
        const auto ok = value.FindBoolKey("ok");
        ASSERT_TRUE(ok);
        ASSERT_FALSE(*ok);

        const std::string* text = value.FindStringKey("text");
        ASSERT_TRUE(text);
        ASSERT_TRUE(text->empty());

        const std::string* statusText = value.FindStringKey("statusText");
        ASSERT_TRUE(statusText);
        EXPECT_EQ(*statusText, "Unknown request rejected");

        callback_called = true;
      }));
  handler_->HandleFetchRequest(&list);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
}

TEST_F(TrezorBridgeHandlerUnitTest, HandleUnknownPOSTRequests) {
  base::ListValue list;
  list.Append(base::Value("1"));
  list.Append(base::Value("http://1.1.1.1:21325/path"));
  absl::optional<base::Value> payload = base::JSONReader::Read(R"({
    "method": "POST",
    "body": "data"
  })");
  list.Append(payload.value().Clone());

  bool callback_called = false;
  handler_->SetRequestCallbackForTesting(
      base::BindLambdaForTesting([&](const base::Value& value) {
        const auto ok = value.FindBoolKey("ok");
        ASSERT_TRUE(ok);
        ASSERT_FALSE(*ok);

        const std::string* text = value.FindStringKey("text");
        ASSERT_TRUE(text);
        ASSERT_TRUE(text->empty());

        const std::string* statusText = value.FindStringKey("statusText");
        ASSERT_TRUE(statusText);
        EXPECT_EQ(*statusText, "Unknown request rejected");

        callback_called = true;
      }));
  handler_->HandleFetchRequest(&list);
  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(callback_called);
}
