/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_url_loader_impl.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/rust/chromium_crates_io/vendor/cxx-1.0.117/include/cxx.h"

class SkusUrlLoaderImplUnitTest : public testing::Test {
 public:
  SkusUrlLoaderImplUnitTest() = default;
  ~SkusUrlLoaderImplUnitTest() override = default;

 protected:
  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &SkusUrlLoaderImplUnitTest::Interceptor, base::Unretained(this)));

    skus_url_loader_ = std::make_unique<skus::SkusUrlLoaderImpl>(
        url_loader_factory_.GetSafeWeakWrapper());
  }

  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(request.url.spec(), response_text_,
                                    status_);
  }
  skus::SkusUrlLoaderImpl* skus_url_loader() { return skus_url_loader_.get(); }
  void SetResponseText(const std::string& response) {
    response_text_ = response;
  }
  void SetResponseCode(net::HttpStatusCode status) { status_ = status; }

  base::Value GetRequestResponse(const std::string& method,
                                 const std::string& url) {
    bool callback_called = false;
    base::Value response;
    skus_url_loader()->Request(
        method, GURL(url), std::string(), std::string(),
        base::BindLambdaForTesting(
            [&](api_request_helper::APIRequestResult result) {
              callback_called = true;
              response = result.value_body().Clone();
            }),
        {}, {});
    task_environment_.RunUntilIdle();
    return response;
  }
  void FetchResponse(const std::string& method,
                     rust::cxxbridge1::Box<skus::HttpRoundtripContext> rt_ctx,
                     const std::string& url,
                     skus::SkusResult expected_result,
                     const std::string& expected_response) {
    bool callback_called = false;
    skus::HttpRequest req = {method, url, {}, {}};
    skus_url_loader()->SetFetchCompleteCallbackForTesting(
        base::BindLambdaForTesting([&](const skus::HttpResponse& result) {
          callback_called = true;
          std::string body(result.body.begin(), result.body.end());
          EXPECT_EQ(result.result, expected_result);
          EXPECT_EQ(body, expected_response);
        }));
    skus_url_loader()->BeginFetch(
        req, skus::SkusUrlLoaderImpl::FetchResponseCallback(),
        std::move(rt_ctx));
    task_environment_.RunUntilIdle();
    EXPECT_TRUE(callback_called);
  }

  base::test::TaskEnvironment task_environment_;

 private:
  std::string response_text_;
  net::HttpStatusCode status_ = net::HTTP_OK;
  std::unique_ptr<skus::SkusUrlLoaderImpl> skus_url_loader_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SkusUrlLoaderImplUnitTest, SanitizedResponse) {
  SetResponseText("{}");
  EXPECT_TRUE(GetRequestResponse("GET", "https://brave.com").is_dict());
  SetResponseText("{,}");
  EXPECT_TRUE(GetRequestResponse("GET", "https://brave.com").is_none());
}

TEST_F(SkusUrlLoaderImplUnitTest, BeginFetch) {
  SetResponseText("{}");
  FetchResponse(
      "GET",
      rust::cxxbridge1::Box<skus::HttpRoundtripContext>::from_raw(nullptr),
      "https://brave.com", skus::SkusResult::Ok, "{}");

  SetResponseText("");
  FetchResponse(
      "GET",
      rust::cxxbridge1::Box<skus::HttpRoundtripContext>::from_raw(nullptr),
      "https://brave.com", skus::SkusResult::Ok, "");

  SetResponseText("");
  SetResponseCode(net::HTTP_INTERNAL_SERVER_ERROR);
  FetchResponse(
      "GET",
      rust::cxxbridge1::Box<skus::HttpRoundtripContext>::from_raw(nullptr),
      "https://brave.com", skus::SkusResult::Ok, std::string());
}
