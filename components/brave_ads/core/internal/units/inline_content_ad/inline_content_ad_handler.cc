/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/inline_content_ad/inline_content_ad_handler.h"

#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/inline_content_ad/inline_content_ad_info.h"

namespace brave_ads {

namespace {

void FireServedEventCallback(
    const std::string& dimensions,
    const InlineContentAdInfo& ad,
    MaybeServeInlineContentAdCallback callback,
    const bool success,
    const std::string& /*placement_id=*/,
    const mojom::InlineContentAdEventType /*event_type=*/) {
  if (!success) {
    return std::move(callback).Run(dimensions, /*ad=*/absl::nullopt);
  }

  std::move(callback).Run(dimensions, ad);
}

void FireEventCallback(TriggerAdEventCallback callback,
                       const bool success,
                       const std::string& /*placement_id=*/,
                       const mojom::InlineContentAdEventType /*event_type=*/) {
  std::move(callback).Run(success);
}

}  // namespace

InlineContentAdHandler::InlineContentAdHandler(
    Account& account,
    Transfer& transfer,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : account_(account),
      transfer_(transfer),
      serving_(subdivision_targeting, anti_targeting_resource) {
  event_handler_.SetDelegate(this);
  serving_.SetDelegate(this);
  TabManager::GetInstance().AddObserver(this);
}

InlineContentAdHandler::~InlineContentAdHandler() {
  TabManager::GetInstance().RemoveObserver(this);
}

void InlineContentAdHandler::MaybeServe(
    const std::string& dimensions,
    MaybeServeInlineContentAdCallback callback) {
  if (!UserHasOptedInToBraveNewsAds()) {
    return std::move(callback).Run(dimensions, /*ad=*/absl::nullopt);
  }

  serving_.MaybeServeAd(
      dimensions,
      base::BindOnce(&InlineContentAdHandler::MaybeServeCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void InlineContentAdHandler::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));
  CHECK_NE(mojom::InlineContentAdEventType::kServed, event_type)
      << "Should not be called with kServed as this event is handled when "
         "calling MaybeServe";

  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (!UserHasOptedInToBraveNewsAds()) {
    return std::move(callback).Run(/*success=*/false);
  }

  event_handler_.FireEvent(
      placement_id, creative_instance_id, event_type,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void InlineContentAdHandler::MaybeServeCallback(
    MaybeServeInlineContentAdCallback callback,
    const std::string& dimensions,
    const absl::optional<InlineContentAdInfo>& ad) {
  if (!ad) {
    return std::move(callback).Run(dimensions, ad);
  }

  event_handler_.FireEvent(ad->placement_id, ad->creative_instance_id,
                           mojom::InlineContentAdEventType::kServed,
                           base::BindOnce(&FireServedEventCallback, dimensions,
                                          *ad, std::move(callback)));
}

void InlineContentAdHandler::CacheAdPlacement(const int32_t tab_id,
                                              const InlineContentAdInfo& ad) {
  BLOG(1, "Cached inline content ad with placement id "
              << ad.placement_id << " and tab id " << tab_id);

  placement_ids_[tab_id].push_back(ad.placement_id);
}

void InlineContentAdHandler::PurgeOrphanedCachedAdPlacements(
    const int32_t tab_id) {
  if (placement_ids_[tab_id].empty()) {
    return;
  }

  BLOG(1, "Purged orphaned inline content ad placements for tab id " << tab_id);

  PurgeOrphanedAdEvents(
      placement_ids_[tab_id],
      base::BindOnce(
          [](const std::vector<std::string>& placement_ids,
             const bool success) {
            const std::string joined_placement_ids =
                base::JoinString(placement_ids, ", ");

            if (!success) {
              return BLOG(
                  0, "Failed to purge orphaned inline content ad events for "
                         << joined_placement_ids << " placement ids");
            }

            BLOG(1, "Successfully purged orphaned inline content ad events for "
                        << joined_placement_ids << " placement ids");
          },
          placement_ids_[tab_id]));

  placement_ids_.erase(tab_id);
}

void InlineContentAdHandler::OnOpportunityAroseToServeInlineContentAd() {
  BLOG(1, "Opportunity arose to serve an inline content ad");

  RecordP2AAdOpportunity(AdType::kInlineContentAd, /*segments=*/{});
}

void InlineContentAdHandler::OnDidServeInlineContentAd(
    const int32_t tab_id,
    const InlineContentAdInfo& ad) {
  CacheAdPlacement(tab_id, ad);

  BLOG(1, "Served inline content ad:\n"
              << "  placementId: " << ad.placement_id << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  title: " << ad.title << "\n"
              << "  description: " << ad.description << "\n"
              << "  imageUrl: " << ad.image_url << "\n"
              << "  dimensions: " << ad.dimensions << "\n"
              << "  ctaText: " << ad.cta_text << "\n"
              << "  targetUrl: " << ad.target_url);
}

void InlineContentAdHandler::OnDidFireInlineContentAdServedEvent(
    const InlineContentAdInfo& ad) {
  BLOG(3, "Served inline content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  ClientStateManager::GetInstance().UpdateSeenAd(ad);
}

void InlineContentAdHandler::OnDidFireInlineContentAdViewedEvent(
    const InlineContentAdInfo& ad) {
  BLOG(3, "Viewed inline content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kViewed);
}

void InlineContentAdHandler::OnDidFireInlineContentAdClickedEvent(
    const InlineContentAdInfo& ad) {
  BLOG(3, "Clicked inline content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kClicked);
}

void InlineContentAdHandler::OnTabDidChange(const TabInfo& tab) {
  PurgeOrphanedCachedAdPlacements(tab.id);
}

void InlineContentAdHandler::OnDidCloseTab(const int32_t tab_id) {
  PurgeOrphanedCachedAdPlacements(tab_id);
}

}  // namespace brave_ads
