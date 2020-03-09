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

ExtensionRewardsServiceObserver::~ExtensionRewardsServiceObserver() {
}

void ExtensionRewardsServiceObserver::OnWalletInitialized(
    RewardsService* rewards_service,
    int32_t result) {
  auto* event_router = extensions::EventRouter::Get(profile_);

  auto converted_result = static_cast<ledger::Result>(result);

  // Don't report back if there is no ledger file
  if (event_router && converted_result != ledger::Result::NO_LEDGER_STATE) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_rewards::OnWalletInitialized::Create(
          result).release());

    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnWalletInitialized::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

void ExtensionRewardsServiceObserver::OnWalletProperties(
    RewardsService* rewards_service,
    int error_code,
    std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) {
  auto* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  if (error_code == 17) {  // ledger::Result::CORRUPT_WALLET
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_rewards::OnWalletInitialized::Create(
          error_code).release());
    std::unique_ptr<extensions::Event> event(new extensions::Event(
        extensions::events::BRAVE_START,
        extensions::api::brave_rewards::OnWalletInitialized::kEventName,
        std::move(args)));
    event_router->BroadcastEvent(std::move(event));
    return;
  }

  if (!wallet_properties) {
    return;
  }

  extensions::api::brave_rewards::OnWalletProperties::Properties properties;

  properties.default_tip_choices = wallet_properties->default_tip_choices;
  properties.default_monthly_tip_choices =
      wallet_properties->default_monthly_tip_choices;

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnWalletProperties::Create(properties)
          .release());

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_ON_WALLET_PROPERTIES,
      extensions::api::brave_rewards::OnWalletProperties::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnPanelPublisherInfo(
    RewardsService* rewards_service,
    int error_code,
    const ledger::PublisherInfo* info,
    uint64_t windowId) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router || !info) {
    return;
  }

  extensions::api::brave_rewards::OnPublisherData::Publisher publisher;

  publisher.percentage = info->percent;
  publisher.status = static_cast<int>(info->status);
  publisher.excluded = info->excluded == ledger::PublisherExclude::EXCLUDED;
  publisher.name = info->name;
  publisher.url = info->url;
  publisher.provider = info->provider;
  publisher.favicon_url = info->favicon_url;
  publisher.publisher_key = info->id;
  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnPublisherData::Create(windowId,
                                                              publisher)
          .release());

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_ON_PUBLISHER_DATA,
      extensions::api::brave_rewards::OnPublisherData::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnFetchPromotions(
    RewardsService* rewards_service,
    const uint32_t result,
    const std::vector<brave_rewards::Promotion>& list) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::vector<extensions::api::brave_rewards::OnPromotions::
        PromotionsType> promotions;

  for (size_t i = 0; i < list.size(); i ++) {
    promotions.push_back(
        extensions::api::brave_rewards::OnPromotions::PromotionsType());

    auto& promotion = promotions[promotions.size() -1];

    promotion.promotion_id = list[i].promotion_id;
    promotion.type = list[i].type;
    promotion.status = list[i].status;
    promotion.expires_at = list[i].expires_at;
    promotion.amount = list[i].amount;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnPromotions::Create(result, promotions)
          .release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPromotions::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnPromotionFinished(
    RewardsService* rewards_service,
    const uint32_t result,
    brave_rewards::Promotion promotion) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router || result != 0) {
    return;
  }

  extensions::api::brave_rewards::OnPromotionFinish::
        Promotion promotion_api;

  promotion_api.promotion_id = promotion.promotion_id;
  promotion_api.type = promotion.type;
  promotion_api.status = promotion.status;
  promotion_api.expires_at = promotion.expires_at;
  promotion_api.amount = promotion.amount;

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnPromotionFinish::Create
      (result, promotion_api).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPromotionFinish::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnAdsEnabled(
    RewardsService* rewards_service,
    bool ads_enabled) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnAdsEnabled::Create(
          ads_enabled).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnAdsEnabled::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnRewardsMainEnabled(
    RewardsService* rewards_service,
    bool rewards_main_enabled) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnEnabledMain::Create(
          rewards_main_enabled).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnEnabledMain::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnPendingContributionSaved(
    RewardsService* rewards_service,
    int result) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnPendingContributionSaved::Create(result)
          .release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPendingContributionSaved::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnPublisherListNormalized(
    RewardsService* rewards_service,
    const brave_rewards::ContentSiteList& list) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::vector<extensions::api::brave_rewards::OnPublisherListNormalized::
        PublishersType> publishers;

  for (size_t i = 0; i < list.size(); i ++) {
    publishers.push_back(
        extensions::api::brave_rewards::OnPublisherListNormalized::
        PublishersType());

    auto& publisher = publishers[publishers.size() -1];

    publisher.publisher_key = list[i].id;
    publisher.percentage = list[i].percentage;
    publisher.status = list[i].status;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::
      OnPublisherListNormalized::Create(publishers)
          .release());

  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPublisherListNormalized::kEventName,
      std::move(args)));
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

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnExcludedSitesChanged::Create(result)
          .release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnExcludedSitesChanged::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnRecurringTipSaved(
    RewardsService* rewards_service,
    bool success) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnRecurringTipSaved::Create(
          success).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnRecurringTipSaved::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnRecurringTipRemoved(
    RewardsService* rewards_service,
    bool success) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnRecurringTipRemoved::Create(
          success).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnRecurringTipRemoved::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnPendingContributionRemoved(
    RewardsService* rewards_service,
    int32_t result) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnPendingContributionRemoved::Create(
          result).release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnPendingContributionRemoved::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnReconcileComplete(
    RewardsService* rewards_service,
    unsigned int result,
    const std::string& contribution_id,
    const double amount,
    const int32_t type) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  extensions::api::brave_rewards::OnReconcileComplete::Properties properties;
  properties.result = result;
  properties.type = type;

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnReconcileComplete::Create(properties)
          .release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnReconcileComplete::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      int32_t result,
      const std::string& wallet_type) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  extensions::api::brave_rewards::OnDisconnectWallet::Properties properties;
  properties.result = result;
  properties.wallet_type = wallet_type;

  std::unique_ptr<base::ListValue> args(
      extensions::api::brave_rewards::OnDisconnectWallet::Create(properties)
          .release());
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnDisconnectWallet::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

void ExtensionRewardsServiceObserver::OnUnblindedTokensReady(
      brave_rewards::RewardsService* rewards_service) {
  auto* event_router = extensions::EventRouter::Get(profile_);
  if (!event_router) {
    return;
  }

  auto args = std::make_unique<base::ListValue>();
  std::unique_ptr<extensions::Event> event(new extensions::Event(
      extensions::events::BRAVE_START,
      extensions::api::brave_rewards::OnUnblindedTokensReady::kEventName,
      std::move(args)));
  event_router->BroadcastEvent(std::move(event));
}

}  // namespace brave_rewards
