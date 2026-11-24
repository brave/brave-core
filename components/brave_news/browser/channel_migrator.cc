// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/channel_migrator.h"

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"

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

std::string GetMigratedChannel(const std::string& channel) {
  const auto* migrated_channel = base::FindOrNull(kMigrateChannels, channel);
  return migrated_channel ? std::string{*migrated_channel} : channel;
}

}  // namespace brave_news
