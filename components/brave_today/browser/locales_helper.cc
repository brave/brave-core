// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/locales_helper.h"
#include <string>
#include <vector>
#include "absl/types/optional.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/browser/urls.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom-shared.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/features.h"

namespace brave_news {
namespace {
bool HasAnyLocale(const base::flat_set<std::string>& locales,
                  const mojom::Publisher* publisher) {
  for (const auto& locale : publisher->locales) {
    if (locales.contains(locale))
      return true;
  }
  return false;
}

absl::optional<std::string> GetBestMissingLocale(
    const base::flat_set<std::string>& locales,
    const std::vector<mojom::Publisher*> publishers) {
  base::flat_map<std::string, uint32_t> missing_locale_counts;
  for (auto const* publisher : publishers) {
    // If this publisher is already covered by the list of locales we don't want
    // its locales to skew the list of what's missing.
    if (HasAnyLocale(locales, publisher))
      continue;

    for (const auto& locale : publisher->locales)
      missing_locale_counts[locale]++;
  }

  absl::optional<std::string> most_common;
  uint32_t max_occurences = 0;
  for (const auto& it : missing_locale_counts) {
    if (it.second > max_occurences) {
      most_common = it.first;
      max_occurences = it.second;
    }
  }
  return most_common;
}

}  // namespace

base::flat_set<std::string> GetPublisherLocales(const Publishers& publishers) {
  base::flat_set<std::string> result;
  for (const auto& it : publishers) {
    for (const auto& locale : it.second->locales)
      result.insert(locale);
  }
  return result;
}

std::vector<std::string> GetMinimalLocalesSet(
    const base::flat_set<std::string>& channel_locales,
    const Publishers& publishers) {
  if (!base::FeatureList::IsEnabled(
          brave_today::features::kBraveNewsV2Feature)) {
    return {brave_today::GetV1RegionUrlPart()};
  }

  base::flat_set<std::string> result;
  // All channel locales are part of the minimal set - we need all of them.
  for (const auto& locale : channel_locales)
    result.insert(locale);

  std::vector<mojom::Publisher*> subscribed_publishers;
  for (const auto& it : publishers) {
    // This API is only used by the V2 news API, so we don't need to care about
    // the legacy |.enabled| property.
    // We are only interested in explicitly enabled publishers, as channel
    // enabled ones will be covered by |channel_locales|.
    if (it.second->user_enabled_status != mojom::UserEnabled::ENABLED)
      continue;
    subscribed_publishers.push_back(it.second.get());
  }

  // While there are publishers which won't be included in the feed, add a new
  // locale and recalculate what's missing.
  absl::optional<std::string> best_missing_locale;
  while ((best_missing_locale =
              GetBestMissingLocale(result, subscribed_publishers))) {
    result.insert(best_missing_locale.value());
  }

  return std::vector<std::string>(result.begin(), result.end());
}
}  // namespace brave_news
