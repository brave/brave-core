/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_url_loader_impl.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

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
    url_loader_factory_.AddResponse(request.url.spec(), response_text_);
  }
  skus::SkusUrlLoaderImpl* skus_url_loader() { return skus_url_loader_.get(); }
  void SetResponseText(const std::string& response) {
    response_text_ = response;
  }
  std::string GetRequestResponse(const std::string& method,
                                 const std::string& url) {
    bool callback_called = false;
    std::string response;
    skus_url_loader()->Request(
        method, GURL(url), std::string(), std::string(), false,
        base::BindLambdaForTesting(
            [&](api_request_helper::APIRequestResult result) {
              callback_called = true;
              response = result.body();
            }),
        {}, -1u);
    base::RunLoop().RunUntilIdle();
    return response;
  }

 private:
  std::string response_text_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<skus::SkusUrlLoaderImpl> skus_url_loader_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SkusUrlLoaderImplUnitTest, SanitizedResponse) {
  SetResponseText("{}");
  EXPECT_EQ(GetRequestResponse("GET", "https://brave.com"), "{}");
  SetResponseText("{,}");
  EXPECT_EQ(GetRequestResponse("GET", "https://brave.com"), "");
}
