// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/unsupported_publisher_migrator.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/direct_feed_controller.h"
#include "brave/components/brave_today/browser/publishers_parsing.h"
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

class UnsupportedPublisherMigratorTest : public testing::Test {
 public:
  UnsupportedPublisherMigratorTest()
      : api_request_helper_(TRAFFIC_ANNOTATION_FOR_TESTS,
                            test_url_loader_factory_.GetSafeWeakWrapper()),
        direct_feed_controller_(profile_.GetPrefs(), nullptr),
        migrator_(profile_.GetPrefs(),
                  &direct_feed_controller_,
                  &api_request_helper_) {}

  std::string OldSourcesURL() {
    return "https://" + brave_today::GetHostname() + "/sources." +
           brave_today::GetV1RegionUrlPart() + "json";
  }

  uint64_t MaybeMigrateFeeds(const std::vector<std::string>& publisher_ids) {
    base::RunLoop loop;
    uint64_t migrated_count;
    migrator_.MigrateUnsupportedFeeds(
        publisher_ids,
        base::BindLambdaForTesting([&loop, &migrated_count](uint64_t c) {
          migrated_count = c;
          loop.Quit();
        }));
    loop.Run();

    return migrated_count;
  }

  std::vector<mojom::PublisherPtr> GetDirectFeeds() {
    return direct_feed_controller_.ParseDirectFeedsPref();
  }

  void SetSubscribedSources(const std::vector<std::string>& publisher_ids) {
    DictionaryPrefUpdate update(profile_.GetPrefs(), prefs::kBraveTodaySources);
    for (const auto& id : publisher_ids)
      update->SetBoolKey(id, true);
  }

  void SetDirectSources(const std::vector<std::string>& publisher_ids) {
    DictionaryPrefUpdate update(profile_.GetPrefs(),
                                prefs::kBraveTodayDirectFeeds);
    for (const auto& id : publisher_ids) {
      base::Value direct_source(base::Value::Type::DICTIONARY);
      direct_source.SetStringKey(prefs::kBraveTodayDirectFeedsKeySource,
                                 "https://" + id + ".example.com/feed");
      direct_source.SetStringKey(prefs::kBraveTodayDirectFeedsKeyTitle,
                                 "Test Publisher " + id);
      update->SetPath(id, std::move(direct_source));
    }
  }

  bool CombinedSourceExists(const std::string& publisher_id) {
    const auto& value = profile_.GetPrefs()->GetDict(prefs::kBraveTodaySources);
    return value.FindBool(publisher_id).has_value();
  }

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;
  data_decoder::test::InProcessDataDecoder data_decoder_;
  network::TestURLLoaderFactory test_url_loader_factory_;
  api_request_helper::APIRequestHelper api_request_helper_;
  TestingProfile profile_;
  DirectFeedController direct_feed_controller_;
  UnsupportedPublisherMigrator migrator_;
};

TEST_F(UnsupportedPublisherMigratorTest, FeedsCanBeMigrated) {
  test_url_loader_factory_.AddResponse(OldSourcesURL(), kV1PublishersResponse,
                                       net::HTTP_OK);
  SetSubscribedSources({"111", "222", "333", "444"});

  auto migrated_count = MaybeMigrateFeeds({"111", "222"});
  EXPECT_EQ(2u, migrated_count);

  auto migrated = GetDirectFeeds();
  ASSERT_EQ(2u, migrated.size());

  const auto& f1 = migrated[0];
  EXPECT_EQ("111", f1->publisher_id);
  EXPECT_EQ("https://tp1.example.com/feed", f1->feed_source);
  EXPECT_EQ("Test Publisher 1", f1->publisher_name);
  EXPECT_FALSE(CombinedSourceExists(f1->publisher_id));

  const auto& f2 = migrated[1];
  EXPECT_EQ("222", f2->publisher_id);
  EXPECT_EQ("https://tp2.example.com/feed", f2->feed_source);
  EXPECT_EQ("Test Publisher 2", f2->publisher_name);
  EXPECT_FALSE(CombinedSourceExists(f2->publisher_id));

  // We didn't migrate these sources, so they shouldn't get touched.
  EXPECT_TRUE(CombinedSourceExists("333"));
  EXPECT_TRUE(CombinedSourceExists("444"));
}

TEST_F(UnsupportedPublisherMigratorTest, MigrationDoesNotDuplicateFeeds) {
  test_url_loader_factory_.AddResponse(OldSourcesURL(), kV1PublishersResponse,
                                       net::HTTP_OK);
  SetSubscribedSources({"111", "222"});
  // Feed "tp1" has the same feed url as feed "111" https://tp1.example.com/feed
  SetDirectSources({"tp1"});

  auto migrated_count = MaybeMigrateFeeds({"111", "222"});

  // Feed should still count as migrated, but we shouldn't duplicate the entry.
  EXPECT_EQ(2u, migrated_count);

  auto direct_feeds = GetDirectFeeds();
  ASSERT_EQ(2u, direct_feeds.size());

  const auto& f1 = direct_feeds[1];
  EXPECT_EQ("tp1", f1->publisher_id);
  EXPECT_EQ("https://tp1.example.com/feed", f1->feed_source);
  EXPECT_EQ("Test Publisher tp1", f1->publisher_name);
  EXPECT_FALSE(CombinedSourceExists("111"));
  EXPECT_FALSE(CombinedSourceExists("tp1"));

  const auto& f2 = direct_feeds[0];
  EXPECT_EQ("222", f2->publisher_id);
  EXPECT_EQ("https://tp2.example.com/feed", f2->feed_source);
  EXPECT_EQ("Test Publisher 2", f2->publisher_name);
  EXPECT_FALSE(CombinedSourceExists(f2->publisher_id));
}

TEST_F(UnsupportedPublisherMigratorTest, UnknownFeedsAreNotMigrated) {
  test_url_loader_factory_.AddResponse(OldSourcesURL(), kV1PublishersResponse,
                                       net::HTTP_OK);
  SetSubscribedSources({"111", "secret feed"});

  auto migrated_count = MaybeMigrateFeeds({"111", "secret feed"});
  EXPECT_EQ(1u, migrated_count);

  EXPECT_FALSE(CombinedSourceExists("111"));

  // Because we didn't know how to migrate this, we should leave it alone.
  EXPECT_TRUE(CombinedSourceExists("secret feed"));
}

TEST_F(UnsupportedPublisherMigratorTest, NonSuccessDoesNotCrash) {
  SetSubscribedSources({"222"});
  test_url_loader_factory_.AddResponse(OldSourcesURL(), "",
                                       net::HTTP_SERVICE_UNAVAILABLE);
  auto migrated = MaybeMigrateFeeds({"222"});
  EXPECT_EQ(0u, migrated);
  EXPECT_TRUE(CombinedSourceExists("222"));
  EXPECT_EQ(0u, GetDirectFeeds().size());
}

}  // namespace brave_news
