/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"

#include <utility>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

class WaybackClient : public WaybackMachineURLFetcher::Client {
 public:
  WaybackClient() {}

  void SetCallback(base::OnceClosure callback) {
    callback_ = std::move(callback);
  }
  void SetExpectedURL(GURL expected_url) { expected_url_ = expected_url; }
  void OnWaybackURLFetched(const GURL& lastest_wayback_url) override {
    EXPECT_EQ(lastest_wayback_url, expected_url_);
    if (callback_)
      std::move(callback_).Run();
  }

 private:
  GURL expected_url_;
  base::OnceClosure callback_;
};

}  // namespace

class WaybackMachineURLFetcherUnitTest : public testing::Test {
 public:
  WaybackMachineURLFetcherUnitTest() = default;
  ~WaybackMachineURLFetcherUnitTest() override = default;

 protected:
  void SetUp() override {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    url_loader_factory_.SetInterceptor(
        base::BindRepeating(&WaybackMachineURLFetcherUnitTest::Interceptor,
                            base::Unretained(this)));
    client_ = std::make_unique<WaybackClient>();
    wayback_url_loader_ = std::make_unique<WaybackMachineURLFetcher>(
        client_.get(), url_loader_factory_.GetSafeWeakWrapper());
  }

  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(request.url.spec(), response_text_);
  }
  void SetResponseText(const std::string& response) {
    response_text_ = response;
  }
  void Fetch(const GURL& expected_url) {
    base::RunLoop loop;
    client_->SetCallback(loop.QuitClosure());
    client_->SetExpectedURL(expected_url);
    wayback_url_loader_->Fetch(expected_url);
    loop.Run();
  }

 private:
  std::string response_text_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<WaybackMachineURLFetcher> wayback_url_loader_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<WaybackClient> client_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(WaybackMachineURLFetcherUnitTest, SanitizedResponse) {
  SetResponseText("");
  Fetch(GURL());
  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}})");
  Fetch(GURL("https://example.com/favicon.ico"));
  // broken json
  SetResponseText(
      R"(,{"archived_snapshots":{"closest":{"url":"https://example.com/favicon.ico"}}})");
  Fetch(GURL());
}
