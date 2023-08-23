/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"

#include <utility>

#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wayback_machine/url_constants.h"
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

    if (!expected_fetch_url_.is_empty()) {
      EXPECT_EQ(expected_fetch_url_, request.url);
      std::move(callback_).Run();
    }
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

  void TestFetchURL(const GURL& url, const GURL& expected_fetch_url) {
    base::RunLoop loop;
    callback_ = loop.QuitClosure();
    expected_fetch_url_ = expected_fetch_url;
    wayback_url_loader_->Fetch(url);
    loop.Run();
  }

  base::OnceClosure callback_;
  GURL expected_fetch_url_;
  std::string response_text_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<WaybackClient> client_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<WaybackMachineURLFetcher> wayback_url_loader_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(WaybackMachineURLFetcherUnitTest, SanitizedResponse) {
  SetResponseText("");
  Fetch(GURL::EmptyGURL());
  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"https://web.archive.org/favicon.ico"}}})");
  Fetch(GURL("https://web.archive.org/favicon.ico"));
  // broken json
  SetResponseText(
      R"(,{"archived_snapshots":{"closest":{"url":"https://web.archive.com/favicon.ico"}}})");
  Fetch(GURL::EmptyGURL());
}

TEST_F(WaybackMachineURLFetcherUnitTest, InputURLSanitizeTest) {
  constexpr char kInputURL[] = "http://myid:mypwd@test.com/";
  constexpr char kSanitizedURL[] = "http://test.com/";
  EXPECT_EQ(GURL(kSanitizedURL),
            wayback_url_loader_->GetSanitizedInputURL(GURL(kInputURL)));

  // Test sanitized url is passed to url loader.
  TestFetchURL(GURL(kInputURL),
               GURL(base::StrCat({kWaybackQueryURL, kSanitizedURL})));
}

TEST_F(WaybackMachineURLFetcherUnitTest, WaybackURLSanitizeTest) {
  // Blocked non http/https sheme urls.
  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"javascript:abcd"}}})");
  Fetch(GURL::EmptyGURL());

  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"chrome://abcd"}}})");
  Fetch(GURL::EmptyGURL());

  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"brave://abcd"}}})");
  Fetch(GURL::EmptyGURL());

  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"file://abcd"}}})");
  Fetch(GURL::EmptyGURL());

  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"http://another_archive.org/favicon.ico"}}})");
  Fetch(GURL::EmptyGURL());

  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"http://web.archive.org/favicon.ico"}}})");
  // Check above http url is upgraded to https.
  Fetch(GURL("https://web.archive.org/favicon.ico"));

  SetResponseText(
      R"({"archived_snapshots":{"closest":{"url":"https://web.archive.org/favicon.ico"}}})");
  Fetch(GURL("https://web.archive.org/favicon.ico"));
}
