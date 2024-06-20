/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/double_fetcher.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "brave/components/web_discovery/browser/wdp_service.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

namespace {
constexpr char kTestUrl[] = "https://example.com/test";
constexpr char kTestResponseText[] = "<html><body>test</body></html>";
}  // namespace

class WebDiscoveryDoubleFetcherTest : public testing::Test {
 public:
  WebDiscoveryDoubleFetcherTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~WebDiscoveryDoubleFetcherTest() override = default;

  // testing::Test:
  void SetUp() override {
    WDPService::RegisterProfilePrefs(profile_prefs_.registry());

    InitDoubleFetcher();
    SetUpResponse(net::HTTP_OK);
  }

 protected:
  void InitDoubleFetcher() {
    double_fetcher_ = std::make_unique<DoubleFetcher>(
        &profile_prefs_, shared_url_loader_factory_.get(),
        base::BindRepeating(&WebDiscoveryDoubleFetcherTest::HandleDoubleFetch,
                            base::Unretained(this)));
  }

  void SetUpResponse(net::HttpStatusCode status) {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(kTestUrl, kTestResponseText, status);
  }

  base::test::TaskEnvironment task_environment_;
  struct CompletedFetch {
    GURL url;
    base::Value associated_data;
    std::optional<std::string> response_body;
  };
  std::unique_ptr<DoubleFetcher> double_fetcher_;
  std::vector<CompletedFetch> completed_fetches_;
  network::TestURLLoaderFactory url_loader_factory_;

 private:
  void HandleDoubleFetch(const GURL& url,
                         const base::Value& associated_data,
                         std::optional<std::string> response_body) {
    completed_fetches_.push_back(
        CompletedFetch{.url = url,
                       .associated_data = associated_data.Clone(),
                       .response_body = response_body});
  }

  TestingPrefServiceSimple profile_prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(WebDiscoveryDoubleFetcherTest, ScheduleAndFetch) {
  GURL url(kTestUrl);
  double_fetcher_->ScheduleDoubleFetch(url, base::Value("foo1 data"));
  double_fetcher_->ScheduleDoubleFetch(url, base::Value("foo2 data"));

  task_environment_.FastForwardBy(base::Seconds(45));
  EXPECT_TRUE(completed_fetches_.empty());

  task_environment_.FastForwardBy(base::Seconds(30));
  ASSERT_EQ(completed_fetches_.size(), 1u);

  EXPECT_EQ(completed_fetches_[0].url, url);
  EXPECT_EQ(completed_fetches_[0].associated_data, base::Value("foo1 data"));
  ASSERT_TRUE(completed_fetches_[0].response_body);
  EXPECT_EQ(*completed_fetches_[0].response_body, kTestResponseText);

  completed_fetches_.clear();

  task_environment_.FastForwardBy(base::Seconds(25));
  EXPECT_TRUE(completed_fetches_.empty());

  task_environment_.FastForwardBy(base::Seconds(45));
  ASSERT_EQ(completed_fetches_.size(), 1u);

  EXPECT_EQ(completed_fetches_[0].url, url);
  EXPECT_EQ(completed_fetches_[0].associated_data, base::Value("foo2 data"));
  ASSERT_TRUE(completed_fetches_[0].response_body);
  EXPECT_EQ(*completed_fetches_[0].response_body, kTestResponseText);

  completed_fetches_.clear();

  task_environment_.FastForwardBy(base::Seconds(180));
  EXPECT_TRUE(completed_fetches_.empty());
}

TEST_F(WebDiscoveryDoubleFetcherTest, ScheduleRetry) {
  GURL url(kTestUrl);
  SetUpResponse(net::HTTP_INTERNAL_SERVER_ERROR);
  double_fetcher_->ScheduleDoubleFetch(url, base::Value(true));

  task_environment_.FastForwardBy(base::Seconds(75));
  EXPECT_TRUE(completed_fetches_.empty());

  SetUpResponse(net::HTTP_OK);
  task_environment_.FastForwardBy(base::Seconds(30));

  ASSERT_EQ(completed_fetches_.size(), 1u);

  EXPECT_EQ(completed_fetches_[0].url, url);
  EXPECT_EQ(completed_fetches_[0].associated_data, base::Value(true));
  ASSERT_TRUE(completed_fetches_[0].response_body);
  EXPECT_EQ(*completed_fetches_[0].response_body, kTestResponseText);

  completed_fetches_.clear();

  task_environment_.FastForwardBy(base::Seconds(180));
  EXPECT_TRUE(completed_fetches_.empty());
}

TEST_F(WebDiscoveryDoubleFetcherTest, ScheduleMaxRetries) {
  GURL url(kTestUrl);
  SetUpResponse(net::HTTP_INTERNAL_SERVER_ERROR);
  double_fetcher_->ScheduleDoubleFetch(url, base::Value(true));

  task_environment_.FastForwardBy(base::Seconds(70));
  EXPECT_TRUE(completed_fetches_.empty());

  task_environment_.FastForwardBy(base::Seconds(120));
  ASSERT_EQ(completed_fetches_.size(), 1u);

  EXPECT_EQ(completed_fetches_[0].url, url);
  EXPECT_EQ(completed_fetches_[0].associated_data, base::Value(true));
  ASSERT_FALSE(completed_fetches_[0].response_body);

  completed_fetches_.clear();

  SetUpResponse(net::HTTP_OK);
  task_environment_.FastForwardBy(base::Minutes(10));
  EXPECT_TRUE(completed_fetches_.empty());
}

TEST_F(WebDiscoveryDoubleFetcherTest, ScheduleNoRetry) {
  GURL url(kTestUrl);
  SetUpResponse(net::HTTP_NOT_FOUND);
  double_fetcher_->ScheduleDoubleFetch(url, base::Value(123));

  task_environment_.FastForwardBy(base::Seconds(70));
  ASSERT_EQ(completed_fetches_.size(), 1u);

  EXPECT_EQ(completed_fetches_[0].url, url);
  EXPECT_EQ(completed_fetches_[0].associated_data, base::Value(123));
  ASSERT_FALSE(completed_fetches_[0].response_body);

  completed_fetches_.clear();

  SetUpResponse(net::HTTP_OK);
  task_environment_.FastForwardBy(base::Minutes(10));
  EXPECT_TRUE(completed_fetches_.empty());
}

}  // namespace web_discovery
