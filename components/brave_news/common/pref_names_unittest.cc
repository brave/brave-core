// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/pref_names.h"

#include <string>
#include <utility>

#include "base/json/values_util.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_news::prefs {

namespace {

std::string IdFor(const std::string& url) {
  return GetDirectFeedId(GURL(url));
}

}  // namespace

class BraveNewsPrefsMigrationTest : public testing::Test {
 public:
  BraveNewsPrefsMigrationTest() {
    RegisterProfilePrefs(pref_service_.registry());
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(BraveNewsPrefsMigrationTest, MigratesLegacyPrefs) {
  pref_service_.SetDict(kBraveNewsSources, base::test::ParseJsonDict(R"({
    "enabled_publisher": true,
    "disabled_publisher": false
  })"));
  pref_service_.SetDict(kBraveNewsChannels, base::test::ParseJsonDict(R"({
    "en_US": {"Top News": true, "Technology": true},
    "ja_JP": {"Business": true}
  })"));
  pref_service_.SetDict(kBraveNewsDirectFeeds, base::test::ParseJsonDict(R"({
    "feed_id": {"source": "https://example.com/feed", "title": "Example"}
  })"));

  MigrateProfilePrefs(&pref_service_);

  const base::Value now = base::TimeToValue(base::Time::Now());
  EXPECT_EQ(
      pref_service_.GetDict(kBraveNewsSources),
      base::DictValue()
          .Set("enabled_publisher", base::DictValue()
                                        .Set("enabled", true)
                                        .Set("changed_at", now.Clone()))
          .Set("disabled_publisher", base::DictValue()
                                         .Set("enabled", false)
                                         .Set("changed_at", now.Clone())));

  EXPECT_EQ(
      pref_service_.GetDict(kBraveNewsChannels),
      base::DictValue()
          .Set("en_US",
               base::DictValue()
                   .Set("Top News", base::DictValue()
                                        .Set("subscribed", true)
                                        .Set("changed_at", now.Clone()))
                   .Set("Technology", base::DictValue()
                                          .Set("subscribed", true)
                                          .Set("changed_at", now.Clone())))
          .Set("ja_JP", base::DictValue().Set(
                            "Business", base::DictValue()
                                            .Set("subscribed", true)
                                            .Set("changed_at", now.Clone()))));

  EXPECT_EQ(pref_service_.GetDict(kBraveNewsDirectFeeds),
            base::DictValue().Set(IdFor("https://example.com/feed"),
                                  base::DictValue()
                                      .Set("source", "https://example.com/feed")
                                      .Set("title", "Example")
                                      .Set("changed_at", now.Clone())));
}

TEST_F(BraveNewsPrefsMigrationTest, MigrationIsIdempotent) {
  pref_service_.SetDict(kBraveNewsSources,
                        base::test::ParseJsonDict(R"({"publisher": true})"));
  pref_service_.SetDict(
      kBraveNewsChannels,
      base::test::ParseJsonDict(R"({"en_US": {"Top News": true}})"));
  pref_service_.SetDict(kBraveNewsDirectFeeds, base::test::ParseJsonDict(R"({
    "feed_id": {"source": "https://example.com/feed", "title": "Example"}
  })"));

  MigrateProfilePrefs(&pref_service_);
  const base::Value sources = pref_service_.GetValue(kBraveNewsSources).Clone();
  const base::Value channels =
      pref_service_.GetValue(kBraveNewsChannels).Clone();
  const base::Value direct_feeds =
      pref_service_.GetValue(kBraveNewsDirectFeeds).Clone();

  // Running again later must not bump `changed_at` on already-migrated entries.
  task_environment_.AdvanceClock(base::Days(1));
  MigrateProfilePrefs(&pref_service_);

  EXPECT_EQ(sources, pref_service_.GetValue(kBraveNewsSources));
  EXPECT_EQ(channels, pref_service_.GetValue(kBraveNewsChannels));
  EXPECT_EQ(direct_feeds, pref_service_.GetValue(kBraveNewsDirectFeeds));
}

TEST_F(BraveNewsPrefsMigrationTest, MigratesOnlyLegacyEntries) {
  const base::Value earlier = base::TimeToValue(base::Time::Now());
  task_environment_.AdvanceClock(base::Days(1));
  const base::Value now = base::TimeToValue(base::Time::Now());

  pref_service_.SetDict(
      kBraveNewsSources,
      base::DictValue()
          .Set("legacy", true)
          .Set("current", base::DictValue()
                              .Set("enabled", false)
                              .Set("changed_at", earlier.Clone())));
  pref_service_.SetDict(
      kBraveNewsChannels,
      base::DictValue().Set(
          "en_US",
          base::DictValue()
              .Set("Legacy", true)
              .Set("Current", base::DictValue()
                                  .Set("subscribed", true)
                                  .Set("changed_at", earlier.Clone()))));
  pref_service_.SetDict(
      kBraveNewsDirectFeeds,
      base::DictValue()
          .Set(IdFor("https://a.com/"),
               base::DictValue().Set("source", "https://a.com/"))
          .Set(IdFor("https://b.com/"),
               base::DictValue()
                   .Set("source", "https://b.com/")
                   .Set("changed_at", earlier.Clone())));

  MigrateProfilePrefs(&pref_service_);

  const auto& sources = pref_service_.GetDict(kBraveNewsSources);
  EXPECT_EQ(
      *sources.FindDict("legacy"),
      base::DictValue().Set("enabled", true).Set("changed_at", now.Clone()));
  EXPECT_EQ(*sources.FindDict("current"),
            base::DictValue()
                .Set("enabled", false)
                .Set("changed_at", earlier.Clone()));

  const auto* channels =
      pref_service_.GetDict(kBraveNewsChannels).FindDict("en_US");
  ASSERT_TRUE(channels);
  EXPECT_EQ(
      *channels->FindDict("Legacy"),
      base::DictValue().Set("subscribed", true).Set("changed_at", now.Clone()));
  EXPECT_EQ(*channels->FindDict("Current"),
            base::DictValue()
                .Set("subscribed", true)
                .Set("changed_at", earlier.Clone()));

  const auto& direct_feeds = pref_service_.GetDict(kBraveNewsDirectFeeds);
  EXPECT_EQ(*direct_feeds.FindDict(IdFor("https://a.com/")),
            base::DictValue()
                .Set("source", "https://a.com/")
                .Set("changed_at", now.Clone()));
  EXPECT_EQ(*direct_feeds.FindDict(IdFor("https://b.com/")),
            base::DictValue()
                .Set("source", "https://b.com/")
                .Set("changed_at", earlier.Clone()));
}

TEST_F(BraveNewsPrefsMigrationTest, EmptyPrefsAreUnchanged) {
  MigrateProfilePrefs(&pref_service_);

  EXPECT_FALSE(pref_service_.HasPrefPath(kBraveNewsSources));
  EXPECT_FALSE(pref_service_.HasPrefPath(kBraveNewsChannels));
  EXPECT_FALSE(pref_service_.HasPrefPath(kBraveNewsDirectFeeds));
}

TEST_F(BraveNewsPrefsMigrationTest, DirectFeedIdIsStableHashOfUrl) {
  EXPECT_EQ(IdFor("https://example.com/feed"),
            IdFor("https://example.com/feed"));
  EXPECT_NE(IdFor("https://example.com/feed"),
            IdFor("https://example.com/other"));
  // SHA-256, hex encoded.
  EXPECT_EQ(64u, IdFor("https://example.com/feed").size());
}

TEST_F(BraveNewsPrefsMigrationTest, RekeysDirectFeedsByUrlHash) {
  pref_service_.SetDict(kBraveNewsDirectFeeds, base::test::ParseJsonDict(R"({
    "uuid-live": {"source": "https://a.com/feed", "title": "A"},
    "uuid-deleted": {"source": "https://b.com/feed", "deleted": true,
                     "changed_at": "1"},
    "uuid-no-source": {"title": "No source"},
    "uuid-invalid-source": {"source": "not a url", "title": "Invalid"}
  })"));

  MigrateProfilePrefs(&pref_service_);

  const base::Value now = base::TimeToValue(base::Time::Now());
  EXPECT_EQ(
      pref_service_.GetDict(kBraveNewsDirectFeeds),
      base::DictValue()
          .Set(IdFor("https://a.com/feed"),
               base::DictValue()
                   .Set("source", "https://a.com/feed")
                   .Set("title", "A")
                   .Set("changed_at", now.Clone()))
          .Set(IdFor("https://b.com/feed"),
               base::DictValue()
                   .Set("source", "https://b.com/feed")
                   .Set("deleted", true)
                   .Set("changed_at", "1"))
          .Set("uuid-no-source", base::DictValue()
                                     .Set("title", "No source")
                                     .Set("changed_at", now.Clone()))
          .Set("uuid-invalid-source", base::DictValue()
                                          .Set("source", "not a url")
                                          .Set("title", "Invalid")
                                          .Set("changed_at", now.Clone())));
}

TEST_F(BraveNewsPrefsMigrationTest, RekeyingDuplicateDirectFeedsPrefersLive) {
  const base::Value older = base::TimeToValue(base::Time::Now());
  task_environment_.AdvanceClock(base::Days(1));
  const base::Value newer = base::TimeToValue(base::Time::Now());

  pref_service_.SetDict(
      kBraveNewsDirectFeeds,
      base::DictValue()
          .Set("uuid-1", base::DictValue()
                             .Set("source", "https://a.com/feed")
                             .Set("title", "Live")
                             .Set("changed_at", older.Clone()))
          .Set("uuid-2", base::DictValue()
                             .Set("source", "https://a.com/feed")
                             .Set("deleted", true)
                             .Set("changed_at", newer.Clone())));

  MigrateProfilePrefs(&pref_service_);

  EXPECT_EQ(pref_service_.GetDict(kBraveNewsDirectFeeds),
            base::DictValue().Set(IdFor("https://a.com/feed"),
                                  base::DictValue()
                                      .Set("source", "https://a.com/feed")
                                      .Set("title", "Live")
                                      .Set("changed_at", older.Clone())));
}

TEST_F(BraveNewsPrefsMigrationTest,
       RekeyingDuplicateDirectFeedsPrefersMostRecent) {
  const base::Value older = base::TimeToValue(base::Time::Now());
  task_environment_.AdvanceClock(base::Days(1));
  const base::Value newer = base::TimeToValue(base::Time::Now());

  pref_service_.SetDict(
      kBraveNewsDirectFeeds,
      base::DictValue()
          .Set("uuid-1", base::DictValue()
                             .Set("source", "https://a.com/feed")
                             .Set("title", "Older")
                             .Set("changed_at", older.Clone()))
          .Set("uuid-2", base::DictValue()
                             .Set("source", "https://a.com/feed")
                             .Set("title", "Newer")
                             .Set("changed_at", newer.Clone())));

  MigrateProfilePrefs(&pref_service_);

  const auto& direct_feeds = pref_service_.GetDict(kBraveNewsDirectFeeds);
  EXPECT_EQ(1u, direct_feeds.size());
  const auto* feed = direct_feeds.FindDict(IdFor("https://a.com/feed"));
  ASSERT_TRUE(feed);
  EXPECT_EQ("Newer", *feed->FindString("title"));
}

}  // namespace brave_news::prefs
