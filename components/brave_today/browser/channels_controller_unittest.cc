// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/channels_controller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/browser/publishers_parsing.h"
#include "brave/components/brave_today/browser/unsupported_publisher_migrator.h"
#include "brave/components/brave_today/browser/urls.h"
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

constexpr char kPublishersResponse[] = R"([
    {
        "publisher_id": "111",
        "publisher_name": "Test Publisher 1",
        "feed_url": "https://tp1.example.com/feed",
        "site_url": "https://tp1.example.com",
        "category": "One",
        "channels": ["One", "Two", "Five"],
        "enabled": false
    },
    {
        "publisher_id": "222",
        "publisher_name": "Test Publisher 2",
        "feed_url": "https://tp2.example.com/feed",
        "site_url": "https://tp2.example.com",
        "category": "Two",
        "channels": ["Two", "Five"],
        "enabled": true
    },
    {
        "publisher_id": "333",
        "publisher_name": "Test Publisher 3",
        "feed_url": "https://tp3.example.com/feed",
        "site_url": "https://tp3.example.com",
        "category": "Four",
        "channels": ["Two", "Four"],
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
                               &api_request_helper_),
        channels_controller_(profile_.GetPrefs(), &publishers_controller_) {}

  std::string GetPublishersURL() {
    return "https://" + brave_today::GetHostname() + "/sources." +
           brave_today::GetRegionUrlPart() + "json";
  }

  brave_news::Channels GetAllChannels(const std::string& locale) {
    base::RunLoop loop;
    brave_news::Channels channels;
    channels_controller_.GetAllChannels(
        locale,
        base::BindLambdaForTesting([&loop, &channels](brave_news::Channels c) {
          channels = std::move(c);
          loop.Quit();
        }));
    loop.Run();

    return channels;
  }

  void SetSubscribedSources(const std::vector<std::string>& publisher_ids) {
    DictionaryPrefUpdate update(profile_.GetPrefs(), prefs::kBraveTodaySources);
    for (const auto& id : publisher_ids)
      update->SetBoolKey(id, true);
  }

  bool CombinedSourceExists(const std::string& publisher_id) {
    auto* value = profile_.GetPrefs()->GetDictionary(prefs::kBraveTodaySources);
    return value->FindBoolKey(publisher_id).has_value();
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

  auto channels = GetAllChannels("en_US");
  EXPECT_EQ(4u, channels.size());
  EXPECT_TRUE(base::Contains(channels, "One"));
  EXPECT_TRUE(base::Contains(channels, "Two"));
  EXPECT_TRUE(base::Contains(channels, "Four"));
  EXPECT_TRUE(base::Contains(channels, "Five"));

  // By default, none of these channels should be subscribed.
  for (const auto& it : channels) {
    EXPECT_FALSE(it.second->subscribed);
  }
}

TEST_F(ChannelsControllerTest, GetAllChannelsLoadsSubscribedState) {
  channels_controller_.SetChannelSubscribed("en_US", "One", true);
  channels_controller_.SetChannelSubscribed("en_US", "Five", true);

  test_url_loader_factory_.AddResponse(GetPublishersURL(), kPublishersResponse,
                                       net::HTTP_OK);

  auto channels = GetAllChannels("en_US");
  EXPECT_EQ(4u, channels.size());

  auto one = channels.find("One");
  ASSERT_NE(channels.end(), one);
  EXPECT_TRUE(one->second->subscribed);

  auto two = channels.find("Two");
  ASSERT_NE(channels.end(), two);
  EXPECT_FALSE(two->second->subscribed);

  auto four = channels.find("Four");
  ASSERT_NE(channels.end(), four);
  EXPECT_FALSE(four->second->subscribed);

  auto five = channels.find("Five");
  ASSERT_NE(channels.end(), five);
  EXPECT_TRUE(five->second->subscribed);
}

TEST_F(ChannelsControllerTest,
       GetAllChannelsLoadsCorrectLocaleSubscriptionStatus) {
  channels_controller_.SetChannelSubscribed("en_US", "One", true);
  channels_controller_.SetChannelSubscribed("ja_JA", "Five", true);

  test_url_loader_factory_.AddResponse(GetPublishersURL(), kPublishersResponse,
                                       net::HTTP_OK);

  auto channels = GetAllChannels("en_US");
  EXPECT_EQ(4u, channels.size());
  // In the en_US region, only the channel 'One' should be subscribed.
  for (const auto& it : channels)
    EXPECT_EQ(it.first == "One", it.second->subscribed);

  channels = GetAllChannels("ja_JA");
  EXPECT_EQ(4u, channels.size());
  // In the ja_JA region, only the channel 'Five' should be subscribed.
  for (const auto& it : channels)
    EXPECT_EQ(it.first == "Five", it.second->subscribed);
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

}  // namespace brave_news
