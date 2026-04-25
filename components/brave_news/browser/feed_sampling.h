// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_SAMPLING_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_SAMPLING_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/functional/function_ref.h"
#include "base/rand_util.h"
#include "base/types/id_type.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"

namespace brave_news {

// An opaque ID used to represent an interned channel ID or publisher ID. These
// IDs are used for efficient comparison during article selection in the feed
// building process.
using NameId = base::IdTypeU32<class NameIdClass>;

// A table that stores interned channel ID or publisher ID strings, and allows
// bidirectional mapping from string to ID and from ID to string.
class NameTable {
 public:
  NameTable();
  NameTable(NameTable&&);
  NameTable& operator=(NameTable&&);

  ~NameTable();

  // Adds a string to the table.
  NameId Add(std::string_view s);

  // Returns the ID associated with the specified string. If a string is not
  // found, a null `NameId` is returned.
  NameId Find(std::string_view s) const;

  // Returns a copy of the string associated with the specified ID. If there is
  // no such string in the table, the std::nullopt is returned.
  std::optional<std::string> GetString(NameId id) const;

  size_t size() const { return strings_.size(); }

 private:
  std::vector<std::unique_ptr<std::string>> strings_;

  // Note: map_ uses string_view keys pointing into strings_. This is safe
  // because unique_ptr provides reference stability.
  absl::flat_hash_map<std::string_view, NameId> map_;
};

struct ArticleMetadata {
  // The pop_recency of the article. This is used for discover cards, where we
  // don't consider the subscription status or visit_weighting.
  double pop_recency = 0;

  // The complete weighting of the article, combining the pop_score,
  // visit_weighting & subscribed_weighting.
  double weighting = 0;

  // Whether the source which this article comes from has been visited. This
  // only considers Publishers, not Channels.
  bool visited = false;

  // Whether any sources/channels that could cause this article to be shown are
  // subscribed. At this point, disabled sources have already been filtered out.
  bool subscribed = false;

  // Whether the source/channels of this article are "discoverable": this is a
  // selection of articles outside the user's explicit interests. Sensitive
  // content should not be used for discovery.
  bool discoverable = false;

  // Interned publisher ID.
  NameId publisher_id;

  // All the channels this Article belongs to (interned IDs).
  absl::flat_hash_set<NameId> channels;

  ArticleMetadata();
  ArticleMetadata(const ArticleMetadata&) = delete;
  ArticleMetadata& operator=(const ArticleMetadata&) = delete;
  ArticleMetadata& operator=(ArticleMetadata&&);
  ArticleMetadata(ArticleMetadata&&);
  ~ArticleMetadata();
};

using ArticleInfo = std::tuple<mojom::FeedItemMetadataPtr, ArticleMetadata>;
using ArticleInfos = std::vector<ArticleInfo>;

// Gets a weighting for a specific article. This determines how likely an
// article is to be chosen.
using GetWeighting =
    base::FunctionRef<double(const mojom::FeedItemMetadataPtr& data,
                             const ArticleMetadata& meta)>;

using PublisherChannels =
    absl::flat_hash_map<NameId, absl::flat_hash_set<NameId>>;

// PickArticles is a strategy used to pick articles (for example, taking the
// first article). Different feeds use different strategies for picking
// articles. This is a non-owning reference, so the callable must outlive the
// FunctionRef.
using PickArticles =
    base::FunctionRef<std::optional<size_t>(const ArticleInfos& infos)>;
using ContentGroup = std::pair<NameId, bool>;

template <typename T>
T PickRandom(const base::span<T>& items) {
  CHECK(!items.empty());
  // Note: RandInt is inclusive, hence the minus 1
  return items[base::RandInt(0, items.size() - 1)];
}

// Sample across subscribed channels (direct and native) and publishers.
ContentGroup SampleContentGroup(
    base::span<const ContentGroup> eligible_content_groups);

// Randomly true/false with equal probability.
bool TossCoin();

// This is a Box Muller transform for getting a normally distributed value
// between [0, 1)
// https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
double GetNormal();

// Returns a normally distributed value between min (inclusive) and max
// (exclusive).
int GetNormal(int min, int max);

// These functions are for deciding which article from a list to select. They
// should either:
// 1. Return a valid index into a list of articles.
// 2. Return -1 if there is no valid article to pick.
std::optional<size_t> PickFirstIndex(const ArticleInfos& articles);
std::optional<size_t> PickRouletteWithWeighting(const ArticleInfos& articles,
                                                GetWeighting get_weighting);
std::optional<size_t> PickRoulette(const ArticleInfos& articles);
std::optional<size_t> PickChannelRoulette(const ArticleInfos& articles,
                                          NameId channel_id);
std::optional<size_t> PickDiscoveryRoulette(const ArticleInfos& articles);
std::optional<size_t> PickContentGroupRoulette(
    const ArticleInfos& articles,
    const ContentGroup& content_group,
    const PublisherChannels& publisher_channels,
    bool require_image);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_SAMPLING_H_
