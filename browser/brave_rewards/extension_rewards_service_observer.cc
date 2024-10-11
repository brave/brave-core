/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/extension_rewards_service_observer.h"

#include <utility>
#include <string>
#include <vector>

#include "base/base64.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/event_router.h"

namespace brave_rewards {

ExtensionRewardsServiceObserver::ExtensionRewardsServiceObserver(
    Profile* profile)
    : profile_(profile) {
}

ExtensionRewardsServiceObserver::~ExtensionRewardsServiceObserver() = default;

void ExtensionRewardsServiceObserver::OnRewardsInitialized(
    RewardsService* rewards_service) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::Initialized::kEventName,
      extensions::api::brave_rewards::Initialized::Create(0)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnRewardsWalletCreated() {
  if (auto* event_router = extensions::EventRouter::Get(profile_)) {
    event_router->BroadcastEvent(std::make_unique<extensions::Event>(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnRewardsWalletCreated::kEventName,
        base::Value::List()));
  }
}

void ExtensionRewardsServiceObserver::OnTermsOfServiceUpdateAccepted() {
  if (auto* event_router = extensions::EventRouter::Get(profile_)) {
    event_router->BroadcastEvent(std::make_unique<extensions::Event>(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnTermsOfServiceUpdateAccepted::
            kEventName,
        base::Value::List()));
  }
}

void ExtensionRewardsServiceObserver::OnPanelPublisherInfo(
    RewardsService* rewards_service,
    const brave_rewards::mojom::Result result,
    const brave_rewards::mojom::PublisherInfo* info,
    uint64_t windowId) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router || !info) {
    return;
  }

  extensions::api::brave_rewards::OnPublisherData::Publisher publisher;

  publisher.percentage = info->percent;
  publisher.status = static_cast<int>(info->status);
  publisher.excluded =
      info->excluded == brave_rewards::mojom::PublisherExclude::EXCLUDED;
  publisher.name = info->name;
  publisher.url = info->url;
  publisher.provider = info->provider;
  publisher.fav_icon_url = info->favicon_url;
  publisher.publisher_key = info->id;

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_ON_PUBLISHER_DATA,
      extensions::api::brave_rewards::OnPublisherData::kEventName,
      extensions::api::brave_rewards::OnPublisherData::Create(windowId,
                                                              publisher)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnPublisherListNormalized(
    RewardsService* rewards_service,
    std::vector<brave_rewards::mojom::PublisherInfoPtr> list) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::vector<extensions::api::brave_rewards::OnPublisherListNormalized::
        PublishersType> publishers;

  publishers.reserve(list.size());
  for (const auto& item : list) {
    extensions::api::brave_rewards::OnPublisherListNormalized::PublishersType
        publisher;
    publisher.publisher_key = item->id;
    publisher.percentage = item->percent;
    publisher.status = static_cast<int>(item->status);
    publishers.emplace_back(std::move(publisher));
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPublisherListNormalized::kEventName,
      extensions::api::brave_rewards::OnPublisherListNormalized::Create(
          publishers)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnExcludedSitesChanged(
    RewardsService* rewards_service,
    std::string publisher_key,
    bool excluded) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  extensions::api::brave_rewards::OnExcludedSitesChanged::Properties result;
  result.publisher_key = publisher_key;
  result.excluded = excluded;

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnExcludedSitesChanged::kEventName,
      extensions::api::brave_rewards::OnExcludedSitesChanged::Create(result)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnRecurringTipSaved(
    RewardsService* rewards_service,
    bool success) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnRecurringTipSaved::kEventName,
      extensions::api::brave_rewards::OnRecurringTipSaved::Create(success)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnRecurringTipRemoved(
    RewardsService* rewards_service,
    bool success) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnRecurringTipRemoved::kEventName,
      extensions::api::brave_rewards::OnRecurringTipRemoved::Create(success)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnReconcileComplete(
    RewardsService* rewards_service,
    const brave_rewards::mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const brave_rewards::mojom::RewardsType type,
    const brave_rewards::mojom::ContributionProcessor processor) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  extensions::api::brave_rewards::OnReconcileComplete::Properties properties;
  properties.result = static_cast<int>(result);
  properties.type = static_cast<int>(type);

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnReconcileComplete::kEventName,
      extensions::api::brave_rewards::OnReconcileComplete::Create(properties)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnExternalWalletConnected() {
  if (auto* event_router = extensions::EventRouter::Get(profile_)) {
    event_router->BroadcastEvent(std::make_unique<extensions::Event>(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnExternalWalletConnected::kEventName,
        base::Value::List()));
  }
}

void ExtensionRewardsServiceObserver::OnExternalWalletLoggedOut() {
  if (auto* event_router = extensions::EventRouter::Get(profile_)) {
    event_router->BroadcastEvent(std::make_unique<extensions::Event>(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnExternalWalletLoggedOut::kEventName,
        base::Value::List()));
  }
}

void ExtensionRewardsServiceObserver::OnExternalWalletDisconnected() {
  if (auto* event_router = extensions::EventRouter::Get(profile_)) {
    event_router->BroadcastEvent(std::make_unique<extensions::Event>(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnExternalWalletDisconnected::
            kEventName,
        base::Value::List()));
  }
}

void ExtensionRewardsServiceObserver::OnCompleteReset(const bool success) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  extensions::api::brave_rewards::OnCompleteReset::Properties properties;
  properties.success = success;

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnCompleteReset::kEventName,
      extensions::api::brave_rewards::OnCompleteReset::Create(properties)));
  event_router->BroadcastEvent(std::move(event));
}

}  // namespace brave_rewards
