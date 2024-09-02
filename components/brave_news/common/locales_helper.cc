// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/locales_helper.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/l10n/common/locale_util.h"

namespace brave_news {
namespace {

// In lieue of a component providing a dynamic list, we keep a hard-coded
// list of matches for enabling Brave News on the NTP and prompting the user
// to opt-in.
constexpr auto kEnabledLanguages =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "en",
                                                 "ja",
                                             });
// We can add to this list as new locales become available to have Brave News
// show when it's ready for those users.
constexpr auto kEnabledLocales =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "de_DE",
                                                 "es_AR",
                                                 "es_ES",
                                                 "es_MX",
                                                 "fr_FR",
                                                 "pt_BR",
                                             });

bool HasAnyLocale(const base::flat_set<std::string>& locales,
                  const mojom::Publisher* publisher) {
  return base::ranges::any_of(publisher->locales,
                              [&locales](const auto& locale_info) {
                                return locales.contains(locale_info->locale);
                              });
}

std::optional<std::string> GetBestMissingLocale(
    const base::flat_set<std::string>& locales,
    const std::vector<mojom::Publisher*> publishers) {
  base::flat_map<std::string, uint32_t> missing_locale_counts;
  for (auto const* publisher : publishers) {
    // If this publisher is already covered by the list of locales we don't want
    // its locales to skew the list of what's missing.
    if (HasAnyLocale(locales, publisher)) {
      continue;
    }

    for (const auto& locale_info : publisher->locales) {
      missing_locale_counts[locale_info->locale]++;
    }
  }

  if (missing_locale_counts.empty()) {
    return {};
  }

  return base::ranges::max_element(
             missing_locale_counts,
             [](const auto& a, const auto& b) { return a.second < b.second; })
      ->first;
}

}  // namespace

base::flat_set<std::string> GetPublisherLocales(const Publishers& publishers) {
  base::flat_set<std::string> result;
  for (const auto& [_, publisher] : publishers) {
    for (const auto& locale_info : publisher->locales) {
      result.insert(locale_info->locale);
    }
  }
  return result;
}

base::flat_set<std::string> GetMinimalLocalesSet(
    const base::flat_set<std::string>& channel_locales,
    const Publishers& publishers) {
  // All channel locales are part of the minimal set - we need to include all of
  // them.
  base::flat_set<std::string> result = channel_locales;

  std::vector<mojom::Publisher*> subscribed_publishers;
  for (const auto& [id, publisher] : publishers) {
    // This API is only used by the V2 news API, so we don't need to care about
    // the legacy |.enabled| property.
    // We are only interested in explicitly enabled publishers, as channel
    // enabled ones will be covered by |channel_locales|.
    if (publisher->user_enabled_status != mojom::UserEnabled::ENABLED) {
      continue;
    }
    subscribed_publishers.push_back(publisher.get());
  }

  // While there are publishers which won't be included in the feed, add a new
  // locale and recalculate what's missing.
  std::optional<std::string> best_missing_locale;
  while ((best_missing_locale =
              GetBestMissingLocale(result, subscribed_publishers))) {
    result.insert(best_missing_locale.value());
  }

  return result;
}

bool IsUserInDefaultEnabledLocale() {
  // Only default Brave News to be shown for
  // certain languages and locales on browser startup.
  const std::string language_code =
      brave_l10n::GetDefaultISOLanguageCodeString();
  return (base::Contains(kEnabledLanguages, language_code) ||
          base::Contains(
              kEnabledLocales,
              base::StrCat({language_code, "_",
                            brave_l10n::GetDefaultISOCountryCodeString()})));
}

}  // namespace brave_news
