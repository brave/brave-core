// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/publishers_controller.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/containers/contains.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/publishers_parsing.h"
#include "brave/components/brave_today/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

constexpr char kV1PublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "enabled": false
    },
    {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 2",
        "feed_url": "https://tp2.example.com/feed",
        "site_url": "https://tp2.example.com",
        "category": "Sports",
        "enabled": true
    },
    {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "feed_url": "https://tp3.example.com/feed",
        "site_url": "https://tp3.example.com",
        "category": "Design",
        "enabled": true
    }
])";

constexpr char kV2PublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "channels": ["One", "Tech"],
        "locales": ["en_US"],
        "enabled": false
    },
    {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "feed_url": "https://tp3.example.com/feed",
        "site_url": "https://tp3.example.com",
        "cover_url": "https://tp3.example.com/cover",
        "cover_url": "https://tp3.example.com/favicon",
        "background_color": "#FF00FF",
        "category": "Sports",
        "channels": ["Sports", "Two"],
        "locales": ["en_US"],
        "enabled": true
    },
    {
        "publisher_id": "555",
        "publisher_name": "Test Publisher 5",
        "feed_url": "https://tp5.example.com/feed",
        "site_url": "https://tp5.example.com",
        "cover_url": "https://tp5.example.com/cover",
        "cover_url": "https://tp5.example.com/favicon",
        "background_color": "#FFFF00",
        "category": "Design",
        "channels": ["Design"],
        "locales": ["ja_JA"],
        "enabled": true
    }
])";

class WaitForPublishersChanged : public PublishersController::Observer {
 public:
  explicit WaitForPublishersChanged(PublishersController* controller)
      : controller_(controller) {
    controller->AddObserver(this);
  }

  WaitForPublishersChanged(const WaitForPublishersChanged&) = delete;
  WaitForPublishersChanged& operator=(const WaitForPublishersChanged&) = delete;
  ~WaitForPublishersChanged() override { controller_->RemoveObserver(this); }

  void Wait() {
    if (updated_)
      return;
    loop_.Run();
  }

  void OnPublishersUpdated(PublishersController* controller) override {
    updated_ = true;
    loop_.Quit();
  }

 private:
  raw_ptr<PublishersController> controller_;
  bool updated_ = false;
  base::RunLoop loop_;
};

class PublishersControllerTest : public testing::Test {
 public:
  PublishersControllerTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        direct_feed_controller_(profile_.GetPrefs(), nullptr),
        unsupported_publishers_migrator_(profile_.GetPrefs(),
                                         &direct_feed_controller_,
                                         &api_request_helper_),
        publishers_controller_(profile_.GetPrefs(),
                               &direct_feed_controller_,
                               &unsupported_publishers_migrator_,
                               &api_request_helper_) {
    scoped_features_.InitAndEnableFeature(
        brave_today::features::kBraveNewsV2Feature);
  }

  std::string GetV1SourcesUrl() {
    return "https://" + brave_today::GetHostname() + "/sources." +
           brave_today::GetV1RegionUrlPart() + "json";
  }

  std::string GetSourcesUrl() {
    return "https://" + brave_today::GetHostname() + "/sources." +
           brave_today::GetRegionUrlPart() + "json";
  }

  void SetSubscribedSources(const std::vector<std::string>& publisher_ids) {
    DictionaryPrefUpdate update(profile_.GetPrefs(), prefs::kBraveTodaySources);
    for (const auto& id : publisher_ids)
      update->SetBoolKey(id, true);
  }

  bool CombinedSourceExists(const std::string& publisher_id) {
    const auto& value = profile_.GetPrefs()->GetDict(prefs::kBraveTodaySources);
    return value.FindBool(publisher_id).has_value();
  }

  bool DirectSourceExists(const std::string& publisher_id) {
    for (const auto& publisher :
         direct_feed_controller_.ParseDirectFeedsPref()) {
      if (publisher->publisher_id == publisher_id)
        return true;
    }
    return false;
  }

  Publishers GetPublishers() {
    base::RunLoop loop;
    Publishers publishers;
    publishers_controller_.GetOrFetchPublishers(
        base::BindLambdaForTesting([&publishers, &loop](Publishers result) {
          LOG(ERROR) << "Quitting!";
          publishers = std::move(result);
          loop.Quit();
        }));
    LOG(ERROR) << "Looping";
    loop.Run();
    return publishers;
  }

  void WaitForUpdate() {
    WaitForPublishersChanged waiter(&publishers_controller_);
    waiter.Wait();
  }

 protected:
  base::test::ScopedFeatureList scoped_features_;
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  TestingProfile profile_;
  DirectFeedController direct_feed_controller_;
  UnsupportedPublisherMigrator unsupported_publishers_migrator_;
  PublishersController publishers_controller_;
};

TEST_F(PublishersControllerTest, CanRecieveFeeds) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  LOG(ERROR) << "Sources URL: " << GetSourcesUrl();
  auto result = GetPublishers();
  ASSERT_EQ(3u, result.size());
  EXPECT_TRUE(base::Contains(result, "111"));
  EXPECT_TRUE(base::Contains(result, "333"));
  EXPECT_TRUE(base::Contains(result, "555"));
}

TEST_F(PublishersControllerTest,
       UnseenFeedsAreMigratedToDirectFeedsIfSubscribed) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  test_url_loader_factory_.AddResponse(GetV1SourcesUrl(), kV1PublishersResponse,
                                       net::HTTP_OK);
  SetSubscribedSources({"111", "222", "333"});

  GetPublishers();

  // Direct feed migration happens after the initial update. It should trigger
  // another when completed.
  WaitForUpdate();
  LOG(ERROR) << "Direct: "
             << profile_.GetPrefs()->GetDict(prefs::kBraveTodayDirectFeeds);
  EXPECT_TRUE(CombinedSourceExists("111"));
  EXPECT_TRUE(DirectSourceExists("222"));
  EXPECT_TRUE(CombinedSourceExists("333"));
}

TEST_F(PublishersControllerTest, CanGetPublisherBySiteUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();
  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp5.example.com"));
  EXPECT_EQ("555", publisher->publisher_id);
}

TEST_F(PublishersControllerTest, CantGetNonExistingPublisherBySiteUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();

  EXPECT_EQ(nullptr, publishers_controller_.GetPublisherForSite(
                         GURL("https://not-a-site.com")));
}

TEST_F(PublishersControllerTest, CanGetPublisherByFeedUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();
  auto* publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp5.example.com/feed"));
  EXPECT_EQ("555", publisher->publisher_id);
}

TEST_F(PublishersControllerTest, CantGetNonExistingPublisherByFeedUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();

  EXPECT_EQ(nullptr, publishers_controller_.GetPublisherForFeed(
                         GURL("https://tp5.example.com")));
}

TEST_F(PublishersControllerTest, CacheCanBeCleared) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();

  EXPECT_NE(nullptr, publishers_controller_.GetPublisherForFeed(
                         GURL("https://tp5.example.com/feed")));

  publishers_controller_.ClearCache();

  // When there's nothing in the cache, we shouldn't be able to look up a
  // publisher by feed.
  EXPECT_EQ(nullptr, publishers_controller_.GetPublisherForFeed(
                         GURL("https://tp5.example.com/feed")));
}

TEST_F(PublishersControllerTest, LocaleDefaultsToENUS) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "channels": ["One", "Tech"],
        "locales": ["not_LOCALE"],
        "enabled": false
    }])",
                                       net::HTTP_OK);
  std::string locale;
  base::RunLoop loop;
  publishers_controller_.GetLocale(
      base::BindLambdaForTesting([&locale, &loop](const std::string& result) {
        locale = result;
        loop.Quit();
      }));
  loop.Run();

  EXPECT_EQ("en_US", locale);
}

}  // namespace brave_news
