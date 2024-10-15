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
#include "base/functional/bind.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_news/browser/brave_news_controller.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/browser/test/wait_for_callback.h"
#include "brave/components/brave_news/browser/urls.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
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

class BraveNewsChannelsControllerTest : public testing::Test {
 public:
  BraveNewsChannelsControllerTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()) {
    prefs::RegisterProfilePrefs(pref_service_.registry());

    pref_manager_ = std::make_unique<BraveNewsPrefManager>(pref_service_);

    // Ensure Brave News is enabled.
    pref_manager_->SetConfig(mojom::Configuration::New(true, true, true));

    publishers_controller_ =
        std::make_unique<PublishersController>(&api_request_helper_);
    channels_controller_ =
        std::make_unique<ChannelsController>(publishers_controller_.get());
  }

  std::string GetPublishersURL() {
    return "https://" + brave_news::GetHostname() + "/sources." +
           brave_news::kRegionUrlPart + "json";
  }

  brave_news::Channels GetAllChannels() {
    auto [channels] = WaitForCallback(
        base::BindOnce(&ChannelsController::GetAllChannels,
                       base::Unretained(channels_controller_.get()),
                       pref_manager_->GetSubscriptions()));
    return std::move(channels);
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;

  std::unique_ptr<BraveNewsPrefManager> pref_manager_;
  std::unique_ptr<PublishersController> publishers_controller_;
  std::unique_ptr<ChannelsController> channels_controller_;
};

TEST_F(BraveNewsChannelsControllerTest, CanGetAllChannels) {
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

TEST_F(BraveNewsChannelsControllerTest, GetAllChannelsLoadsSubscribedState) {
  pref_manager_->SetChannelSubscribed("en_US", "One", true);
  pref_manager_->SetChannelSubscribed("en_US", "Five", true);

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

TEST_F(BraveNewsChannelsControllerTest,
       GetAllChannelsLoadsCorrectLocaleSubscriptionStatus) {
  pref_manager_->SetChannelSubscribed("en_US", "One", true);
  pref_manager_->SetChannelSubscribed("ja_JA", "Five", true);

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

TEST_F(BraveNewsChannelsControllerTest, CanGetPublisherChannels) {
  auto publisher = mojom::Publisher::New();
  auto l1 = mojom::LocaleInfo::New();
  l1->locale = "en_NZ";
  l1->channels.push_back("foo");
  l1->channels.push_back("bar");
  publisher->locales.push_back(std::move(l1));

  auto l2 = mojom::LocaleInfo::New();
  l2->locale = "en_AU";
  l2->channels.push_back("foo");
  publisher->locales.push_back(std::move(l2));

  EXPECT_EQ(0u, GetChannelsForPublisher("en_US", publisher).size());

  auto channels = GetChannelsForPublisher("en_NZ", publisher);
  EXPECT_EQ(2u, channels.size());
  EXPECT_EQ("foo", channels.at(0));
  EXPECT_EQ("bar", channels.at(1));

  channels = GetChannelsForPublisher("en_AU", publisher);
  EXPECT_EQ(1u, channels.size());
  EXPECT_EQ("foo", channels.at(0));
}

}  // namespace brave_news
