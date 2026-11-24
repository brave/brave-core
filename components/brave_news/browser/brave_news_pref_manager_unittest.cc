// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_pref_manager.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_set.h"
#include "base/json/values_util.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

class BraveNewsPrefManagerTest : public testing::Test {
 public:
  BraveNewsPrefManagerTest() {
    prefs::RegisterProfilePrefs(pref_service_.registry());

    pref_manager_ = std::make_unique<BraveNewsPrefManager>(pref_service_);
  }

 protected:
  std::optional<base::Time> GetSourceChangedAt(const std::string& id) {
    const auto* entry =
        pref_service_.GetDict(prefs::kBraveNewsSources).FindDict(id);
    return entry ? base::ValueToTime(entry->Find(prefs::kBraveNewsKeyChangedAt))
                 : std::nullopt;
  }

  std::optional<base::Time> GetDirectFeedChangedAt(const std::string& id) {
    const auto* entry =
        pref_service_.GetDict(prefs::kBraveNewsDirectFeeds).FindDict(id);
    return entry ? base::ValueToTime(entry->Find(prefs::kBraveNewsKeyChangedAt))
                 : std::nullopt;
  }

  std::optional<base::Time> GetChannelChangedAt(const std::string& locale,
                                                const std::string& channel) {
    const auto* locale_dict =
        pref_service_.GetDict(prefs::kBraveNewsChannels).FindDict(locale);
    const auto* entry = locale_dict ? locale_dict->FindDict(channel) : nullptr;
    return entry ? base::ValueToTime(entry->Find(prefs::kBraveNewsKeyChangedAt))
                 : std::nullopt;
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  sync_preferences::TestingPrefServiceSyncable pref_service_;

  std::unique_ptr<BraveNewsPrefManager> pref_manager_;
};

TEST_F(BraveNewsPrefManagerTest, CanAddDirectFeed) {
  EXPECT_NE("", pref_manager_->AddDirectPublisher(GURL("https://example.com"),
                                                  "Example"));
}

TEST_F(BraveNewsPrefManagerTest, CantAddDuplicateFeed) {
  auto id = pref_manager_->AddDirectPublisher(GURL("https://example.com"),
                                              "Example 1");

  // As the feed is already present, this should be the same publisher.
  EXPECT_EQ(id, pref_manager_->AddDirectPublisher(GURL("https://example.com"),
                                                  "Example 2"));
}

TEST_F(BraveNewsPrefManagerTest, EmptyTitleFallsBackToFeedSource) {
  constexpr char kFeedSource[] = "https://example.com/";
  auto id = pref_manager_->AddDirectPublisher(GURL(kFeedSource), "");
  auto subscriptions = pref_manager_->GetSubscriptions().direct_feeds();
  ASSERT_EQ(1u, subscriptions.size());

  for (const auto& subscription : subscriptions) {
    EXPECT_EQ(id, subscription.id);
    EXPECT_EQ(kFeedSource, subscription.title);
    EXPECT_EQ(kFeedSource, subscription.url.spec());
  }
}

TEST_F(BraveNewsPrefManagerTest, ChannelsDiffIsSane) {
  auto one = pref_manager_->GetSubscriptions();
  pref_manager_->SetChannelSubscribed("en_US", "News", true);

  auto two = pref_manager_->GetSubscriptions();
  auto diff = two.DiffChannels(one);
  ASSERT_EQ(1u, diff.changed.size());
  EXPECT_EQ("News", diff.changed[0]);

  pref_manager_->SetChannelSubscribed("en_NZ", "News", true);
  auto three = pref_manager_->GetSubscriptions();
  ASSERT_EQ(0u, three.DiffChannels(two).changed.size());

  pref_manager_->SetChannelSubscribed("en_NZ", "The Spinoff", true);
  pref_manager_->SetChannelSubscribed("en_NZ", "Politics", true);
  auto four = pref_manager_->GetSubscriptions();

  pref_manager_->SetChannelSubscribed("en_NZ", "FooBar", true);
  pref_manager_->SetChannelSubscribed("en_NZ", "Politics", false);
  auto five = pref_manager_->GetSubscriptions();

  diff = five.DiffChannels(four);
  EXPECT_EQ(2u, diff.changed.size());
  EXPECT_TRUE(std::ranges::contains(diff.changed, "Politics"));
  EXPECT_TRUE(std::ranges::contains(diff.changed, "FooBar"));
}

TEST_F(BraveNewsPrefManagerTest, PublishersDiffIsSane) {
  pref_manager_->SetPublisherSubscribed("One", mojom::UserEnabled::ENABLED);
  pref_manager_->SetPublisherSubscribed("Two", mojom::UserEnabled::ENABLED);
  pref_manager_->SetPublisherSubscribed("Three", mojom::UserEnabled::ENABLED);
  pref_manager_->SetPublisherSubscribed("Four", mojom::UserEnabled::DISABLED);
  pref_manager_->SetPublisherSubscribed("Five", mojom::UserEnabled::DISABLED);
  pref_manager_->SetPublisherSubscribed("Six", mojom::UserEnabled::DISABLED);
  auto direct_one =
      pref_manager_->AddDirectPublisher(GURL("https://example.com"), "");
  pref_manager_->AddDirectPublisher(GURL("https://foobar.com"), "");

  auto s1 = pref_manager_->GetSubscriptions();
  pref_manager_->SetPublisherSubscribed("One",
                                        mojom::UserEnabled::NOT_MODIFIED);
  pref_manager_->SetPublisherSubscribed("Two", mojom::UserEnabled::DISABLED);
  pref_manager_->SetPublisherSubscribed("Four",
                                        mojom::UserEnabled::NOT_MODIFIED);
  pref_manager_->SetPublisherSubscribed("Five", mojom::UserEnabled::ENABLED);
  pref_manager_->SetPublisherSubscribed(direct_one,
                                        mojom::UserEnabled::DISABLED);
  auto direct_three =
      pref_manager_->AddDirectPublisher(GURL("https://foo.nz"), "");

  auto s2 = pref_manager_->GetSubscriptions();
  auto diff = s2.DiffPublishers(s1);

  base::flat_set<std::string> distinct_changes(diff.changed);
  EXPECT_EQ(5u, distinct_changes.size());
  EXPECT_TRUE(distinct_changes.contains("One"));
  EXPECT_TRUE(distinct_changes.contains("Two"));
  EXPECT_TRUE(distinct_changes.contains("Four"));
  EXPECT_TRUE(distinct_changes.contains("Five"));
  EXPECT_TRUE(distinct_changes.contains(direct_three));

  EXPECT_EQ(1u, diff.removed.size());
  EXPECT_TRUE(std::ranges::contains(diff.removed, direct_one));
}

TEST_F(BraveNewsPrefManagerTest, DirectFeedCanBeInspectedAndRemoved) {
  auto id =
      pref_manager_->AddDirectPublisher(GURL("https://example.com"), "Example");

  auto parsed = pref_manager_->GetSubscriptions().direct_feeds();
  ASSERT_EQ(parsed.size(), 1u);
  EXPECT_EQ(id, parsed[0].id);
  EXPECT_EQ("Example", parsed[0].title);
  EXPECT_EQ(GURL("https://example.com"), parsed[0].url);

  pref_manager_->SetPublisherSubscribed(id, mojom::UserEnabled::DISABLED);
  parsed = pref_manager_->GetSubscriptions().direct_feeds();
  EXPECT_EQ(0u, parsed.size());
}

TEST_F(BraveNewsPrefManagerTest, CanToggleChannelSubscribed) {
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));

  pref_manager_->SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));

  pref_manager_->SetChannelSubscribed("en_US", "Test", false);
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
}

TEST_F(BraveNewsPrefManagerTest,
       ChangingAChannelInOneLocaleDoesNotAffectOtherLocales) {
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_->SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_->SetChannelSubscribed("ja_JA", "Test", true);
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_->SetChannelSubscribed("en_US", "Test", false);
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_->SetChannelSubscribed("ja_JA", "Test", false);
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));
}

TEST_F(BraveNewsPrefManagerTest, NoChannelsNoChannelLocales) {
  EXPECT_EQ(0u, pref_manager_->GetSubscriptions().GetChannelLocales().size());
}

TEST_F(BraveNewsPrefManagerTest, SubscribedChannelLocalesIncluded) {
  pref_manager_->SetChannelSubscribed("en_US", "Test", true);

  auto locales = pref_manager_->GetSubscriptions().GetChannelLocales();
  ASSERT_EQ(1u, locales.size());
  EXPECT_EQ("en_US", locales[0]);

  pref_manager_->SetChannelSubscribed("en_US", "Foo", true);
  locales = pref_manager_->GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(1u, locales.size());

  pref_manager_->SetChannelSubscribed("ja_JA", "Foo", true);
  locales = pref_manager_->GetSubscriptions().GetChannelLocales();
  ASSERT_EQ(2u, locales.size());
  EXPECT_EQ("en_US", locales[0]);
  EXPECT_EQ("ja_JA", locales[1]);
}

TEST_F(BraveNewsPrefManagerTest, LocaleWithNoSubscribedChannelsIsNotIncluded) {
  pref_manager_->SetChannelSubscribed("en_US", "Test", true);

  auto locales = pref_manager_->GetSubscriptions().GetChannelLocales();
  ASSERT_EQ(1u, locales.size());
  EXPECT_EQ("en_US", locales[0]);

  pref_manager_->SetChannelSubscribed("en_US", "Test", false);
  locales = pref_manager_->GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(0u, locales.size());
}

TEST_F(BraveNewsPrefManagerTest, ChannelMigrationsAreApplied) {
  pref_manager_->SetChannelSubscribed("en_US", "Tech News", true);
  pref_manager_->SetChannelSubscribed("en_US", "Sport", true);

  EXPECT_FALSE(pref_manager_->GetSubscriptions().GetChannelSubscribed(
      "en_US", "Tech News"));
  EXPECT_TRUE(pref_manager_->GetSubscriptions().GetChannelSubscribed(
      "en_US", "Technology"));
  EXPECT_FALSE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Sport"));
  EXPECT_TRUE(pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US",
                                                                     "Sports"));
}

TEST_F(BraveNewsPrefManagerTest, SetPublisherSubscribedRecordsChangedAt) {
  const base::Time first = base::Time::Now();
  pref_manager_->SetPublisherSubscribed("One", mojom::UserEnabled::ENABLED);
  pref_manager_->SetPublisherSubscribed("Two", mojom::UserEnabled::DISABLED);
  EXPECT_EQ(first, GetSourceChangedAt("One"));
  EXPECT_EQ(first, GetSourceChangedAt("Two"));

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time second = base::Time::Now();
  pref_manager_->SetPublisherSubscribed("Two", mojom::UserEnabled::ENABLED);
  EXPECT_EQ(first, GetSourceChangedAt("One"));
  EXPECT_EQ(second, GetSourceChangedAt("Two"));

  auto subscriptions = pref_manager_->GetSubscriptions();
  EXPECT_TRUE(subscriptions.enabled_publishers().contains("One"));
  EXPECT_TRUE(subscriptions.enabled_publishers().contains("Two"));
  EXPECT_TRUE(subscriptions.disabled_publishers().empty());

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time third = base::Time::Now();
  pref_manager_->SetPublisherSubscribed("Two",
                                        mojom::UserEnabled::NOT_MODIFIED);
  EXPECT_EQ(third, GetSourceChangedAt("Two"));
}

TEST_F(BraveNewsPrefManagerTest, AddDirectPublisherRecordsChangedAt) {
  const base::Time added = base::Time::Now();
  auto id =
      pref_manager_->AddDirectPublisher(GURL("https://example.com"), "Example");
  EXPECT_EQ(added, GetDirectFeedChangedAt(id));

  // Re-adding an existing feed is not a change.
  task_environment_.AdvanceClock(base::Minutes(1));
  EXPECT_EQ(id, pref_manager_->AddDirectPublisher(GURL("https://example.com"),
                                                  "Example"));
  EXPECT_EQ(added, GetDirectFeedChangedAt(id));
}

TEST_F(BraveNewsPrefManagerTest, SetChannelSubscribedRecordsChangedAt) {
  const base::Time first = base::Time::Now();
  pref_manager_->SetChannelSubscribed("en_US", "Test", true);
  EXPECT_EQ(first, GetChannelChangedAt("en_US", "Test"));

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time second = base::Time::Now();
  pref_manager_->SetChannelSubscribed("ja_JA", "Test", true);
  EXPECT_EQ(first, GetChannelChangedAt("en_US", "Test"));
  EXPECT_EQ(second, GetChannelChangedAt("ja_JA", "Test"));

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time third = base::Time::Now();
  pref_manager_->SetChannelSubscribed("en_US", "Test", false);
  EXPECT_EQ(third, GetChannelChangedAt("en_US", "Test"));
}

TEST_F(BraveNewsPrefManagerTest, MigratedPrefsAreReadable) {
  {
    base::DictValue sources;
    sources.Set("enabled_publisher", true);
    sources.Set("disabled_publisher", false);
    pref_service_.SetDict(prefs::kBraveNewsSources, std::move(sources));

    base::DictValue channels;
    channels.Set("en_US", base::DictValue().Set("Top News", true));
    pref_service_.SetDict(prefs::kBraveNewsChannels, std::move(channels));

    base::DictValue direct_feeds;
    direct_feeds.Set(
        "feed_id",
        base::DictValue()
            .Set(prefs::kBraveNewsDirectFeedsKeySource, "https://example.com/")
            .Set(prefs::kBraveNewsDirectFeedsKeyTitle, "Example"));
    pref_service_.SetDict(prefs::kBraveNewsDirectFeeds,
                          std::move(direct_feeds));
  }

  prefs::MigrateProfilePrefs(&pref_service_);

  auto subscriptions = pref_manager_->GetSubscriptions();
  EXPECT_EQ(1u, subscriptions.enabled_publishers().size());
  EXPECT_TRUE(subscriptions.enabled_publishers().contains("enabled_publisher"));
  EXPECT_EQ(1u, subscriptions.disabled_publishers().size());
  EXPECT_TRUE(
      subscriptions.disabled_publishers().contains("disabled_publisher"));
  EXPECT_TRUE(subscriptions.GetChannelSubscribed("en_US", "Top News"));
  ASSERT_EQ(1u, subscriptions.direct_feeds().size());
  EXPECT_EQ("feed_id", subscriptions.direct_feeds()[0].id);
  EXPECT_EQ(GURL("https://example.com/"), subscriptions.direct_feeds()[0].url);
  EXPECT_EQ("Example", subscriptions.direct_feeds()[0].title);
}

TEST_F(BraveNewsPrefManagerTest, ResettingPublisherLeavesTombstone) {
  pref_manager_->SetPublisherSubscribed("One", mojom::UserEnabled::ENABLED);
  pref_manager_->SetPublisherSubscribed("Two", mojom::UserEnabled::DISABLED);

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time deleted_at = base::Time::Now();
  pref_manager_->SetPublisherSubscribed("One",
                                        mojom::UserEnabled::NOT_MODIFIED);
  pref_manager_->SetPublisherSubscribed("Two",
                                        mojom::UserEnabled::NOT_MODIFIED);

  const auto tombstone =
      base::DictValue()
          .Set(prefs::kBraveNewsKeyDeleted, true)
          .Set(prefs::kBraveNewsKeyChangedAt, base::TimeToValue(deleted_at));
  const auto& sources = pref_service_.GetDict(prefs::kBraveNewsSources);
  EXPECT_EQ(tombstone, *sources.FindDict("One"));
  EXPECT_EQ(tombstone, *sources.FindDict("Two"));

  auto subscriptions = pref_manager_->GetSubscriptions();
  EXPECT_TRUE(subscriptions.enabled_publishers().empty());
  EXPECT_TRUE(subscriptions.disabled_publishers().empty());

  // Re-subscribing replaces the tombstone.
  pref_manager_->SetPublisherSubscribed("One", mojom::UserEnabled::ENABLED);
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().enabled_publishers().contains("One"));
}

TEST_F(BraveNewsPrefManagerTest, ResettingUnknownPublisherDoesNotTombstone) {
  pref_manager_->SetPublisherSubscribed("Unknown",
                                        mojom::UserEnabled::NOT_MODIFIED);
  EXPECT_FALSE(
      pref_service_.GetDict(prefs::kBraveNewsSources).contains("Unknown"));
}

TEST_F(BraveNewsPrefManagerTest, UnsubscribingChannelLeavesTombstone) {
  pref_manager_->SetChannelSubscribed("en_US", "Test", true);

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time deleted_at = base::Time::Now();
  pref_manager_->SetChannelSubscribed("en_US", "Test", false);

  const auto* locale =
      pref_service_.GetDict(prefs::kBraveNewsChannels).FindDict("en_US");
  ASSERT_TRUE(locale);
  EXPECT_EQ(
      base::DictValue()
          .Set(prefs::kBraveNewsKeyDeleted, true)
          .Set(prefs::kBraveNewsKeyChangedAt, base::TimeToValue(deleted_at)),
      *locale->FindDict("Test"));

  auto subscriptions = pref_manager_->GetSubscriptions();
  EXPECT_FALSE(subscriptions.GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(subscriptions.GetChannelLocales().empty());

  // Re-subscribing replaces the tombstone.
  pref_manager_->SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(
      pref_manager_->GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
}

TEST_F(BraveNewsPrefManagerTest, UnsubscribingUnknownChannelDoesNotTombstone) {
  pref_manager_->SetChannelSubscribed("en_US", "Test", false);
  const auto* locale =
      pref_service_.GetDict(prefs::kBraveNewsChannels).FindDict("en_US");
  EXPECT_FALSE(locale && locale->contains("Test"));
}

TEST_F(BraveNewsPrefManagerTest, RemovingDirectFeedLeavesTombstone) {
  auto id = pref_manager_->AddDirectPublisher(GURL("https://example.com/"),
                                              "Example");

  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time deleted_at = base::Time::Now();
  pref_manager_->SetPublisherSubscribed(id, mojom::UserEnabled::DISABLED);

  EXPECT_EQ(
      base::DictValue()
          .Set(prefs::kBraveNewsKeyDeleted, true)
          .Set(prefs::kBraveNewsKeyChangedAt, base::TimeToValue(deleted_at))
          .Set(prefs::kBraveNewsDirectFeedsKeySource, "https://example.com/"),
      *pref_service_.GetDict(prefs::kBraveNewsDirectFeeds).FindDict(id));
  EXPECT_TRUE(pref_manager_->GetSubscriptions().direct_feeds().empty());

  // Re-adding the feed revives the tombstone.
  task_environment_.AdvanceClock(base::Minutes(1));
  const base::Time readded_at = base::Time::Now();
  EXPECT_EQ(id, pref_manager_->AddDirectPublisher(GURL("https://example.com/"),
                                                  "Example 2"));
  EXPECT_EQ(readded_at, GetDirectFeedChangedAt(id));
  auto direct_feeds = pref_manager_->GetSubscriptions().direct_feeds();
  ASSERT_EQ(1u, direct_feeds.size());
  EXPECT_EQ(id, direct_feeds[0].id);
  EXPECT_EQ("Example 2", direct_feeds[0].title);
}

}  // namespace brave_news
