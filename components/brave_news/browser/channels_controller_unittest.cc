// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/channels_controller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/direct_feed_controller.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/pref_names.h"
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

constexpr char kPublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "One",
        "locales": [{
          "locale": "en_US",
          "channels": ["One", "Two", "Five"]
        }],
        "enabled": false
    },
    {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 2",
        "feed_url": "https://tp2.example.com/feed",
        "site_url": "https://tp2.example.com",
        "category": "Two",
        "locales": [{
          "locale": "en_US",
          "channels": ["Two", "Five"]
        }],
        "enabled": true
    },
    {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "feed_url": "https://tp3.example.com/feed",
        "site_url": "https://tp3.example.com",
        "category": "Four",
        "locales": [{
          "locale": "en_US",
          "channels": ["One", "Four"]
        }, {
          "locale": "ja_JA",
          "channels": ["One", "Five"]
        }],
        "enabled": true
    }
])";

class ChannelsControllerTest : public testing::Test {
 public:
  ChannelsControllerTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        direct_feed_controller_(profile_.GetPrefs(), nullptr),
        unsupported_publisher_migrator_(profile_.GetPrefs(),
                                        &direct_feed_controller_,
                                        &api_request_helper_),
        publishers_controller_(profile_.GetPrefs(),
                               &direct_feed_controller_,
                               &unsupported_publisher_migrator_,
                               &api_request_helper_,
                               nullptr),
        channels_controller_(profile_.GetPrefs(),
                             &publishers_controller_,
                             nullptr) {
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kBraveNewsOptedIn, true);
    profile_.GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                    true);
  }

  std::string GetPublishersURL() {
    return "https://" + brave_news::GetHostname() + "/sources." +
           brave_news::kRegionUrlPart + "json";
  }

  brave_news::Channels GetAllChannels() {
    base::RunLoop loop;
    brave_news::Channels channels;
    channels_controller_.GetAllChannels(
        base::BindLambdaForTesting([&loop, &channels](brave_news::Channels c) {
          channels = std::move(c);
          loop.Quit();
        }));
    loop.Run();

    return channels;
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

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  TestingProfile profile_;

  DirectFeedController direct_feed_controller_;
  UnsupportedPublisherMigrator unsupported_publisher_migrator_;
  PublishersController publishers_controller_;
  ChannelsController channels_controller_;
};

TEST_F(ChannelsControllerTest, CanGetAllChannels) {
  test_url_loader_factory_.AddResponse(GetPublishersURL(), kPublishersResponse,
                                       net::HTTP_OK);

  auto channels = GetAllChannels();
  EXPECT_EQ(4u, channels.size());
  EXPECT_TRUE(base::Contains(channels, "One"));
  EXPECT_TRUE(base::Contains(channels, "Two"));
  EXPECT_TRUE(base::Contains(channels, "Four"));
  EXPECT_TRUE(base::Contains(channels, "Five"));

  // By default, none of these channels should be subscribed.
  for (const auto& it : channels) {
    EXPECT_EQ(0u, it.second->subscribed_locales.size());
  }
}

TEST_F(ChannelsControllerTest, GetAllChannelsLoadsSubscribedState) {
  channels_controller_.SetChannelSubscribed("en_US", "One", true);
  channels_controller_.SetChannelSubscribed("en_US", "Five", true);

  test_url_loader_factory_.AddResponse(GetPublishersURL(), kPublishersResponse,
                                       net::HTTP_OK);

  auto channels = GetAllChannels();
  EXPECT_EQ(4u, channels.size());

  auto one = channels.find("One");
  ASSERT_NE(channels.end(), one);
  EXPECT_TRUE(base::Contains(one->second->subscribed_locales, "en_US"));

  auto two = channels.find("Two");
  ASSERT_NE(channels.end(), two);
  EXPECT_EQ(0u, two->second->subscribed_locales.size());

  auto four = channels.find("Four");
  ASSERT_NE(channels.end(), four);
  EXPECT_EQ(0u, four->second->subscribed_locales.size());

  auto five = channels.find("Five");
  ASSERT_NE(channels.end(), five);
  EXPECT_TRUE(base::Contains(five->second->subscribed_locales, "en_US"));
}

TEST_F(ChannelsControllerTest,
       GetAllChannelsLoadsCorrectLocaleSubscriptionStatus) {
  channels_controller_.SetChannelSubscribed("en_US", "One", true);
  channels_controller_.SetChannelSubscribed("ja_JA", "Five", true);

  test_url_loader_factory_.AddResponse(GetPublishersURL(), kPublishersResponse,
                                       net::HTTP_OK);

  auto channels = GetAllChannels();
  EXPECT_EQ(4u, channels.size());
  // In the en_US region, only the channel 'One' should be subscribed.
  for (const auto& it : channels) {
    EXPECT_EQ(it.first == "One",
              base::Contains(it.second->subscribed_locales, "en_US"));
  }

  // In the ja_JA region, only the channel 'Five' should be subscribed.
  for (const auto& it : channels) {
    EXPECT_EQ(it.first == "Five",
              base::Contains(it.second->subscribed_locales, "ja_JA"));
  }
}

TEST_F(ChannelsControllerTest, CanToggleChannelSubscribed) {
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("en_US", "Test"));

  channels_controller_.SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(channels_controller_.GetChannelSubscribed("en_US", "Test"));

  channels_controller_.SetChannelSubscribed("en_US", "Test", false);
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("en_US", "Test"));
}

TEST_F(ChannelsControllerTest,
       ChangingAChannelInOneLocaleDoesNotAffectOtherLocales) {
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("ja_JA", "Test"));

  channels_controller_.SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(channels_controller_.GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("ja_JA", "Test"));

  channels_controller_.SetChannelSubscribed("ja_JA", "Test", true);
  EXPECT_TRUE(channels_controller_.GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(channels_controller_.GetChannelSubscribed("ja_JA", "Test"));

  channels_controller_.SetChannelSubscribed("en_US", "Test", false);
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(channels_controller_.GetChannelSubscribed("ja_JA", "Test"));

  channels_controller_.SetChannelSubscribed("ja_JA", "Test", false);
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(channels_controller_.GetChannelSubscribed("ja_JA", "Test"));
}

TEST_F(ChannelsControllerTest, NoChannelsNoChannelLocales) {
  EXPECT_EQ(0u, channels_controller_.GetChannelLocales().size());
}

TEST_F(ChannelsControllerTest, SubscribedChannelLocalesIncluded) {
  channels_controller_.SetChannelSubscribed("en_US", "Test", true);

  auto locales = channels_controller_.GetChannelLocales();
  EXPECT_EQ(1u, locales.size());
  EXPECT_EQ("en_US", locales[0]);

  channels_controller_.SetChannelSubscribed("en_US", "Foo", true);
  locales = channels_controller_.GetChannelLocales();
  EXPECT_EQ(1u, locales.size());

  channels_controller_.SetChannelSubscribed("ja_JA", "Foo", true);
  locales = channels_controller_.GetChannelLocales();
  EXPECT_EQ(2u, locales.size());
  EXPECT_EQ("en_US", locales[0]);
  EXPECT_EQ("ja_JA", locales[1]);
}

TEST_F(ChannelsControllerTest, LocaleWithNoSubscribedChannelsIsNotIncluded) {
  channels_controller_.SetChannelSubscribed("en_US", "Test", true);

  auto locales = channels_controller_.GetChannelLocales();
  EXPECT_EQ(1u, locales.size());
  EXPECT_EQ("en_US", locales[0]);

  channels_controller_.SetChannelSubscribed("en_US", "Test", false);
  locales = channels_controller_.GetChannelLocales();
  EXPECT_EQ(0u, locales.size());
}

TEST_F(ChannelsControllerTest, ChannelMigrationsAreApplied) {
  channels_controller_.SetChannelSubscribed("en_US", "Tech News", true);
  channels_controller_.SetChannelSubscribed("en_US", "Sport", true);

  ChannelsController controller(profile_.GetPrefs(), &publishers_controller_,
                                nullptr);
  EXPECT_FALSE(controller.GetChannelSubscribed("en_US", "Tech News"));
  EXPECT_TRUE(controller.GetChannelSubscribed("en_US", "Technology"));
  EXPECT_FALSE(controller.GetChannelSubscribed("en_US", "Sport"));
  EXPECT_TRUE(controller.GetChannelSubscribed("en_US", "Sports"));
}

}  // namespace brave_news
