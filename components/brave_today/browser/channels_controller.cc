// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/channels_controller.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "brave/components/brave_today/browser/brave_news_controller.h"
#include "brave/components/brave_today/browser/publishers_controller.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_news {

ChannelsController::ChannelsController(
    PrefService* prefs,
    PublishersController* publishers_controller)
    : prefs_(prefs), publishers_controller_(publishers_controller) {}

ChannelsController::~ChannelsController() = default;

Channels ChannelsController::GetChannelsFromPublishers(
    const Publishers& publishers,
    PrefService* prefs) {
  Channels channels;
  const auto& channel_subscriptions = prefs->GetDict(prefs::kBraveNewsChannels);

  for (const auto& it : publishers) {
    for (const auto& locale_info : it.second->locales) {
      for (const auto& channel_id : locale_info->channels) {
        auto& channel = channels[channel_id];

        // We already know we're subscribed to this channel in this locale.
        if (base::Contains(channel->subscribed_locales, locale_info->locale)) {
          continue;
        }

        channel->channel_name = channel_id;
        auto subscribed_in_locale =
            channel_subscriptions
                .FindBoolByDottedPath(locale_info->locale + "." + channel_id)
                .value_or(false);
        if (subscribed_in_locale)
          channel->subscribed_locales.push_back(locale_info->locale);
      }
    }
  }
  return channels;
}

std::vector<std::string> ChannelsController::GetChannelLocales() const {
  std::vector<std::string> result;
  const auto& pref = prefs_->GetDict(prefs::kBraveNewsChannels);

  for (const auto&& [locale, channel] : pref) {
    if (channel.DictEmpty())
      continue;
    result.push_back(locale);
  }

  return result;
}

std::vector<std::string> ChannelsController::GetChannelLocales(
    const std::string& channel_id) const {
  std::vector<std::string> result;
  const auto& pref = prefs_->GetDict(prefs::kBraveNewsChannels);
  for (const auto&& [locale, channels] : pref) {
    auto subscribed = channels.FindBoolKey(channel_id).value_or(false);
    if (subscribed)
      result.push_back(locale);
  }
  return result;
}

void ChannelsController::GetAllChannels(ChannelsCallback callback) {
  publishers_controller_->GetOrFetchPublishers(base::BindOnce(
      [](ChannelsCallback callback, PrefService* prefs, Publishers publishers) {
        auto result = GetChannelsFromPublishers(publishers, prefs);
        std::move(callback).Run(std::move(std::move(result)));
      },
      std::move(callback), base::Unretained(prefs_)));
}

mojom::ChannelPtr ChannelsController::SetChannelSubscribed(
    const std::string& locale,
    const std::string& channel_id,
    bool subscribed) {
  // Note: DictionaryPrefUpdate is nested here so the update happens before
  // we call |GetChannelLocales|.
  {
    DictionaryPrefUpdate update(prefs_, prefs::kBraveNewsChannels);
    const auto path = locale + "." + channel_id;
    if (!subscribed) {
      update->GetDict().RemoveByDottedPath(path);
    } else {
      update->GetDict().SetByDottedPath(path, true);
    }
  }

  auto result = mojom::Channel::New();
  result->channel_name = channel_id;
  result->subscribed_locales = GetChannelLocales(channel_id);

  return result;
}

bool ChannelsController::GetChannelSubscribed(const std::string& locale,
                                              const std::string& channel_id) {
  const auto& subscriptions = prefs_->GetDict(prefs::kBraveNewsChannels);
  return subscriptions.FindBoolByDottedPath(locale + "." + channel_id)
      .value_or(false);
}
}  // namespace brave_news
