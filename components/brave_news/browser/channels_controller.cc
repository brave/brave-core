// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/browser/channels_controller.h"

#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

namespace brave_news {

ChannelsController::ChannelsController(
    PublishersController* publishers_controller)
    : publishers_controller_(publishers_controller) {}

ChannelsController::~ChannelsController() = default;

Channels ChannelsController::GetChannelsFromPublishers(
    const Publishers& publishers,
    const BraveNewsSubscriptions& subscriptions) {
  Channels channels;
  for (const auto& it : publishers) {
    for (const auto& locale_info : it.second->locales) {
      for (const auto& channel_id : locale_info->channels) {
        auto migrated_channel_id = GetMigratedChannel(channel_id);
        if (!channels.contains(migrated_channel_id)) {
          auto channel = mojom::Channel::New();
          channel->channel_name = channel_id;
          channels[migrated_channel_id] = std::move(channel);
        }

        auto& channel = channels[migrated_channel_id];

        // We already know we're subscribed to this channel in this locale.
        if (base::Contains(channel->subscribed_locales, locale_info->locale)) {
          continue;
        }

        auto subscribed_in_locale = subscriptions.GetChannelSubscribed(
            locale_info->locale, migrated_channel_id);
        if (subscribed_in_locale) {
          channel->subscribed_locales.push_back(locale_info->locale);
        }
      }
    }
  }
  return channels;
}

void ChannelsController::GetAllChannels(
    const BraveNewsSubscriptions& subscriptions,
    ChannelsCallback callback) {
  publishers_controller_->GetOrFetchPublishers(
      subscriptions, base::BindOnce(
                         [](const BraveNewsSubscriptions& subscriptions,
                            ChannelsCallback callback, Publishers publishers) {
                           std::move(callback).Run(GetChannelsFromPublishers(
                               publishers, subscriptions));
                         },
                         subscriptions, std::move(callback)));
}

}  // namespace brave_news
