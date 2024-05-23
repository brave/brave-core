// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/feed_sampling.h"

#include <numeric>
#include <optional>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

namespace brave_news {

ArticleMetadata::ArticleMetadata() = default;
ArticleMetadata::~ArticleMetadata() = default;
ArticleMetadata::ArticleMetadata(ArticleMetadata&&) = default;
ArticleMetadata& ArticleMetadata::operator=(ArticleMetadata&&) = default;

ContentGroup SampleContentGroup(
    const std::vector<ContentGroup>& eligible_content_groups) {
  ContentGroup sampled_content_group;
  if (eligible_content_groups.empty()) {
    return sampled_content_group;
  }

  return PickRandom<ContentGroup>(eligible_content_groups);
}

bool TossCoin() {
  return base::RandDouble() < 0.5;
}

double GetNormal() {
  double u;
  double v;

  do {
    u = base::RandDouble();
  } while (u == 0);
  do {
    v = base::RandDouble();
  } while (v == 0);

  double result = sqrt(-2.0 * log(u)) * cos(2 * 3.1415 * v);
  result = result / 10 + 0.5;

  // Resample if outside the [0, 1] range
  if (result > 1 || result < 0) {
    return GetNormal();
  }

  return result;
}

int GetNormal(int min, int max) {
  return min + floor((max - min) * GetNormal());
}

std::optional<size_t> PickFirstIndex(const ArticleInfos& articles) {
  return articles.empty() ? std::nullopt : std::make_optional(0);
}

std::optional<size_t> PickRouletteWithWeighting(const ArticleInfos& articles,
                                                GetWeighting get_weighting) {
  std::vector<double> weights;
  base::ranges::transform(articles, std::back_inserter(weights),
                          [&get_weighting](const auto& article_info) {
                            return get_weighting.Run(std::get<0>(article_info),
                                                     std::get<1>(article_info));
                          });

  // None of the items are eligible to be picked.
  const auto total_weight =
      std::accumulate(weights.begin(), weights.end(), 0.0);
  if (total_weight == 0) {
    return std::nullopt;
  }

  double picked_value = base::RandDouble() * total_weight;
  double current_weight = 0;

  for (size_t i = 0; i < weights.size(); ++i) {
    current_weight += weights[i];
    if (current_weight >= picked_value) {
      return i;
    }
  }

  return std::nullopt;
}

std::optional<size_t> PickRoulette(const ArticleInfos& articles) {
  return PickRouletteWithWeighting(
      articles, base::BindRepeating([](const mojom::FeedItemMetadataPtr& data,
                                       const ArticleMetadata& meta) {
        return meta.subscribed ? meta.weighting : 0;
      }));
}

std::optional<size_t> PickChannelRoulette(const std::string& channel,
                                          const ArticleInfos& articles) {
  return PickRouletteWithWeighting(
      articles, base::BindRepeating(
                    [](const std::string& channel,
                       const mojom::FeedItemMetadataPtr& metadata,
                       const ArticleMetadata& weight) {
                      return base::Contains(weight.channels, channel)
                                 ? weight.weighting
                                 : 0.0;
                    },
                    channel));
}

}  // namespace brave_news
