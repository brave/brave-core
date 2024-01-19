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
#include "base/values.h"
#include "brave/components/brave_news/browser/brave_news_p3a.h"
#include "brave/components/brave_news/browser/channel_migrator.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "mojo/public/cpp/bindings/remote_set.h"

namespace brave_news {
namespace {
bool IsChannelSubscribedInLocale(const base::Value::Dict& subscriptions,
                                 const std::string& locale,
                                 const std::string& channel_id) {
  const auto* locale_dict = subscriptions.FindDict(locale);
  if (!locale_dict) {
    return false;
  }

  return locale_dict->FindBool(channel_id).value_or(false);
}
}  // namespace

// static
void ChannelsController::SetChannelSubscribedPref(PrefService* prefs,
                                                  const std::string& locale,
                                                  const std::string& channel_id,
                                                  bool subscribed) {
  ScopedDictPrefUpdate update(prefs, prefs::kBraveNewsChannels);
  auto* dict = update->EnsureDict(locale);
  if (!subscribed) {
    dict->Remove(channel_id);
  } else {
    dict->Set(channel_id, true);
  }
}

ChannelsController::ChannelsController(
    PrefService* prefs,
    PublishersController* publishers_controller,
    p3a::NewsMetrics* news_metrics)
    : prefs_(prefs),
      publishers_controller_(publishers_controller),
      news_metrics_(news_metrics) {
  MigrateChannels(*prefs);
  scoped_observation_.Observe(publishers_controller_);
}

ChannelsController::~ChannelsController() = default;

Channels ChannelsController::GetChannelsFromPublishers(
    const Publishers& publishers,
    PrefService* prefs) {
  Channels channels;
  const auto& channel_subscriptions = prefs->GetDict(prefs::kBraveNewsChannels);

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

        auto subscribed_in_locale = IsChannelSubscribedInLocale(
            channel_subscriptions, locale_info->locale, migrated_channel_id);
        if (subscribed_in_locale) {
          channel->subscribed_locales.push_back(locale_info->locale);
        }
      }
    }
  }
  return channels;
}

std::vector<std::string> ChannelsController::GetChannelLocales() const {
  std::vector<std::string> result;
  const auto& pref = prefs_->GetDict(prefs::kBraveNewsChannels);

  for (const auto&& [locale, channel] : pref) {
    if (channel.GetDict().empty()) {
      continue;
    }
    result.push_back(locale);
  }

  return result;
}

std::vector<std::string> ChannelsController::GetChannelLocales(
    const std::string& channel_id) const {
  std::vector<std::string> result;
  const auto& pref = prefs_->GetDict(prefs::kBraveNewsChannels);
  for (const auto&& [locale, channels] : pref) {
    auto subscribed = channels.GetDict().FindBool(channel_id).value_or(false);
    if (subscribed) {
      result.push_back(locale);
    }
  }
  return result;
}

void ChannelsController::GetAllChannels(ChannelsCallback callback) {
  publishers_controller_->GetOrFetchPublishers(base::BindOnce(
      [](ChannelsController* controller, ChannelsCallback callback,
         PrefService* prefs, Publishers publishers) {
        auto result = GetChannelsFromPublishers(publishers, prefs);
        // fix this. result is a map
        controller->enabled_channel_count_ = 0;
        for (const auto& it : result) {
          if (!it.second->subscribed_locales.empty()) {
            controller->enabled_channel_count_++;
          }
        }
        if (controller->news_metrics_) {
          controller->news_metrics_->RecordTotalSubscribedCount(
              p3a::SubscribeType::kChannels,
              controller->enabled_channel_count_);
        }
        std::move(callback).Run(std::move(std::move(result)));
      },
      base::Unretained(this), std::move(callback), base::Unretained(prefs_)));
}

void ChannelsController::AddListener(
    mojo::PendingRemote<mojom::ChannelsListener> listener) {
  auto id = listeners_.Add(std::move(listener));
  GetAllChannels(base::BindOnce(
      [](ChannelsController* controller, mojo::RemoteSetElementId id,
         Channels channels) {
        auto* listener = controller->listeners_.Get(id);
        if (listener) {
          auto event = mojom::ChannelsEvent::New();
          event->addedOrUpdated = std::move(channels);
          listener->Changed(std::move(event));
        }
      },
      base::Unretained(this), id));
}

mojom::ChannelPtr ChannelsController::SetChannelSubscribed(
    const std::string& locale,
    const std::string& channel_id,
    bool subscribed) {
  // Persist the pref
  ChannelsController::SetChannelSubscribedPref(prefs_, locale, channel_id,
                                               subscribed);

  // Provide an updated entity
  auto result = mojom::Channel::New();
  result->channel_name = channel_id;
  result->subscribed_locales = GetChannelLocales(channel_id);

  for (const auto& listener : listeners_) {
    auto event = mojom::ChannelsEvent::New();
    event->addedOrUpdated[channel_id] = result->Clone();
    listener->Changed(std::move(event));
  }

  if (subscribed) {
    enabled_channel_count_++;
  } else if (enabled_channel_count_ > 0) {
    enabled_channel_count_--;
  }
  if (news_metrics_) {
    news_metrics_->RecordTotalSubscribedCount(p3a::SubscribeType::kChannels,
                                              enabled_channel_count_);
  }

  return result;
}

bool ChannelsController::GetChannelSubscribed(const std::string& locale,
                                              const std::string& channel_id) {
  const auto& subscriptions = prefs_->GetDict(prefs::kBraveNewsChannels);
  return IsChannelSubscribedInLocale(subscriptions, locale, channel_id);
}

void ChannelsController::OnPublishersUpdated(PublishersController* controller) {
  GetAllChannels(base::BindOnce(
      [](ChannelsController* controller, Channels channels) {
        auto event = mojom::ChannelsEvent::New();
        event->addedOrUpdated = std::move(channels);
        for (const auto& listener : controller->listeners_) {
          listener->Changed(event->Clone());
        }
      },
      base::Unretained(this)));
}

}  // namespace brave_news
