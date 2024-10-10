// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/brave_news_pref_manager.h"

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

class BraveNewsPrefManagerTest : public testing::Test {
 public:
  BraveNewsPrefManagerTest() : pref_manager_(*profile_.GetPrefs()) {}

 protected:
  content::BrowserTaskEnvironment browser_task_environment_;

  TestingProfile profile_;
  BraveNewsPrefManager pref_manager_;
};

TEST_F(BraveNewsPrefManagerTest, CanAddDirectFeed) {
  EXPECT_NE("", pref_manager_.AddDirectPublisher(GURL("https://example.com"),
                                                 "Example"));
}

TEST_F(BraveNewsPrefManagerTest, CantAddDuplicateFeed) {
  auto id = pref_manager_.AddDirectPublisher(GURL("https://example.com"),
                                             "Example 1");

  // As the feed is already present, this should be the same publisher.
  EXPECT_EQ(id, pref_manager_.AddDirectPublisher(GURL("https://example.com"),
                                                 "Example 2"));
}

TEST_F(BraveNewsPrefManagerTest, EmptyTitleFallsBackToFeedSource) {
  constexpr char kFeedSource[] = "https://example.com/";
  auto id = pref_manager_.AddDirectPublisher(GURL(kFeedSource), "");
  auto subscriptions = pref_manager_.GetSubscriptions().direct_feeds();
  ASSERT_EQ(1u, subscriptions.size());

  for (const auto& subscription : subscriptions) {
    EXPECT_EQ(id, subscription.id);
    EXPECT_EQ(kFeedSource, subscription.title);
    EXPECT_EQ(kFeedSource, subscription.url.spec());
  }
}

TEST_F(BraveNewsPrefManagerTest, ChannelsDiffIsSane) {
  auto one = pref_manager_.GetSubscriptions();
  pref_manager_.SetChannelSubscribed("en_US", "News", true);

  auto two = pref_manager_.GetSubscriptions();
  auto diff = two.DiffChannels(one);
  ASSERT_EQ(1u, diff.changed.size());
  EXPECT_EQ("News", diff.changed[0]);

  pref_manager_.SetChannelSubscribed("en_NZ", "News", true);
  auto three = pref_manager_.GetSubscriptions();
  ASSERT_EQ(0u, three.DiffChannels(two).changed.size());

  pref_manager_.SetChannelSubscribed("en_NZ", "The Spinoff", true);
  pref_manager_.SetChannelSubscribed("en_NZ", "Politics", true);
  auto four = pref_manager_.GetSubscriptions();

  pref_manager_.SetChannelSubscribed("en_NZ", "FooBar", true);
  pref_manager_.SetChannelSubscribed("en_NZ", "Politics", false);
  auto five = pref_manager_.GetSubscriptions();

  diff = five.DiffChannels(four);
  EXPECT_EQ(2u, diff.changed.size());
  EXPECT_TRUE(base::Contains(diff.changed, "Politics"));
  EXPECT_TRUE(base::Contains(diff.changed, "FooBar"));
}

TEST_F(BraveNewsPrefManagerTest, PublishersDiffIsSane) {
  pref_manager_.SetPublisherSubscribed("One", mojom::UserEnabled::ENABLED);
  pref_manager_.SetPublisherSubscribed("Two", mojom::UserEnabled::ENABLED);
  pref_manager_.SetPublisherSubscribed("Three", mojom::UserEnabled::ENABLED);
  pref_manager_.SetPublisherSubscribed("Four", mojom::UserEnabled::DISABLED);
  pref_manager_.SetPublisherSubscribed("Five", mojom::UserEnabled::DISABLED);
  pref_manager_.SetPublisherSubscribed("Six", mojom::UserEnabled::DISABLED);
  auto direct_one =
      pref_manager_.AddDirectPublisher(GURL("https://example.com"), "");
  pref_manager_.AddDirectPublisher(GURL("https://foobar.com"), "");

  auto s1 = pref_manager_.GetSubscriptions();
  pref_manager_.SetPublisherSubscribed("One", mojom::UserEnabled::NOT_MODIFIED);
  pref_manager_.SetPublisherSubscribed("Two", mojom::UserEnabled::DISABLED);
  pref_manager_.SetPublisherSubscribed("Four",
                                       mojom::UserEnabled::NOT_MODIFIED);
  pref_manager_.SetPublisherSubscribed("Five", mojom::UserEnabled::ENABLED);
  pref_manager_.SetPublisherSubscribed(direct_one,
                                       mojom::UserEnabled::DISABLED);
  auto direct_three =
      pref_manager_.AddDirectPublisher(GURL("https://foo.nz"), "");

  auto s2 = pref_manager_.GetSubscriptions();
  auto diff = s2.DiffPublishers(s1);

  base::flat_set<std::string> distinct_changes(diff.changed);
  EXPECT_EQ(5u, distinct_changes.size());
  EXPECT_TRUE(base::Contains(distinct_changes, "One"));
  EXPECT_TRUE(base::Contains(distinct_changes, "Two"));
  EXPECT_TRUE(base::Contains(distinct_changes, "Four"));
  EXPECT_TRUE(base::Contains(distinct_changes, "Five"));
  EXPECT_TRUE(base::Contains(distinct_changes, direct_three));

  EXPECT_EQ(1u, diff.removed.size());
  EXPECT_TRUE(base::Contains(diff.removed, direct_one));
}

TEST_F(BraveNewsPrefManagerTest, DirectFeedCanBeInspectedAndRemoved) {
  auto id =
      pref_manager_.AddDirectPublisher(GURL("https://example.com"), "Example");

  auto parsed = pref_manager_.GetSubscriptions().direct_feeds();
  EXPECT_EQ(parsed.size(), 1u);
  EXPECT_EQ(id, parsed[0].id);
  EXPECT_EQ("Example", parsed[0].title);
  EXPECT_EQ(GURL("https://example.com"), parsed[0].url);

  pref_manager_.SetPublisherSubscribed(id, mojom::UserEnabled::DISABLED);
  parsed = pref_manager_.GetSubscriptions().direct_feeds();
  EXPECT_EQ(0u, parsed.size());
}

TEST_F(BraveNewsPrefManagerTest, CanToggleChannelSubscribed) {
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));

  pref_manager_.SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));

  pref_manager_.SetChannelSubscribed("en_US", "Test", false);
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
}

TEST_F(BraveNewsPrefManagerTest,
       ChangingAChannelInOneLocaleDoesNotAffectOtherLocales) {
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_.SetChannelSubscribed("en_US", "Test", true);
  EXPECT_TRUE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_.SetChannelSubscribed("ja_JA", "Test", true);
  EXPECT_TRUE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_.SetChannelSubscribed("en_US", "Test", false);
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_TRUE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));

  pref_manager_.SetChannelSubscribed("ja_JA", "Test", false);
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Test"));
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("ja_JA", "Test"));
}

TEST_F(BraveNewsPrefManagerTest, NoChannelsNoChannelLocales) {
  EXPECT_EQ(0u, pref_manager_.GetSubscriptions().GetChannelLocales().size());
}

TEST_F(BraveNewsPrefManagerTest, SubscribedChannelLocalesIncluded) {
  pref_manager_.SetChannelSubscribed("en_US", "Test", true);

  auto locales = pref_manager_.GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(1u, locales.size());
  EXPECT_EQ("en_US", locales[0]);

  pref_manager_.SetChannelSubscribed("en_US", "Foo", true);
  locales = pref_manager_.GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(1u, locales.size());

  pref_manager_.SetChannelSubscribed("ja_JA", "Foo", true);
  locales = pref_manager_.GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(2u, locales.size());
  EXPECT_EQ("en_US", locales[0]);
  EXPECT_EQ("ja_JA", locales[1]);
}

TEST_F(BraveNewsPrefManagerTest, LocaleWithNoSubscribedChannelsIsNotIncluded) {
  pref_manager_.SetChannelSubscribed("en_US", "Test", true);

  auto locales = pref_manager_.GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(1u, locales.size());
  EXPECT_EQ("en_US", locales[0]);

  pref_manager_.SetChannelSubscribed("en_US", "Test", false);
  locales = pref_manager_.GetSubscriptions().GetChannelLocales();
  EXPECT_EQ(0u, locales.size());
}

TEST_F(BraveNewsPrefManagerTest, ChannelMigrationsAreApplied) {
  pref_manager_.SetChannelSubscribed("en_US", "Tech News", true);
  pref_manager_.SetChannelSubscribed("en_US", "Sport", true);

  EXPECT_FALSE(pref_manager_.GetSubscriptions().GetChannelSubscribed(
      "en_US", "Tech News"));
  EXPECT_TRUE(pref_manager_.GetSubscriptions().GetChannelSubscribed(
      "en_US", "Technology"));
  EXPECT_FALSE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Sport"));
  EXPECT_TRUE(
      pref_manager_.GetSubscriptions().GetChannelSubscribed("en_US", "Sports"));
}
}  // namespace brave_news
