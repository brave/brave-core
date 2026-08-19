// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_controller.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_news/browser/direct_feed_fetcher.h"
#include "brave/components/brave_news/browser/test/wait_for_callback.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/features.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_policy/policy_initialization_waiter.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/test/history_service_test_util.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

namespace {

constexpr char kLocale[] = "en_NZ";

constexpr char kPublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "en_NZ",
          "channels": ["Top Sources", "Tech"]
        }],
        "enabled": false
    }])";

class TestDirectFeedFetcherDelegate : public DirectFeedFetcher::Delegate {
 public:
  ~TestDirectFeedFetcherDelegate() override = default;

  HTTPSUpgradeInfo GetURLHTTPSUpgradeInfo(const GURL& url) override {
    return HTTPSUpgradeInfo{.should_upgrade = false, .should_force = false};
  }

  base::WeakPtr<DirectFeedFetcher::Delegate> AsWeakPtr() override {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<TestDirectFeedFetcherDelegate> weak_ptr_factory_{this};
};

class TestControllerDelegate : public BraveNewsController::Delegate {
 public:
  ~TestControllerDelegate() override = default;
  void OpenSettings() override {}
  void CloseUI() override {}
};

}  // namespace

class BraveNewsControllerTest : public testing::Test {
 public:
  BraveNewsControllerTest() {
    // These tests exercise the FeedV2 code paths, which are gated behind
    // |kBraveNewsFeedUpdate|. That feature is disabled by default on Android,
    // so enable it explicitly to keep the tests platform independent.
    feature_list_.InitAndEnableFeature(features::kBraveNewsFeedUpdate);
  }
  ~BraveNewsControllerTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    prefs::RegisterProfilePrefs(pref_service_.registry());

    // Brave News starts out disabled, as it is for a user who hasn't opted in
    // yet.
    pref_service_.SetBoolean(prefs::kNewTabPageShowToday, true);
    pref_service_.SetBoolean(prefs::kBraveNewsOptedIn, false);

    history_service_ =
        history::CreateHistoryService(temp_dir_.GetPath(), /*create_db=*/true);

    // Only respond to the sources request - we aren't interested in the feed
    // contents here, just in which subscriptions the feed was built from.
    test_url_loader_factory_.SetInterceptor(
        base::BindLambdaForTesting([this](const network::ResourceRequest& req) {
          const bool is_sources = req.url.spec() == GetSourcesUrl();
          test_url_loader_factory_.AddResponse(
              req.url.spec(), is_sources ? kPublishersResponse : "",
              is_sources ? net::HTTP_OK : net::HTTP_NOT_FOUND);
        }));

    controller_ = std::make_unique<BraveNewsController>(
        &pref_service_,
        std::make_unique<brave_policy::PolicyInitializationWaiter>(
            /*policy_service=*/nullptr),
        history_service_.get(), test_url_loader_factory_.GetSafeWeakWrapper(),
        std::make_unique<TestDirectFeedFetcherDelegate>(),
        std::make_unique<TestControllerDelegate>());
  }

 protected:
  static std::string GetSourcesUrl() {
    return "https://" + GetHostname() + "/sources." + kRegionUrlPart + "json";
  }

  // Simulates the user opting in from the NTP. Note: this deliberately does
  // **not** wait for the resulting async work to finish, because the UI
  // requests the feed immediately after opting in.
  void OptIn() {
    controller_->SetConfiguration(
        mojom::Configuration::New(/*isOptedIn=*/true, /*showOnNTP=*/true,
                                  /*openArticlesInNewTab=*/true),
        base::DoNothing());
  }

  mojom::FeedV2Ptr GetFollowingFeed() {
    return std::get<0>(
        WaitForCallback(base::BindOnce(&BraveNewsController::GetFollowingFeed,
                                       base::Unretained(controller_.get()))));
  }

  mojom::FeedV2Ptr GetFeedV2() {
    return std::get<0>(WaitForCallback(base::BindOnce(
        &BraveNewsController::GetFeedV2, base::Unretained(controller_.get()))));
  }

  bool HasInitialSubscriptions() {
    return controller_->prefs().GetSubscriptions().channels().contains(kLocale);
  }

  // Declared before |task_environment_| so that it outlives the threads which
  // read the feature state.
  base::test::ScopedFeatureList feature_list_;

  // Declared before |task_environment_| so that it outlives it: destroying the
  // task environment flushes the thread pool, which is what lets the history
  // backend finish shutting down and release the files in here.
  base::ScopedTempDir temp_dir_;
  content::BrowserTaskEnvironment task_environment_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<history::HistoryService> history_service_;
  std::unique_ptr<BraveNewsController> controller_;

 private:
  const brave_l10n::test::ScopedDefaultLocale locale_{kLocale};
};

// Regression test for https://github.com/brave/brave-browser/issues/57691.
// Requesting the following feed immediately after opting in should wait for the
// initial "Top Sources" subscription to be created, instead of building a feed
// from an empty set of subscriptions and reporting to the UI that the user
// follows nothing.
TEST_F(BraveNewsControllerTest, GetFollowingFeedWaitsForInitialSubscriptions) {
  OptIn();
  ASSERT_FALSE(HasInitialSubscriptions());

  auto feed = GetFollowingFeed();

  EXPECT_TRUE(HasInitialSubscriptions());
  EXPECT_NE(mojom::FeedV2Error::NoFeeds, feed->error);
}

TEST_F(BraveNewsControllerTest, GetFeedV2WaitsForInitialSubscriptions) {
  OptIn();
  ASSERT_FALSE(HasInitialSubscriptions());

  auto feed = GetFeedV2();

  EXPECT_TRUE(HasInitialSubscriptions());
  EXPECT_NE(mojom::FeedV2Error::NoFeeds, feed->error);
}

TEST_F(BraveNewsControllerTest, GetFollowingFeedIsEmptyWhenDisabled) {
  auto feed = GetFollowingFeed();
  EXPECT_TRUE(feed->items.empty());
  EXPECT_FALSE(HasInitialSubscriptions());
}

TEST_F(BraveNewsControllerTest, GetFeedV2IsEmptyWhenDisabled) {
  auto feed = GetFeedV2();
  EXPECT_TRUE(feed->items.empty());
  EXPECT_FALSE(HasInitialSubscriptions());
}

}  // namespace brave_news
