/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_handler.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/analytics/p2a/opportunities/p2a_opportunity.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/ads_feature.h"

namespace brave_ads {

namespace {

void FireServedEventCallback(
    const NewTabPageAdInfo& ad,
    MaybeServeNewTabPageAdCallback callback,
    const bool success,
    const std::string& /*placement_id*/,
    const mojom::NewTabPageAdEventType /*event_type*/) {
  if (!success) {
    return std::move(callback).Run(/*ad=*/std::nullopt);
  }

  std::move(callback).Run(ad);
}

void FireEventCallback(TriggerAdEventCallback callback,
                       const bool success,
                       const std::string& /*placement_id*/,
                       const mojom::NewTabPageAdEventType /*event_type*/) {
  std::move(callback).Run(success);
}

}  // namespace

NewTabPageAdHandler::NewTabPageAdHandler(
    Account& account,
    SiteVisit& site_visit,
    const SubdivisionTargeting& subdivision_targeting,
    const AntiTargetingResource& anti_targeting_resource)
    : account_(account),
      site_visit_(site_visit),
      serving_(subdivision_targeting, anti_targeting_resource) {
  event_handler_.SetDelegate(this);
  serving_.SetDelegate(this);
}

NewTabPageAdHandler::~NewTabPageAdHandler() = default;

void NewTabPageAdHandler::MaybeServe(MaybeServeNewTabPageAdCallback callback) {
  if (!UserHasOptedInToNewTabPageAds()) {
    return std::move(callback).Run(/*ad=*/std::nullopt);
  }

  if (!ShouldAlwaysTriggerNewTabPageAdEvents() &&
      !UserHasJoinedBraveRewards()) {
    return std::move(callback).Run(/*ad=*/std::nullopt);
  }

  serving_.MaybeServeAd(base::BindOnce(&NewTabPageAdHandler::MaybeServeCallback,
                                       weak_factory_.GetWeakPtr(),
                                       std::move(callback)));
}

void NewTabPageAdHandler::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    TriggerAdEventCallback callback) {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (!UserHasOptedInToNewTabPageAds()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (!UserHasJoinedBraveRewards() &&
      !ShouldAlwaysTriggerNewTabPageAdEvents()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (!UserHasJoinedBraveRewards() &&
      event_type == mojom::NewTabPageAdEventType::kViewedImpression) {
    // `MaybeServe()` will trigger a `kServedImpression` event if the user has
    // joined Brave Rewards; otherwise, we need to trigger a `kServedImpression`
    // event when triggering a `kViewedImpression` event for non-Brave-Rewards
    // users.
    return event_handler_.FireEvent(
        placement_id, creative_instance_id,
        mojom::NewTabPageAdEventType::kServedImpression,
        base::BindOnce(&NewTabPageAdHandler::TriggerServedEventCallback,
                       weak_factory_.GetWeakPtr(), creative_instance_id,
                       std::move(callback)));
  }

  event_handler_.FireEvent(
      placement_id, creative_instance_id, event_type,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAdHandler::MaybeServeCallback(
    MaybeServeNewTabPageAdCallback callback,
    const std::optional<NewTabPageAdInfo>& ad) {
  if (!ad) {
    return std::move(callback).Run(ad);
  }

  event_handler_.FireEvent(
      ad->placement_id, ad->creative_instance_id,
      mojom::NewTabPageAdEventType::kServedImpression,
      base::BindOnce(&FireServedEventCallback, *ad, std::move(callback)));
}

void NewTabPageAdHandler::TriggerServedEventCallback(
    const std::string& creative_instance_id,
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& placement_id,
    const mojom::NewTabPageAdEventType /*event_type*/) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  event_handler_.FireEvent(
      placement_id, creative_instance_id,
      mojom::NewTabPageAdEventType::kViewedImpression,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

void NewTabPageAdHandler::OnOpportunityAroseToServeNewTabPageAd() {
  BLOG(1, "Opportunity arose to serve a new tab page ad");

  RecordP2AAdOpportunity(AdType::kNewTabPageAd, /*segments=*/{});
}

void NewTabPageAdHandler::OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {
  BLOG(1, "Served new tab page ad impression:\n"
              << "  placementId: " << ad.placement_id << "\n"
              << "  creativeInstanceId: " << ad.creative_instance_id << "\n"
              << "  creativeSetId: " << ad.creative_set_id << "\n"
              << "  campaignId: " << ad.campaign_id << "\n"
              << "  advertiserId: " << ad.advertiser_id << "\n"
              << "  segment: " << ad.segment << "\n"
              << "  companyName: " << ad.company_name << "\n"
              << "  imageUrl: " << ad.image_url << "\n"
              << "  alt: " << ad.alt << "\n"
              << "  targetUrl: " << ad.target_url << "\n"
              << "  wallpaper:\n"
              << "    imageUrl: " << ad.wallpapers[0].image_url << "\n"
              << "    focalPoint:\n"
              << "      x: " << ad.wallpapers[0].focal_point.x << "\n"
              << "      y: " << ad.wallpapers[0].focal_point.y);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdServedEvent(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Served new tab page ad impression with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  ClientStateManager::GetInstance().UpdateSeenAd(ad);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdViewedEvent(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Viewed new tab page ad impression with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewedImpression);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kViewedImpression);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdClickedEvent(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Clicked new tab page ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  site_visit_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kClicked);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdMediaPlayEvent(
    const NewTabPageAdInfo& ad) {
  BLOG(3, "Started playing new tab page video ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kMediaPlay);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kMediaPlay);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdMedia25Event(
    const NewTabPageAdInfo& ad) {
  BLOG(3, R"(Played 25% of new tab page video ad with placement id )"
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kMedia25);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kMedia25);
}

void NewTabPageAdHandler::OnDidFireNewTabPageAdMedia100Event(
    const NewTabPageAdInfo& ad) {
  BLOG(3, R"(Played 100% of new tab page video ad with placement id )"
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kMedia100);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kMedia100);
}

}  // namespace brave_ads
