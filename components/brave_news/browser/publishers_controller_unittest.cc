// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/publishers_controller.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
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
    if (updated_) {
      return;
    }
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
                               &api_request_helper_,
                               nullptr) {
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, true);
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                    true);
  }

  std::string GetV1SourcesUrl() {
    std::string locale =
        brave_l10n::GetDefaultISOLanguageCodeString() == "ja" ? "ja" : "";
    return "https://" + brave_news::GetHostname() + "/sources." + locale +
           "json";
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
    for (const auto& publisher :
         direct_feed_controller_.ParseDirectFeedsPref()) {
      if (publisher->publisher_id == publisher_id) {
        return true;
      }
    }
    return false;
  }

  Publishers GetPublishers() {
    base::RunLoop loop;
    Publishers publishers;
    publishers_controller_.GetOrFetchPublishers(
        base::BindLambdaForTesting([&publishers, &loop](Publishers result) {
          publishers = std::move(result);
          loop.Quit();
        }));
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
             << profile_.GetPrefs()->GetDict(prefs::kBraveNewsDirectFeeds);
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

TEST_F(PublishersControllerTest,
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
  GetPublishers();

  std::string locale;
  base::RunLoop loop;
  publishers_controller_.GetLocale(
      base::BindLambdaForTesting([&locale, &loop](const std::string& result) {
        locale = result;
        loop.Quit();
      }));
  loop.Run();

  EXPECT_EQ("en_US", locale);

  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("111", publisher->publisher_id);

  publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("111", publisher->publisher_id);
}

TEST_F(PublishersControllerTest,
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

  std::string locale;
  base::RunLoop loop;
  publishers_controller_.GetLocale(
      base::BindLambdaForTesting([&locale, &loop](const std::string& result) {
        locale = result;
        loop.Quit();
      }));
  loop.Run();

  EXPECT_EQ("en_US", locale);

  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp1.example.com/"));
  EXPECT_EQ("222", publisher->publisher_id);

  publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("222", publisher->publisher_id);
}

TEST_F(PublishersControllerTest, NoPreferredLocale_ReturnsFirstMatch) {
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

  std::string locale;
  base::RunLoop loop;
  publishers_controller_.GetLocale(
      base::BindLambdaForTesting([&locale, &loop](const std::string& result) {
        locale = result;
        loop.Quit();
      }));
  loop.Run();

  EXPECT_EQ("en_US", locale);

  auto* publisher = publishers_controller_.GetPublisherForSite(
      GURL("https://tp1.example.com/"));
  EXPECT_EQ("111", publisher->publisher_id);

  publisher = publishers_controller_.GetPublisherForFeed(
      GURL("https://tp1.example.com/feed"));
  EXPECT_EQ("111", publisher->publisher_id);
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
        "locales": [{ "locale": "not_LOCALE", "channels": [] }],
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

TEST_F(PublishersControllerTest, CanGetPublishers) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);

  auto result = GetPublishers();
  EXPECT_EQ(3u, result.size());
}

TEST_F(PublishersControllerTest, DoesntFetchPublishersWhenNotOptedIn) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);

  profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, false);
  auto result = GetPublishers();
  EXPECT_EQ(0u, result.size());
}

TEST_F(PublishersControllerTest, DoesntFetchPublishersWhenNotShowing) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);

  profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                  false);
  auto result = GetPublishers();
  EXPECT_EQ(0u, result.size());
}

TEST_F(PublishersControllerTest,
       DoesntFetchPublishersWhenNotShowingAndNotOptedIn) {
  test_url_loader_factory_.AddResponse(GetSourcesUrl(), kV2PublishersResponse,
                                       net::HTTP_OK);

  profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                  false);
  profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, false);
  auto result = GetPublishers();
  EXPECT_EQ(0u, result.size());
}

}  // namespace brave_news
