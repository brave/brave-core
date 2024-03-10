// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/publishers_controller.h"

#include <tuple>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/browser/wait_for_callback.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/test/browser_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

constexpr char kPublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "en_US",
          "channels": ["One", "Tech"]
        }],
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
        "locales": [{
          "locale": "en_US",
          "channels": ["Sports", "Two"]
        }],
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
        "locales": [{
          "locale": "ja_JA",
          "channels": ["Design"]
        }],
        "enabled": true
    }
])";

class BraveNewsPublishersControllerTest : public testing::Test {
 public:
  BraveNewsPublishersControllerTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        pref_manager_(*profile_.GetPrefs()),
        publishers_controller_(&api_request_helper_) {
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, true);
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                    true);
  }

  std::string GetSourcesUrl() {
    return "https://" + brave_news::GetHostname() + "/sources." +
           brave_news::kRegionUrlPart + "json";
  }

  void SetSubscribedSources(const std::vector<std::string>& publisher_ids) {
    ScopedDictPrefUpdate update(profile_.GetPrefs(), prefs::kBraveNewsSources);
    for (const auto& id : publisher_ids) {
      update->Set(id, true);
    }
  }

  bool CombinedSourceExists(const std::string& publisher_id) {
    const auto& value = profile_.GetPrefs()->GetDict(prefs::kBraveNewsSources);
    return value.FindBool(publisher_id).has_value();
  }

  bool DirectSourceExists(const std::string& publisher_id) {
    return base::Contains(pref_manager_.GetSubscriptions().direct_feeds,
                          publisher_id,
                          [](const auto& feed) { return feed.id; });
  }

  Publishers GetPublishers() {
    auto [publishers] = WaitForCallback(base::BindOnce(
        [](BraveNewsPublishersControllerTest* test,
           GetPublishersCallback callback) {
          test->publishers_controller_.GetOrFetchPublishers(
              test->pref_manager_.GetSubscriptions(), std::move(callback),
              true);
        },
        base::Unretained(this)));
    return std::move(publishers);
  }

 protected:
  base::test::ScopedFeatureList scoped_features_;
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  TestingProfile profile_;
  BraveNewsPrefManager pref_manager_;
  PublishersController publishers_controller_;
};

TEST_F(BraveNewsPublishersControllerTest, CanReceiveFeeds) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
                                       net::HTTP_OK);
  LOG(ERROR) << "Sources URL: " << GetSourcesUrl();
  auto result = GetPublishers();
  ASSERT_EQ(3u, result.size());
  EXPECT_TRUE(base::Contains(result, "111"));
  EXPECT_TRUE(base::Contains(result, "333"));
  EXPECT_TRUE(base::Contains(result, "555"));
}

TEST_F(BraveNewsPublishersControllerTest, CanGetPublisherBySiteUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();
  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp5.example.com"));
  EXPECT_EQ("555", publisher->publisher_id);
}

TEST_F(BraveNewsPublishersControllerTest,
       CantGetNonExistingPublisherBySiteUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();

  EXPECT_EQ(nullptr, publishers_controller_.GetPublisherForSite(
                         GURL("https://not-a-site.com")));
}

TEST_F(BraveNewsPublishersControllerTest, CanGetPublisherByFeedUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();
  auto* publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp5.example.com/feed"));
  EXPECT_EQ("555", publisher->publisher_id);
}

TEST_F(BraveNewsPublishersControllerTest,
       PublisherInDefaultLocaleIsPreferred_PreferredFirst) {
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
        "locales": [{
          "locale": "en_US",
          "channels": ["One", "Tech"]
        }],
        "enabled": false
    },
    {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 1 JA",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com/en-JA",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "ja_JP",
          "channels": ["One"]
        }],
        "enabled": false
    }])",
                                       net::HTTP_OK);
  auto [locale] =
      WaitForCallback(base::BindOnce(&PublishersController::GetLocale,
                                     base::Unretained(&publishers_controller_),
                                     pref_manager_.GetSubscriptions()));

  EXPECT_EQ("en_US", locale);

  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("111", publisher->publisher_id);

  publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("111", publisher->publisher_id);
}

TEST_F(BraveNewsPublishersControllerTest,
       PublisherInDefaultLocaleIsPreferred_PreferredLast) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1 JA",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com/en-JA",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "ja_JP",
          "channels": ["One"]
        }],
        "enabled": false
    },
    {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "en_US",
          "channels": ["One", "Tech"]
        }],
        "enabled": false
    }])",
                                       net::HTTP_OK);
  GetPublishers();

  auto [locale] =
      WaitForCallback(base::BindOnce(&PublishersController::GetLocale,
                                     base::Unretained(&publishers_controller_),
                                     pref_manager_.GetSubscriptions()));

  EXPECT_EQ("en_US", locale);

  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp1.example.com/"));
  EXPECT_EQ("222", publisher->publisher_id);

  publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("222", publisher->publisher_id);
}

TEST_F(BraveNewsPublishersControllerTest, NoPreferredLocale_ReturnsFirstMatch) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1 JA",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com/en-JA",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "ja_JP",
          "channels": ["One"]
        }],
        "enabled": false
    },
    {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "Tech",
        "cover_url": "https://tp1.example.com/cover",
        "cover_url": "https://tp1.example.com/favicon",
        "background_color": "#FF0000",
        "locales": [{
          "locale": "pt_BR",
          "channels": ["One", "Tech"]
        }],
        "enabled": false
    }])",
                                       net::HTTP_OK);
  GetPublishers();

  auto [locale] =
      WaitForCallback(base::BindOnce(&PublishersController::GetLocale,
                                     base::Unretained(&publishers_controller_),
                                     pref_manager_.GetSubscriptions()));

  EXPECT_EQ("en_US", locale);

  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp1.example.com/"));
  EXPECT_EQ("111", publisher->publisher_id);

  publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("111", publisher->publisher_id);
}

TEST_F(BraveNewsPublishersControllerTest,
       CantGetNonExistingPublisherByFeedUrl) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
                                       net::HTTP_OK);
  GetPublishers();

  EXPECT_EQ(nullptr, publishers_controller_.GetPublisherForFeed(
                         GURL("https://tp5.example.com")));
}

TEST_F(BraveNewsPublishersControllerTest, CacheCanBeCleared) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
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

TEST_F(BraveNewsPublishersControllerTest, LocaleDefaultsToENUS) {
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
        "locales": [{ "locale": "not_LOCALE", "channels": [] }],
        "enabled": false
    }])",
                                       net::HTTP_OK);

  auto [locale] =
      WaitForCallback(base::BindOnce(&PublishersController::GetLocale,
                                     base::Unretained(&publishers_controller_),
                                     pref_manager_.GetSubscriptions()));

  EXPECT_EQ("en_US", locale);
}

TEST_F(BraveNewsPublishersControllerTest, CanGetPublishers) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kPublishersResponse,
                                       net::HTTP_OK);

  auto result = GetPublishers();
  EXPECT_EQ(3u, result.size());
}

}  // namespace brave_news
