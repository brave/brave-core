// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/subscriptions_snapshot.h"

#include "base/containers/contains.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_news {

TEST(BraveNewsSubscriptionsSnapshot, DirectFeedsAreDiffed) {
  SubscriptionsSnapshot one(
      {}, {},
      {
          {.id = "one", .url = GURL("https://one.com"), .title = "One"},
          {.id = "two", .url = GURL("https://two.com"), .title = "Two"},
      },
      {

      });
  SubscriptionsSnapshot two(
      {}, {},
      {
          {.id = "one", .url = GURL("https://one.com"), .title = "One"},
          {.id = "three", .url = GURL("https://three.com"), .title = "Three"},
      },
      {});

  auto diff = two.DiffPublishers(one);
  EXPECT_EQ(1u, diff.changed.size());
  EXPECT_TRUE(base::Contains(diff.changed, "three"));

  EXPECT_EQ(1u, diff.removed.size());
  EXPECT_TRUE(base::Contains(diff.removed, "two"));
}

TEST(BraveNewsSubscriptionsSnapshot, ChannelsAreDiffed) {
  SubscriptionsSnapshot one({}, {}, {}, {{"en_US", {"One", "Two"}}});
  SubscriptionsSnapshot two({}, {}, {}, {{"en_US", {"One", "Three"}}});

  // Note: Channels aren't removed, their status is changed.
  auto diff = two.DiffChannels(one);
  EXPECT_EQ(2u, diff.changed.size());
  EXPECT_TRUE(base::Contains(diff.changed, "Two"));
  EXPECT_TRUE(base::Contains(diff.changed, "Three"));
}

TEST(BraveNewsSubscriptionsSnapshot, PublishersAreDiffed) {
  SubscriptionsSnapshot one({"One", "Two"}, {"Three", "Four"}, {}, {});
  SubscriptionsSnapshot two({"One", "Five"}, {"Three", "Six"}, {}, {});

  // Note: Publishers aren't removed but their status is changed.
  auto diff = two.DiffPublishers(one);
  EXPECT_EQ(4u, diff.changed.size());
  EXPECT_TRUE(base::Contains(diff.changed, "Five"));
  EXPECT_TRUE(base::Contains(diff.changed, "Six"));
  EXPECT_TRUE(base::Contains(diff.changed, "Two"));
  EXPECT_TRUE(base::Contains(diff.changed, "Four"));
}

TEST(BraveNewsSubscriptionsSnapshot, NoopHasNoDiff) {
  SubscriptionsSnapshot one({"One", "Two"}, {"Three", "Four"},
                            {
                                {.id = "direct",
                                 .url = GURL("https://direct.com"),
                                 .title = "Direct"},
                            },
                            {{{"en_US", {"c1", "c2"}}}});
  auto diff_publishers = one.DiffPublishers(one);
  EXPECT_EQ(0u, diff_publishers.changed.size());
  EXPECT_EQ(0u, diff_publishers.removed.size());

  auto diff_channels = one.DiffChannels(one);
  EXPECT_EQ(0u, diff_channels.changed.size());
  EXPECT_EQ(0u, diff_channels.removed.size());
}

}  // namespace brave_news
