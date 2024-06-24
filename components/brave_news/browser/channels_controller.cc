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
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

namespace brave_news {

std::vector<std::string> GetChannelsForPublisher(
    const std::string& locale,
    const mojom::PublisherPtr& publisher) {
  std::vector<std::string> result;
  for (const auto& locale_info : publisher->locales) {
    if (locale_info->locale != locale) {
      continue;
    }
    for (const auto& channel : locale_info->channels) {
      result.push_back(channel);
    }
  }
  return result;
}

ChannelsController::ChannelsController(
    PublishersController* publishers_controller)
    : publishers_controller_(publishers_controller) {}

ChannelsController::~ChannelsController() = default;

Channels ChannelsController::GetChannelsFromPublishers(
    const Publishers& publishers,
    const SubscriptionsSnapshot& subscriptions) {
  Channels channels;
  for (const auto& it : publishers) {
    // Collect all channels from all locales first...
    for (const auto& locale_info : it.second->locales) {
      for (const auto& channel_id : locale_info->channels) {
        auto migrated_channel_id = GetMigratedChannel(channel_id);
        if (!channels.contains(migrated_channel_id)) {
          auto channel = mojom::Channel::New();
          channel->channel_name = channel_id;
          channels[migrated_channel_id] = std::move(channel);
        }
      }
    }
    // ...and then check subscription statuses.
    // We iterate twice (once above, and once here),
    // just in case the user is subscribed to a channel
    // that does not have sources for their locale. In that case,
    // the channel struct could be created via occurence in another
    // locale, which would result in potential missing subscription statuses
    // if we only iterated once.
    for (const auto& locale_info : it.second->locales) {
      for (auto& [channel_id, channel] : channels) {
        // We already know we're subscribed to this channel in this locale.
        if (base::Contains(channel->subscribed_locales, locale_info->locale)) {
          continue;
        }

        auto subscribed_in_locale =
            subscriptions.GetChannelSubscribed(locale_info->locale, channel_id);
        if (subscribed_in_locale) {
          channel->subscribed_locales.push_back(locale_info->locale);
        }
      }
    }
  }
  return channels;
}

void ChannelsController::GetAllChannels(
    const SubscriptionsSnapshot& subscriptions,
    ChannelsCallback callback) {
  publishers_controller_->GetOrFetchPublishers(
      subscriptions, base::BindOnce(
                         [](const SubscriptionsSnapshot& subscriptions,
                            ChannelsCallback callback, Publishers publishers) {
                           std::move(callback).Run(GetChannelsFromPublishers(
                               publishers, subscriptions));
                         },
                         subscriptions, std::move(callback)));
}

}  // namespace brave_news
