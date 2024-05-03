// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/channel_migrator.h"

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_news {

namespace {

constexpr auto kMigrateChannels =
    base::MakeFixedFlatMap<std::string_view, std::string_view>({
        {"Celebrity News", "Celebrities"},
        {"Entertainment News", "Entertainment"},
        {"Sport", "Sports"},
        {"Tech News", "Technology"},
        {"Tech Reviews", "Technology"},
    });

}  // namespace

void MigrateChannels(PrefService& prefs) {
  ScopedDictPrefUpdate update(&prefs, prefs::kBraveNewsChannels);

  auto& locale_channel_subscriptions = prefs.GetDict(prefs::kBraveNewsChannels);
  for (auto [locale, channels] : locale_channel_subscriptions) {
    const auto& channels_dict = channels.GetDict();

    for (const auto& [from, to] : kMigrateChannels) {
      auto maybe_migrate = channels_dict.FindBool(from);
      if (!maybe_migrate.value_or(false)) {
        continue;
      }

      // If we were subscribed to the channel "from", subscribe to the channel
      // "to" and remove that subscription.
      update->SetByDottedPath(base::StrCat({locale, ".", to}), true);
      update->RemoveByDottedPath(base::StrCat({locale, ".", from}));
    }
  }
}

std::string GetMigratedChannel(const std::string& channel) {
  const auto it = kMigrateChannels.find(channel);
  return it == kMigrateChannels.end() ? channel : std::string{it->second};
}

}  // namespace brave_news
